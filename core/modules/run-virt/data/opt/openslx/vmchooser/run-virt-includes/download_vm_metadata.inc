#!/bin/bash
########################################################
# Include: Detect, whether runvirt runs in legacy mode #
########################################################
## Functions ##
# Legacy mode: As runvirt has been before.
# New mode: uuid in xml _and_ vmx given via http.
download_metadata() {
	# Sanity checks
	if ! check_dep wget; then
		writelog "Could not find 'wget' in PATH: $PATH"
		EXIT_TYPE="internal" EXIT_REASON="Fehlendes Dienstprogramm 'wget'!" cleanexit 1
	fi

	if isempty SLX_VMCHOOSER_BASE_URL; then
		writelog "SLX_VMCHOOSER_BASE_URL is not set! Was /opt/openslx/config sourced?"
		EXIT_TYPE="internal" EXIT_REASON="msg.vmchooser.url-missing Keine URL zur Schnittstelle des bwLehrpool-Satelliten gefunden!" cleanexit 1
	fi

	writelog "Detecting current/legacy mode..."
	declare -rg TMPCONFIG="$TMPDIR/vmconfig.tmp"

	local TRIES=0
	local EXTRA_ERROR=
	local WLOG=
	while [ "$TRIES" -lt 3 ]; do
		TRIES=$(( TRIES + 1 ))
		writelog "wget try $TRIES"
		WLOG="$TMPDIR/wget-${TRIES}.log"
		if wget -T 6 -O "$TMPCONFIG" "${SLX_VMCHOOSER_BASE_URL}/lecture/${IMGUUID}" &>"$WLOG"; then
			rm -f -- "$WLOG"
			writelog "Downloaded VM description from '${SLX_VMCHOOSER_BASE_URL}/lecture/${IMGUUID}' successfully."
			if [ -s "$TMPCONFIG" ]; then
				# Downloaded a non-zero VM description file, all good
				return 0
			else
				writelog "Server sent zero byte virtual machine description file."
				EXTRA_ERROR="(Der Server hat eine leere Antwort gesendet)"
			fi
		else
			local ERRCODE=$(grep -m1 -o -E '\b[0-9]{3}\b' "$WLOG")
			if [ -n "$ERRCODE" ]; then
				EXTRA_ERROR="(HTTP Error Code $ERRCODE)"
			else
				EXTRA_ERROR="(Der Server ist nicht erreichbar)"
			fi
			writelog "wget failed."
			slxlog --delete "run-virt-wget" "Downloading VMX failed" "$WLOG"
		fi
	done

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

