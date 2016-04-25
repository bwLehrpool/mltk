########################################################
# Include: Detect, whether runvirt runs in legacy mode #
########################################################

# Legacy mode: As runvirt has been before.
# New mode: uuid in xml _and_ vmx given via http.

writelog "Detecting current/legacy mode ..."

# First, let's try to extract an imguuid from xmlfile:
IMGUUID=$(grep -io '<uuid param=.*"' "${xmlfile}" | sed -e "s/&.*;/; /g" | awk -F '"' '{ print $2 }')

writelog "IMGUUID extracted: $IMGUUID"

if [ ! -n "$IMGUUID" ]; then	# Keine uuid: Abbruch, Legacy
	writelog "Could not extract an uuid param from ${xmlfile}. Triggering legacy mode."
	LEGACY=:	# No uuid; legacy.
else
	# Now getting template file:
	if wget -O "$TMPDIR/$IMGUUID" "${SLX_VMCHOOSER_BASE_URL}/lecture/${IMGUUID}" 1>&2 2>/dev/null; then
		writelog "wget ${SLX_VMCHOOSER_BASE_URL}/lecture/${IMGUUID} successful."
		writelog "Triggering current (non-legacy) mode."
		LEGACY=		# both strings set: no legacy.
		if [ ! -s "$TMPDIR/$IMGUUID" ]; then
			LEGACY=		# no legacy: server sent 0 byte file.
			writelog "Server sent zero byte virtual machine description file. Retriggering legacy mode."
		fi
	else
		writelog "wget ${SLX_VMCHOOSER_BASE_URL}/lecture/${IMGUUID}. Triggering legacy mode."
		LEGACY=:
	fi
fi

