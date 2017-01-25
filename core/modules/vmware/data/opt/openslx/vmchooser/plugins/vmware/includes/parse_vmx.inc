######################################################################
# Include: Parsing the non-legacy delivered vmxfile ${TMPCONFIG} #
######################################################################
## Functions
parse_vmx() {
	writelog "Parsing virtual machine description file..."
	local -i HW_VERSION=$(grep -i -m1 '^virtualHW.version *= *' "${TMPCONFIG}" | awk -F '=' '{print $2}' | sed -r 's/[^0-9]//g')
	if notempty HW_VERSION; then
       		declare -rg VM_HW_VERSION="${HW_VERSION}"
		return 0
	fi
	return 1
}

## MAIN ##
if notempty TMPCONFIG; then
	call_post_source parse_vmx
else
	writelog "Path to VMX file is not set or empty! Aborting..."
	EXIT_TYPE="internal" EXIT_REASON="Konnte Pfad zur VMX-Datei nicht finden!" cleanexit 1
fi

