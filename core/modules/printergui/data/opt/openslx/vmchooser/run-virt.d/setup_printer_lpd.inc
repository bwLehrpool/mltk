#####################################
# Include: Setup printer daemon LPD #
#####################################
## Functions ##
setup_virtual_printer() {
	# TODO: QUEUE should prolly not be hardcoded ....
	local QUEUE="STANDARD" # This has to match the queue you configured in your VM
	local SPOOLDIR
	
	### Disabled: 100megs is not enough, some jobs are HUGE, try to use temp which should be on disk
	## Try using user's tmpfs home first, as it gets wiped on logout
	#if [ -n "${HOME}" ] && [ -w "${HOME}" ]; then
	#       SPOOLDIR="${HOME}/.spool"
	#       mkdir -p "${SPOOLDIR}/${QUEUE}"
	#fi
	# If failed, try to fall back to /tmp
	
	if [ -z "${SPOOLDIR}" ] || [ ! -w "${SPOOLDIR}/${QUEUE}" ]; then
		SPOOLDIR="${TMPDIR}/printergui-${RANDOM}"
		rm -rf -- "${SPOOLDIR}"
		if ! mkdir -p "${SPOOLDIR}/${QUEUE}"; then
			slxlog "virt-spooldir" "Could not create spool directory ($SPOOLDIR) for $USER - printing will not work!"
			notify_user "Drucksystem" "Das Drucksystem konnte nicht initialisiert werden. Druckfunktion nicht verfügbar."
		fi
		chmod 0700 "${SPOOLDIR}/${QUEUE}"
	fi
	
	# Start the lpdaemon listening on the given port
	# TODO: externalize with something like runvirt.d (other parts might benefit from that too)
	tcpsvd -E 192.168.101.1 5515 \
		lpd "$SPOOLDIR" \
		ash -c "/opt/openslx/scripts/run-virt_print '${USER}' \"${SPOOLDIR}/${QUEUE}/\$DATAFILE\"" &
	
	# PID to kill the process
	declare -rg PID_LPD="$!"
	add_cleanup "stop_virtual_printer"
	{
		sleep 2
		# Check if tcpsvd is running. Do this a little delayed so we do not check
		# immediately after trying to spawn it, as this could result in
		# success even if it's not really working.
	
		if ! kill -0 "$PID_LPD"; then
			slxlog "virt-lpd" "Could not start tcpsvd/lpd for virtual machine session"
			notify_user "Drucksystem" "Das Drucksystem konnte nicht initialisiert werden. Druckfunktion nicht verfügbar."
		fi
	} &
}

# cleanup helper to stop lpd
stop_virtual_printer() {
	isset PID_LPD && kill "${PID_LPD}"
}

## MAIN ##
# Sanity checks
unset FAIL
if ! check_dep tcpsvd lpd; then
	writelog "One of 'tcpsvd' or 'lpd' could not be found in PATH: $PATH"
	FAIL=y
fi
if [ ! -f "/opt/openslx/scripts/run-virt_print" ]; then
	writelog "File not found: /opt/openslx/scripts/run-virt_print"
	FAIL=y
fi
if isset FAIL; then
	unset FAIL
	notify_user "Drucksystem" "Das Drucksystem konnte nicht initialisiert werden. Druckfunktion nicht verfügbar."
else
	reg_feature_handler "printer" "setup_virtual_printer"
fi

