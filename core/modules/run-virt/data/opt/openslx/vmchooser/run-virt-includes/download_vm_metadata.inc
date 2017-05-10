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

	local WLOG="$TMPDIR/wget.log"
	local TRIES=0
	while [ "$TRIES" -lt 3 ]; do
		TRIES=$(( TRIES + 1 ))
		writelog "wget try $TRIES"
		if wget -T 6 -O "$TMPCONFIG" "${SLX_VMCHOOSER_BASE_URL}/lecture/${IMGUUID}" &>"$WLOG"; then
			rm -f -- "$WLOG"
			writelog "Downloaded VM description from '${SLX_VMCHOOSER_BASE_URL}/lecture/${IMGUUID}' successfully."
			if [ -s "$TMPCONFIG" ]; then
				# Downloaded a non-zero VM description file, all good
				return 0
			else
				writelog "Server sent zero byte virtual machine description file."
			fi
		else
			writelog "wget failed."
			slxlog --delete "run-virt-wget" "Downloading VMX failed" "$WLOG"
		fi
	done

	# Seems we are in legacy mode, which is no longer supported. Warn user and exit
	EXIT_TYPE="user" EXIT_REASON="msg.vm.no-metadata
Die gewählte VM ist eine 'Legacy VM', für die unvollständige
Metadaten auf dem bwLehrpool-Server hinterlegt sind. Diese
werden nicht mehr unterstützt. Um diese VM weiterhin nutzen
zu können, muss sie mittels der bwLehrpool-Suite heruntergeladen,
einmal gebootet, und wieder hochgeladen werden.
(Bei der Gelegenheit könnten z.B. auch gleich anfallende Updates
eingespielt werden.)
" cleanexit 1
}

## Main ##
call_post_source download_metadata

