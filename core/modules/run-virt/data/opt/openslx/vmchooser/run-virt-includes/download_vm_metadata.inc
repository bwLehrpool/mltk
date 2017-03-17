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
		EXIT_TYPE="internal" EXIT_REASON="Keine URL zur Schnittstelle des bwLehrpool-Satelliten gefunden!" cleanexit 1
	fi

	writelog "Detecting current/legacy mode..."	
	declare -rg TMPCONFIG="$TMPDIR/vmconfig.tmp"

	if wget -T 6 -O "$TMPCONFIG" "${SLX_VMCHOOSER_BASE_URL}/lecture/${IMGUUID}" 2>/dev/null >&2; then
		writelog "Downloaded VM description from '${SLX_VMCHOOSER_BASE_URL}/lecture/${IMGUUID}' successfully."
		if [ -s "$TMPCONFIG" ]; then
			# Downloaded a non-zero VM description file, all good
			return 0
		else
			writelog "Server sent zero byte virtual machine description file. Triggering legacy mode."
		fi
	fi

	# Seems we are in legacy mode, which is no longer supported. Warn user and exit
	EXIT_TYPE="user" EXIT_REASON="
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

