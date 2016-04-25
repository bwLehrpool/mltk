############################################
# Include: Checking whether lpd is running #
############################################

# Check if tcpsvd is running. Do this down here so we do not check
# immediately after trying to spawn it, as this could result in
# success even if it's not really working.

if ! kill -0 "$PID_LPD"; then
	slxlog "virt-lpd" "Could not start tcpsvd/lpd for virtual machine session"
fi
