#!/bin/bash
# helper functions for downloading files or packages

# download a file. usage:
# download FROM [TO]
# 1. download "http://example.com/something.tar.gz"
# 2. download "http://example.com/something.tar.gz" "somename.tar.gz"
download () {
	[ $# -lt 1 -o $# -gt 2 ] && perror "download called with $# arguments, need 1 or 2"
	[ -z "$1" ] && perror "download: URL empty."
	local URL="$1"
	# If a sourceforge mirror is set in ./config, try to use it
	if [ -n "$sourceforge_mirror" ] && [[ "$URL" != *use_mirror* ]] && [[ "$URL" == *sourceforge.net* || "$URL" == *.sf.net* ]]; then
		if [[ "$URL" == *\?* ]]; then
			URL+="&use_mirror=$sourceforge_mirror"
		else
			URL+="?use_mirror=$sourceforge_mirror"
		fi
	fi
	if [ $# -eq 2 ]; then
		[ -z "$2" ] && perror "download: target file name given but empty"
		pinfo "Downloading $2 from '$URL'...."
		wget -O "$2" "$URL"
		local RET=$?
	else
		pinfo "Downloading '$URL'...."
		wget "$URL"
		local RET=$?
	fi
	[ "x$RET" != "x0" ] && perror "downloading $URL failed, wget returned exit code $RET"
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
	[ -s "$DST" ] && pdebug "Not downloading $DST: already there." && return 0
	pdebug "Downloading $DST"
	[ -e "$DST" ] && unlink "$DST"
	local DSTDIR="$(dirname "$DST")"
	pdebug "wgetting $SRC"
	local TMP=$(mktemp)
	wget -O "$TMP" "$SRC"
	local RET=$?
	if [ "x$RET" = "x0" -a -s "$TMP" ]; then
		mkdir -p "$DSTDIR"
		mv "$TMP" "$DST" || perror "Could not mv '$TMP' '$DST'"
		return 0
	fi
	pdebug "WGET failed"
	rmdir "$DSTDIR"
	unlink "$TMP"
	return 1
}

autoclone () {
	[ -z "$REQUIRED_GIT" ] && return 0
	local url branch dst dstdir checkout
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
		dst="${url%.git}"
		pinfo "Cloning $url to $dst"
		if [ -n "$branch" ]; then
			git clone -b "$branch" "$url" "$dstdir/$dst" || perror "Could not clone $url branch $branch"
		else
			git clone "$url" "$dstdir/$dst" || perror "Could not clone $url"
			if [ -n "$checkout" ]; then
				cd "$dstdir/$dst" || perror "cd error. check redbook."
				git checkout "$checkout" || perror "Could not checkout $checkout from $url"
			fi
		fi
	done
	return 0
}

