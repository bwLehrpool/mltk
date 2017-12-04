######################################################################
# Include: Parsing the non-legacy delivered vmxfile ${TMPCONFIG} #
######################################################################
## Functions
parse_vmx() {
	# To avoid potential syntax problems from the vmx served by dmsd, sanitize
	# it before continuing with filling in the gaps
	local TMPTMPCONFIG="$(mktemp)"
	grep -E '^\s*(#|[^=]+=[^"]*"[^"]*"\s*$|[^=]+=[^"]+$|$)' "$TMPCONFIG" > "$TMPTMPCONFIG"
	if ! diff -q "$TMPCONFIG" "$TMPTMPCONFIG"; then
		# file was corrupted, log it
		writelog "'$TMPCONFIG' had bad syntax and was pruned by the post-download sanitizer!"
		slxlog -s "virt-vmware-vmx-corrupt" "Downloaded .vmx for '$IMGUUID' has a corrupted format!" "$TMPCONFIG"
	fi
	mv -f -- "$TMPTMPCONFIG" "$TMPCONFIG"

	writelog "Parsing virtual machine description file..."
	local -i HW_VERSION=$(grep -i -m1 '^virtualHW.version *= *' "${TMPCONFIG}" | awk -F '=' '{print $2}' | sed -r 's/[^0-9]//g')
	if notempty HW_VERSION; then
       		declare -g VM_HW_VERSION="${HW_VERSION}"
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
