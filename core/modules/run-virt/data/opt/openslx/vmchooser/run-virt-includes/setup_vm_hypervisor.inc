##########################################################################
# Include: Setup virtual machine hypervisor via vm-specific include file #
##########################################################################

# Get all virtual machine specific stuff from the respective include file
if [ ! -e "$VMCHOOSER_DIR/$PLUGIN_ID/run-virt.include" ] ; then
	slxlog "virt-plugin-missing" "Could not find run-virt.include for $PLUGIN_ID ($VMCHOOSER_DIR/$PLUGIN_ID/run-virt.include)"
	writelog "Failed because of missing ${PLUGIN_ID} plugin."
	error_user "Konnte den Virtualisierer '$PLUGIN_ID' nicht finden. Starten der Virtuellen Maschine fehlgeschlagen.
	Starten Sie den Computer neu und wenden Sie sich an den Support, wenn das Problem weiterhin besteht."
	cleanexit 1
fi

self="${PLUGIN_ID}"

if ! bash -n "$VMCHOOSER_DIR/$PLUGIN_ID/run-virt.include"; then
	slxlog "virt-plugin-syntax" "run-virt.include for $PLUGIN_ID contains syntax errors (bash -n run-virt.include failed)"
	writelog "Erroneous run-virt.include for $PLUGIN_ID (syntax check)"
	error_user "Das Start-Script für den Virtualisierer '$PLUGIN_ID' ist fehlerhaft.
	Starten Sie den Computer neu und wenden Sie sich an den Support, wenn das Problem weiterhin besteht."
	cleanexit 1
fi

setup_vm_commandline () {
	# Now including the hypervisor specific include file:
	if ! source "$VMCHOOSER_DIR/$PLUGIN_ID/run-virt.include"; then
		slxlog "virt-plugin-error" "run-virt.include for $PLUGIN_ID could not be sourced properly."
		writelog "Erroneous run-virt.include for ${PLUGIN_ID}? Source returned != 0"
		error_user "Das Start-Script für den Virtualisierer '$PLUGIN_ID' hat einen fehlercode zurückgegeben.
		Starten Sie den Computer neu, falls es beim Ausführen der VM zu Problemen kommt
		und wenden Sie sich an den Support, wenn das Problem weiterhin besteht."
	fi
}

