#!/bin/bash
############################################
# Feature: Setup firewall rules for the VM #
############################################
## Functions
setup_firewall () {
	local LOGF="${TMPDIR}/firewall.log"
	local RET
	[ "$DISPLAY" = ":0" ] || return 0 # For now, to avoid conflicts, we only do this on display :0
	slxfwtool "$IMGUUID" > "$LOGF" 2>&1
	RET=$?
	if [ "$RET" != "0" ]; then
		slxlog "virt-firewall" "Error setting up firewall rules for lecture $IMGUUID (Exit code $RET)" "$LOGF"
		return 1
	fi
	return 0
}

## MAIN ##
# Sanity checks
if check_dep slxfwtool; then
	reg_feature_handler "firewall" "setup_firewall"
else
	writelog "Could not find 'slxfwtool' in PATH: $PATH. Netrules firewall will not work!"
	error_user "Fehler" "Keine Unterstützung für veranstaltungspezifische Firewall-Regeln!"
fi
