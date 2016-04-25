##########################################################################
# Include: Setup virtual machine hypervisor via vm-specific include file #
##########################################################################

# Get all virtual machine specific stuff from the respective include file
if [ ! -e "$VMCHOOSER_DIR/$xmlvirt/run-virt.include" ] ; then
	# TODO: Tell user
	slxlog "virt-plugin-missing" "Could not find run-virt.include for $xmlvirt ($VMCHOOSER_DIR/$xmlvirt/run-virt.include)"
	writelog "Failed because of missing ${xmlvirt} plugin."
	cleanexit 1
fi

	self="${xmlvirt}"

# Now including the hypervisor specific include file:
if ! source "$VMCHOOSER_DIR/$xmlvirt/run-virt.include"; then
	slxlog "virt-plugin-error" "run-virt.include for $xmlvirt could not be sourced (contains syntax errors?)" "$xmlvirt"
	writelog "Erroneous run-virt.include for $xmlvirt"
fi
