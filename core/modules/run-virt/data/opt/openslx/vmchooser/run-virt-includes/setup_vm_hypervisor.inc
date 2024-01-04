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
	local PLUGIN_DISPLAY_NAME="$(get_xml virtualizer_name)"
	writelog "Plugin Name: $PLUGIN_DISPLAY_NAME"
	isempty PLUGIN_DISPLAY_NAME && PLUGIN_DISPLAY_NAME="${PLUGIN_ID}"
	if [ ! -d "${VMCHOOSER_DIR}/plugins/${PLUGIN_ID}" ]; then
		EXIT_TYPE="internal" EXIT_REASON="msg.vm.missing-hypervisor
Fehlende Unterstützung für '${PLUGIN_DISPLAY_NAME}' im Grundsystem.
Stellen Sie sicher, dass ihre bwLehrpool-Administration das run-virt plugin '${PLUGIN_ID}' zur Verfügung stellt.
" cleanexit 1
	fi
	if ! $(safesource "$VMCHOOSER_DIR/plugins/$PLUGIN_ID/run-virt.include"); then
		# safesource logs errors on its own
		EXIT_TYPE="internal" EXIT_REASON="Konnte Virtualisierungsplugin '$PLUGIN_ID' nicht initialisieren!" cleanexit 1
	fi
}

## Main ##
call_post_source setup_vm_commandline
