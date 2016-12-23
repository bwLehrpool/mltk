
setup_firewall () {
	local LOGF="${TMPDIR}/firewall.log"
	local RET
	[ "$DISPLAY" = ":0" ] || return 0 # For now, to avoid conflicts, we only do this on display :0
	slxfwtool "$IMGUUID" > "$LOGF" 2>&1
	RET=$?
	if [ "$RET" != "0" ]; then
		slxlog "runvirt-firewall" "Error setting up firewall rules for lecture $IMGUUID (Exit code $RET)" "$LOGF"
	fi
	return 0
}
