#####################################
# Include: Setup printer daemon LPD #
#####################################

QUEUE="STANDARD" # This has to match the queue you configured in your VM
SPOOLDIR=

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
		# TODO: Warn user
	fi
	chmod 0700 "${SPOOLDIR}/${QUEUE}"
fi

# Start the lpdaemon listening on the given port
# TODO: externalize with something like runvirt.d (other parts might benefit from that too)
tcpsvd -E 192.168.101.1 5515 \
	lpd "$SPOOLDIR" \
	ash -c "/opt/openslx/scripts/run-virt_print '${USER}' \"${SPOOLDIR}/${QUEUE}/\$DATAFILE\"" &

# PID to kill the process
PID_LPD="$!"
