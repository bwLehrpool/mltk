###########################################################
# Include: Setup dnbd for image access, nfs/cifs fallback #
###########################################################
# This will currently make sure that the variable
# VM_DISKFILE_RO is set which will contain the
# absolute path to the disk image to use for the vm
# session.
# When using DNBD3 this will differ from SRC_IMG_ABSOLUTE,
# otherwise it will be identical.
# In the future DNBD3 (or something else) might provide
# a CoW layer so we don't need snapshots etc. anymore.
# This include should set VM_DISKFILE_RW in that case to
# indicate to the virt plugin that it doesn't need to
# handle creating a temporary CoW layer itself.

# Helper to cleanup the image mounted with dnbd3-fuse
cleanup_dnbd3() {
	if ! isset DNBD3_MOUNT_POINT; then
		writelog "'DNBD3_MOUNT_POINT' is not set! Aborting DNBD3 cleanup."
		return 1
	fi
	# try to umount it now
	for timeout in 1 1 1 2 2 3 FAIL; do
		fs_path_ismountpoint "${DNBD3_MOUNT_POINT}" || break
		fusermount -u "${DNBD3_MOUNT_POINT}" && break
		writelog "dnbd3 still busy...."
		[ "$timeout" = "FAIL" ] && break
		sleep "$timeout"
	done
}

setup_dnbd3 () {
	# Mount path for images mounted with dnbd3-fuse
	declare -rg DNBD3_MOUNT_POINT="${TMPDIR}/dnbd3fuse.mnt"
	mkdir -p "${DNBD3_MOUNT_POINT}"
	# start dnbd3-fuse in subshell
	local DNBD3_TMPLOG="${TMPDIR}/dnbd3fuse.log"
	local DNBD3_EXITFLAG="${TMPDIR}/dnbd3exit$RANDOM"
	local TIMEOUT VM_DISKFILE_REVISION
	rm -f -- "${DNBD3_EXITFLAG}"
	(
		startup="$( date +%s )"
		while [ "$( date +%s )" -lt "$(( startup + 4 ))" ]; do
			dnbd3-fuse -f -o allow_other,max_readahead=262144 -h "${SLX_DNBD3_SERVERS}" -i "${SRC_IMG_RELATIVE}" "${DNBD3_MOUNT_POINT}" > "${DNBD3_TMPLOG}" 2>&1
			RET=$?
		done
		touch "${DNBD3_EXITFLAG}"
		if [ "$RET" != "0" ]; then
			writelog "dnbd3-fuse stopped working (Exit code $RET)"
			slxlog "virt-dnbd3-fuse" "dnbd3-fuse stopped/refused serving '${SRC_IMG_RELATIVE}' from '${SLX_DNBD3_SERVERS}' with error code: $RET" "${DNBD3_TMPLOG}"
		fi
	) &
	# give it a bit of time
	usleep 250000
	# check if we have the image
	for TIMEOUT in 0.5 1 1 1 1 OUT; do
		if [ -r "${DNBD3_MOUNT_POINT}/img" ]; then
			VM_DISKFILE_REVISION="$(grep -m 1 "^Revision:" "${DNBD3_MOUNT_POINT}/status" | cut  -d" " -f2)"
			VM_DISKFILE_RO="${DNBD3_MOUNT_POINT}/img"
			writelog "DNBD3: ${SRC_IMG_RELATIVE} on ${VM_DISKFILE_RO} with rid ${VM_DISKFILE_REVISION}"
			add_cleanup "cleanup_dnbd3"
			break
		fi
		[ "$TIMEOUT" = "OUT" ] || [ -e "$DNBD3_EXITFLAG" ] && break
		sleep "$TIMEOUT"
	done

	if isempty VM_DISKFILE_RO; then
		slxlog "virt-dnbd3" "No dnbd3 server for ${SRC_IMG_RELATIVE} found, trying NFS/CIFS..." "$DNBD3_TMPLOG"
		writelog "No working dnbd3 server found :-("
	fi
}

setup_fallback() {
	# Maybe we're reading a dnbd3 directory with RIDs encoded into the filename - use latest one
	unset VM_DISKFILE_REVISION
	local VM_DISKFILE_REVISION=$(ls -1 "${SRC_IMG_ABSOLUTE}.r"* | grep -E -o '\.r[0-9]+$' | grep -o -E '[0-9]+$' | sort -n | tail -n 1)
	if notempty VM_DISKFILE_REVISION; then
		# found
		VM_DISKFILE_RO="${SRC_IMG_ABSOLUTE}.r${VM_DISKFILE_REVISION}"
	elif [ -e "${SRC_IMG_ABSOLUTE}" ]; then
		# try name we got from xml in the first place
		VM_DISKFILE_RO="${SRC_IMG_ABSOLUTE}"
	fi
}

## MAIN PART / Sanity checks ##
setup_image_access() {
	writelog "Setting up virtual hard disk access for virtualizer/emulator ..."
	unset VM_DISKFILE_RO VM_DISKFILE_RW
	declare -g VM_DISKFILE_RO VM_DISKFILE_RW
	run_hooks "image-access"

	if [ -n "$VM_DISKFILE_RW" ]; then
		writelog "A hook in setup_image_access supplied a writable diskfile. Not running default setup."
	elif [ -n "$VM_DISKFILE_RO" ]; then
		writelog "A hook in setup_image_access supplied a read-only diskfile. Not running default setup."
	else
		setup_image_access_default
	fi
	readonly VM_DISKFILE_RO VM_DISKFILE_RW

	if [ -n "$VM_DISKFILE_RW" ]; then
		writelog "\tVM disk file (RW):\t\t$VM_DISKFILE_RW"
		if ! [ -s "$VM_DISKFILE_RW" ]; then
			writelog ".... not found!"
			EXIT_TYPE="user" EXIT_REASON="Schreibbare VM nicht gefunden. Adminmodus fehlgeschlagen." cleanexit 1
		fi
		if ! [ -w "$VM_DISKFILE_RW" ]; then
			writelog ".... not writable!"
			EXIT_TYPE="user" EXIT_REASON="VM-Image auf schreibbarem VM-Speicher nicht schreibbar. Adminmodus fehlgeschlagen." cleanexit 1
		fi
	elif [ -n "$VM_DISKFILE_RO" ]; then
		writelog "\tVM disk file (RO):\t\t$VM_DISKFILE_RO"
		if ! [ -s "$VM_DISKFILE_RO" ]; then
			writelog ".... not found!"
			EXIT_TYPE="user" EXIT_REASON="VM-Image nicht gefunden." cleanexit 1
		fi
		if ! [ -r "$VM_DISKFILE_RO" ]; then
			writelog ".... not readable!"
			EXIT_TYPE="user" EXIT_REASON="VM-Image nicht lesbar." cleanexit 1
		fi
	else
		EXIT_TYPE="user" EXIT_REASON="Kein VM-Image Dateiname angegeben/gefunden." cleanexit 1
	fi
}

setup_image_access_default() {
	# See if we should setup dnbd3 image access at all
	if ! check_dep dnbd3-fuse fusermount; then
		writelog "\tCan't use dnbd3 as dnbd3-fuse/fusermount binaries are not in PATH"
	elif ! isset SRC_IMG_RELATIVE; then
		writelog "\tCan't use dnbd3 as SRC_IMG_RELATIVE is not set"
	elif isempty SLX_DNBD3_SERVERS || [ "x${SLX_VM_DNBD3}" != "xyes" ]; then
		writelog "\tCan't use dnbd3 as no servers are given in config, or SLX_VM_DNBD3 is not set to yes"
	else
		# register setup_dnbd3 as start hook after sourcing this include
		writelog "Trying image access via DNBD3..."
		setup_dnbd3
	fi
	
	# VM_DISKFILE_RO will be empty if dnbd3 is not used or failed to connect.
	if isempty VM_DISKFILE_RO; then
		# try to fallback to access the image via NFS/CIFS filesystem
		writelog "Trying fallback image access via NFS/CIFS..."
		setup_fallback
	fi
}

call_post_source setup_image_access
