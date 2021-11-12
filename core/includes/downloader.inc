#!/bin/bash
# helper functions for downloading files or packages

# download a file. usage:
# download FROM [TO]
# 1. download "http://example.com/something.tar.gz"
# 2. download "http://example.com/something.tar.gz" "somename.tar.gz"
download () {
	[ $# -lt 1 -o $# -gt 2 ] && perror "download: called with $# arguments, need 1 or 2"
	[ -z "$1" ] && perror "download: url empty."
	local url path filename ret tryhash tmpfile f sum origname
	url="$1"
	# If a sourceforge mirror is set in ./config, try to use it
	if [ -n "$sourceforge_mirror" ] && [[ "$url" != *use_mirror* ]] && [[ "$url" == *sourceforge.net* || "$url" == *.sf.net* ]]; then
		if [[ "$url" == *\?* ]]; then
			url+="&use_mirror=$sourceforge_mirror"
		else
			url+="?use_mirror=$sourceforge_mirror"
		fi
	fi
	path="${url%%'?'*}"
	tryhash=false
	[ "$path" = "$url" ] && tryhash=true
	origname="${path##*/}"
	if [ $# -eq 2 ]; then
		[ -z "$2" ] && perror "download: target file name given but empty"
		filename="$2"
	else
		filename="${origname}"
		[ -z "$filename" ] && perror "download: Cannot determine filename from '$url'"
	fi
	pinfo "Downloading '$filename' from '$url'...."
	wget -O "$filename" "$url"
	ret=$?
	[ "x$ret" != "x0" ] && perror "downloading $url failed, wget returned exit code $ret"
	if $tryhash; then
		path="${path%/*}"
		[ "${path:0:5}" = "http:" ] && path="https:${path:5}"
		tmpfile="$(mktemp)"
		for f in sha256sums.asc SHA256SUMS md5sums.asc MD5SUMS metadata.xml.gz ""; do
			[ -z "$f" ] && break
			wget -O "$tmpfile" "$path/$f" || continue
			# Some ret^W less smart companies decide to just 302 requests for non-existing files/pages
			[ "$( stat -c %s "$tmpfile" )" -lt 100000 ] || continue
			[ "$( grep -c '<' "$tmpfile" )" -lt 10 ] || continue
			if [ "$f" = "metadata.xml.gz" ]; then
				sum=$( gzip -d < "$tmpfile" | grep '<checksum>.*</checksum>' | sed -r 's/<[^>]*>//g' )
				[ -z "$sum" ] && perror "Did not find sha256 in $f"
				[ "$(sha256sum "$filename" | awk '{print $1}')" = "$sum" ] && break
				perror "download: sha256 of '$filename' was expected to be $sum"
			elif [[ "$f" == SHA256SUM* || "$f" == sha256sum* ]]; then
				checkhash sha256sum "$filename" "$tmpfile" "$origname" && break
				perror "download: sha256 of '$filename' is bad"
			elif [[ "$f" == MD5SUM* || "$f" == md5sum* ]]; then
				checkhash md5sum "$filename" "$tmpfile" "$origname" && break
				perror "download: md5 of '$filename' is bad"
			fi
		done
		rm -f -- "$tmpfile"
		[ -n "$f" ] && pinfo "Checksum of downloaded file is valid :-)"
	fi
}

# [algo] [file_to_hash] [sumfile] [name_in_sumfile]
checkhash () {
	local algo sum fth sumfile nis mysum
	algo="$1"
	fth="$2"
	sumfile="$3"
	nis="$4"
	mysum="$("$algo" "$fth" | awk '{print $1}')"
	sum=$(grep -F "$nis" "$sumfile" | awk '{print $1}')
	[ "x$sum" = "x$mysum" ]
}

# download a file and untar it. usage:
# download_untar FROM TO_DIR [TEMPFILE]
# 1. download_untar "http://example.com/something.tar.gz" "src/"
# 2. download_untar "http://example.com/something.tar.gz" "src/" "temporary_name.tar.gz"
download_untar () {
	[ $# -lt 2 -o $# -gt 3 ] && perror "download_untar called with $# arguments, need 2 or 3"
	local URL="$1"
	local DEST="$2"
	if [ $# -eq 2 ]; then
		local TMPFILE=dltmp.$(basename "$URL")
	else
		local TMPFILE="$3"
	fi
	pdebug "$URL ---> $TMPFILE"
	download "$URL" "$TMPFILE"
	mkdir -p "$DEST"
	pinfo "Unpacking to '$DEST'..."
	tar xf "$TMPFILE" -C "${DEST}/"
	local RET=$?
	[ "x$RET" != "x0" ] && perror "could not untar $TMPFILE to $DEST (tar returned $RET)"
	unlink "$TMPFILE"
}

# Download first param URL to second param path,
# iff the local file does not exist or is empty.
# Return 1 if remote file does not exist, 0 otherwise
download_if_empty() {
	[ $# -ne 2 ] && perror "download_if_empty: want 2 args, got $# ($@)"
	local SRC="$1"
	local DST="$2"
	if [ -s "$DST" ]; then
		pdebug "Not downloading $DST: already there."
		return 0
	fi
	download "$SRC" "$DST"
}

# Clone the REQUIRED_GIT git repositories.
# Syntax is:
# <url>
# <url>|branch
# <url>||commit-ish
# Second syntax is mostly for git servers that don't accept a non-branch when shallow-cloning
autoclone () {
	[ -z "$REQUIRED_GIT" ] && return 0
	local url branch dst dstdir checkout i
	if [ -n "$1" ]; then
		dstdir="$1"
	else
		dstdir="${MODULE_WORK_DIR}/src"
	fi
	mkdir -p "$dstdir" || perror "Could not create $dstdir"
	for url in $REQUIRED_GIT; do
		branch=
		checkout="${url#*||}"
		if [ "x$checkout" = "x$url" ]; then
			checkout=
			branch="${url#*|}"
			[ "x$branch" = "x$url" ] && branch=
		fi
		url="${url%%|*}"
		dst="${url##*/}"
		dst="${dst%.git}"
		pinfo "Cloning $url to $dst"
		for i in {1..3} fail; do
			[ "$i" = "fail" ] && perror "Could not clone."
			if [ -n "$branch" ]; then
				git clone --depth 1 -b "$branch" "$url" "$dstdir/$dst" && break
				pwarning "[Try $i] Could not clone $url branch $branch"
			else
				if ! git clone "$url" "$dstdir/$dst"; then
					pwarning "[Try $i] Could not clone $url"
				elif [ -n "$checkout" ]; then
					git -C "$dstdir/$dst" checkout "$checkout" && break
					perror "Could not checkout $checkout from $url"
				fi
			fi
		done
		[ -n "$AUTOCLONE_FILE" ] \
			&& echo "$dst: $( git -C "$dstdir/$dst" rev-parse --abbrev-ref HEAD ) $( git -C "$dstdir/$dst" rev-parse --short HEAD )" \
			>> "$AUTOCLONE_FILE"
	done
	return 0
}

