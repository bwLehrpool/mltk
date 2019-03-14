#!/bin/bash
########################################################
# Include: Detect, whether runvirt runs in legacy mode #
########################################################
## Functions ##
# Legacy mode: As runvirt has been before.
# New mode: uuid in xml _and_ vmx given via http.
download_metadata() {
	local TRIES EXTRA_ERROR WLOG TMPFILE RET ERRCODE CMDLINE
	# Sanity checks
	if ! check_dep wget; then
		writelog "Could not find 'wget' in PATH: $PATH"
		EXIT_TYPE="internal" EXIT_REASON="Fehlendes Dienstprogramm 'wget'!" cleanexit 1
	fi

	writelog "Detecting current/legacy mode..."
	declare -rg TMPCONFIG="$CONFDIR/vmx"

	# Try new unified .tar.gz way
	TMPFILE="$TMPDIR/metadata.tgz"
	WLOG="$TMPDIR/wget-metadata.log"
	CMDLINE=
	curl --help | grep -q -- --retry-connrefused && CMDLINE="--retry-connrefused"
	curl -L -o "$TMPFILE" -w '\nretval:%{http_code}\n' --retry 3 $CMDLINE --max-time 6 --retry-max-time 15 "${url_lecture_metadata}" &> "$WLOG"
	ERRCODE=$( awk -F: '{ if ($1 == "retval") print $2 }' "$WLOG" )
	RET=$?
	if [ "$RET" = 0 ] && [ -n "$ERRCODE" ] && [ "$ERRCODE" -ge 200 ] && [ "$ERRCODE" -lt 300 ]; then
		# Success, see if data is usable
		if tar -C "$CONFDIR" -x -f "$TMPFILE" && [ -s "$CONFDIR/vmx" ]; then
			return 0
		fi
		rm -rf -- "$TMPFILE"
		writelog "Extracting metadata.tgz failed. Trying legacy mode."
	else
		writelog "Downloading metadata.tgz failed (HTTP $ERRCODE). Trying legacy mode."
		#slxlog --delete "run-virt-wget" "Downloading MetaData failed" "$WLOG"
	fi

	TRIES=0
	while [ "$TRIES" -lt 3 ]; do
		TRIES=$(( TRIES + 1 ))
		writelog "wget try $TRIES"
		WLOG="$TMPDIR/wget-${TRIES}.log"
		if wget -T 6 -O "$TMPCONFIG" "${url_lecture_config}" &>"$WLOG"; then
			rm -f -- "$WLOG"
			writelog "Downloaded VM description from '${url_lecture_config}' successfully."
			if [ -s "$TMPCONFIG" ]; then
				# Downloaded a non-zero VM description file, all good
				return 0
			else
				writelog "Server sent zero byte virtual machine description file."
				EXTRA_ERROR="(Der Server hat eine leere Antwort gesendet)"
			fi
		else
			ERRCODE=$(grep -m1 -o -E '\b[0-9]{3}\b' "$WLOG")
			if [ -n "$ERRCODE" ]; then
				EXTRA_ERROR="(HTTP Error Code $ERRCODE)"
			else
				EXTRA_ERROR="(Der Server ist nicht erreichbar)"
			fi
			writelog "wget failed."
		fi
	done

	slxlog --delete "run-virt-wget" "Downloading VMX/Metadata failed" "$WLOG"

	# Seems we are in legacy mode, which is no longer supported. Warn user and exit
	EXIT_TYPE="user" EXIT_REASON="msg.vm.no-metadata
Die Metadaten der Lehrumgebung konnten nicht vom bwLehrpool-Satelliten
heruntergeladen werden. Bitte versuchen Sie es in wenigen Minuten erneut
und wenden Sie sich an den Support, wenn das Problem bestehen bleibt.
$EXTRA_ERROR
" cleanexit 1
}

## Main ##
call_post_source download_metadata

