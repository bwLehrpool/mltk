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
	if [ -s "$CONFDIR/config.inc" ]; then
		$( safesource "$CONFDIR/config.inc" )
		notempty DMSD_IMAGE_PATH && SRC_IMG_RELATIVE="$DMSD_IMAGE_PATH"
	fi

	# Validate metadata from XML, now that it has potentially been augmented
	readonly SRC_IMG_RELATIVE

	if isempty SRC_IMG_ABSOLUTE SRC_IMG_RELATIVE; then
		writelog "Error parsing XML: Neither relative nor absolute path for image found!"
		EXIT_TYPE="user" EXIT_REASON="Konnte keinen Pfad zur ausgewählten virtuellen Maschine ermitteln!" cleanexit 1
	fi

	if isempty SRC_IMG_ABSOLUTE && notempty VMSTORE_PATH; then
		SRC_IMG_ABSOLUTE="${VMSTORE_PATH}/${SRC_IMG_RELATIVE}"
	fi
	readonly SRC_IMG_ABSOLUTE

	declare -rg IMG_BASENAME=$(basename "$SRC_IMG_ABSOLUTE")

	notempty VM_DISPLAYNAME || VM_DISPLAYNAME="${IMG_BASENAME}"
	readonly VM_DISPLAYNAME

	# Define VM_CLEANNAME since VM_DISPLAYNAME can be long and contain weird characters
	declare -rg VM_CLEANNAME=$(echo "${VM_DISPLAYNAME:0:32}" | sed -r 's/[^0-9a-zA-Z_\-\.]+/_/g')

	#
	return 0
}

download_metadata_default() {
	local TRIES WLOG TMPFILE RET ERRCODE CMDLINE
	local argx dmsdFile
	# Sanity checks
	if ! check_dep curl; then
		writelog "Could not find 'curl' in PATH: $PATH"
		EXIT_TYPE="internal" EXIT_REASON="Fehlendes Dienstprogramm 'curl'!" cleanexit 1
	fi

	# Admin-mode (Edit-Mode)
	argx=
	dmsdFile="/run/user/$( id -u )/dmsd-session"
	if is_on "$VMCHOOSER_ADMIN_MODE"; then
		local DMSD_SESSION_ID=$( < "$dmsdFile"  jq -r .authToken )
		if isempty DMSD_SESSION_ID; then
			# Legacy format?
			DMSD_SESSION_ID=$( cat "$dmsdFile" )
		else
			# New format - remember to pass to cowgui
			DMSD_SESSION_JSON="$dmsdFile"
		fi
		if isempty DMSD_SESSION_ID; then
			writelog "No cow token found in edit mode"
			EXIT_TYPE=user EXIT_REASON="Admin-Modus aktiviert, aber keine dmsd session vorgefunden." cleanexit 1
		fi
		argx="?cow-user=${DMSD_SESSION_ID}&cow-type=${VMCHOOSER_ADMIN_TYPE}"
	fi
	#rm -f -- "$dmsdFile"

	# Try new unified .tar.gz way
	TMPFILE="$TMPDIR/metadata.tgz"
	WLOG="$TMPDIR/curl-metadata.log"
	CMDLINE=
	curl --help | grep -q -- --retry-connrefused && CMDLINE="--retry-connrefused"
	curl -L -o "$TMPFILE" -w '\nretval:%{http_code}\n' --retry 3 $CMDLINE --max-time 8 --retry-max-time 15 \
			"${url_lecture_metadata}${argx}" &> "$WLOG"
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
		if [ -s "$TMPFILE" ] && grep -q --binary-files=without-match -E '[a-z]'; then
			EXTRA_ERROR="$( cat "$TMPFILE" )"
		fi
	fi
}

DMSD_SESSION_JSON=

## Main ##
call_post_source download_metadata

