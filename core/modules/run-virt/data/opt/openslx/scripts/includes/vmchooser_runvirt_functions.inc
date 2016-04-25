#######################################################
# Include: Set functions needed by vmchooser-run_virt #
#######################################################

# function to write to stdout and logfile
writelog() {
	local DATE=$(date +%Y-%m-%d-%H-%M-%S)
	# write to stdout?
	if [ "x$1" = "x--quiet" ]; then
		shift
	else
		echo -e "$DATE: $@"
	fi
	# log into file
	echo -e "$DATE: $@" >> "${LOGFILE}"
}

# Clean exit will be called at the end of vmchooser-run_virt
cleanexit() {
	sleep 1
	# Ummount dnbd3-fuse
	if [ -n "$dnbd3_fuse_mount_point" ] && [ -e "$dnbd3_fuse_mount_point/img" ]; then
		for timeout in 1 1 1 FAIL; do
			fusermount -u "$dnbd3_fuse_mount_point" && break
			writelog "dnbd3 still busy...."
			[ "$timeout" = "FAIL" ] && break
			sleep "$timeout"
		done
	fi
	# Kill LPD
	[ -n "${PID_LPD}" ] && kill "${PID_LPD}"

	# If we're not in debug mode, remove all temporary files
	if [ -n "${TMPDIR}" -a -z "$SLX_DEBUG" ]; then
		rm -rf -- "${TMPDIR}"
	fi

	[ $# -gt 0 ] && exit "$1"
	exit 129 # No exit code was given :/
}

rv_clean_string() {
	if [ "$#" -ge 1 ]; then
		echo "$@" | tr '[A-Z]' '[a-z]' | tr -d -c '[a-z0-9\-]'
	else
		tr '[A-Z]' '[a-z]' | tr -d -c '[a-z0-9\-]'
	fi
}
