########################################################
# Include: Detect, whether runvirt runs in legacy mode #
########################################################

# Legacy mode: As runvirt has been before.
# New mode: uuid in xml _and_ vmx given via http.

writelog "Detecting current/legacy mode ..."

declare -rg TMPCONFIG="$TMPDIR/vmconfig.tmp"

# Assume legacy mode by default, only trigger "current" mode if everything else below worked
LEGACY=yes
if [ -z "$IMGUUID" ]; then	# Keine uuid: Abbruch, Legacy
	writelog "Could not extract a uuid param from ${xmlfile}. Triggering legacy mode."
else
	# Now getting template file:
	if ! wget -T 6 -O "$TMPCONFIG" "${SLX_VMCHOOSER_BASE_URL}/lecture/${IMGUUID}" 2>/dev/null >&2; then
		writelog "wget ${SLX_VMCHOOSER_BASE_URL}/lecture/${IMGUUID}. Triggering legacy mode."
	else
		writelog "wget ${SLX_VMCHOOSER_BASE_URL}/lecture/${IMGUUID} successful."
		if [ ! -s "$TMPCONFIG" ]; then
			writelog "Server sent zero byte virtual machine description file. Triggering legacy mode."
		else
			writelog "Triggering current (non-legacy) mode."
			LEGACY=		# everything worked - clear legacy mode variable (so we use the "current" mode)
		fi
	fi
fi

readonly LEGACY

