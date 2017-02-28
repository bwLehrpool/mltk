#!/bin/bash
##########################################################################
# Include: Setup virtual machine hypervisor via vm-specific include file #
##########################################################################
## Functions ##
setup_vm_commandline() {
	# Sanity checks
	if ! isset PLUGIN_ID; then
		writelog "Sanity check failed: PLUGIN_ID is not set."
		EXIT_TYPE="internal" EXIT_REASON="Konnte kein Virtualisierer für das gewählte Image ermitteln!" cleanexit 1
	fi
	if ! $(safesource "$VMCHOOSER_DIR/plugins/$PLUGIN_ID/run-virt.include"); then
		# safesource logs errors on its own
		EXIT_TYPE="internal" EXIT_REASON="Konnte Virtualisierungsplugin '$PLUGIN_ID' nicht initialisieren!" cleanexit 1
	fi
}

## Main ##
call_post_source setup_vm_commandline
