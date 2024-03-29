#!/bin/bash
########################################################
# Include: Detect, whether runvirt runs in legacy mode #
########################################################
## Functions ##
# Legacy mode: As runvirt has been before.
# New mode: uuid in xml _and_ vmx given via http.
download_metadata() {
	local EXTRA_ERROR=
	local DOWNLOADED_VMX="$CONFDIR/vmx"
	declare -rg TMPCONFIG="$CONFDIR/vmx_modified"

	# Run hooks
	if run_hooks "download" "$CONFDIR"; then
		writelog "Metadata download handled via hook."
	else
		# No hooks found
		writelog "Downloading metadata via default method."
		download_metadata_default
	fi
	if ! [ -s "$DOWNLOADED_VMX" ]; then
		# Seems we are in legacy mode, which is no longer supported. Warn user and exit
		EXIT_TYPE="user" EXIT_REASON="msg.vm.no-metadata
Die Metadaten der Lehrumgebung konnten nicht vom bwLehrpool-Satelliten
heruntergeladen werden, oder sind leer. Bitte versuchen Sie es in wenigen Minuten erneut
und wenden Sie sich an den Support, wenn das Problem bestehen bleibt.
$EXTRA_ERROR" cleanexit 1
	fi
	if ! cp "$DOWNLOADED_VMX" "$TMPCONFIG"; then
		EXIT_TYPE="user" EXIT_REASON="msg.vm.copy-metadata
Die Metadaten der Lehrumgebung konnten nicht umkopiert werden.
Wenden Sie sich an den Support, wenn das Problem bestehen bleibt.
$EXTRA_ERROR" cleanexit 1
	fi
	return 0
}

download_metadata_default() {
	local TRIES WLOG TMPFILE RET ERRCODE CMDLINE
	# Sanity checks
	if ! check_dep curl; then
		writelog "Could not find 'curl' in PATH: $PATH"
		EXIT_TYPE="internal" EXIT_REASON="Fehlendes Dienstprogramm 'curl'!" cleanexit 1
	fi

	# Try new unified .tar.gz way
	TMPFILE="$TMPDIR/metadata.tgz"
	WLOG="$TMPDIR/curl-metadata.log"
	CMDLINE=
	curl --help | grep -q -- --retry-connrefused && CMDLINE="--retry-connrefused"
	curl -L -o "$TMPFILE" -w '\nretval:%{http_code}\n' --retry 3 $CMDLINE --max-time 8 --retry-max-time 15 "${url_lecture_metadata}" &> "$WLOG"
	ERRCODE=$( awk -F: '{ if ($1 == "retval") print $2 }' "$WLOG" )
	RET=$?
	if [ "$RET" = 0 ] && [ -n "$ERRCODE" ] && [ "$ERRCODE" -ge 200 ] && [ "$ERRCODE" -lt 300 ]; then
		# Success, see if data is usable
		if tar -C "$CONFDIR" -x -f "$TMPFILE" && [ -s "$DOWNLOADED_VMX" ]; then
			rm -rf -- "$TMPFILE"
			return 0
		fi
		writelog "Extracting metadata.tgz failed. Cannot start VM."
	else
		writelog "Downloading metadata.tgz failed (RET=$RET, HTTP=$ERRCODE). Cannot start VM.
$( cat $WLOG )"
		#
	fi
}

## Main ##
call_post_source download_metadata

