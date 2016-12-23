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

writelog "Setting up disk access for virtualizer/emulator ..."

# Try to use dnbd3 to access the image
unset VM_DISKFILE_RO
unset dnbd3_fuse_mount_point

setup_dnbd3 () {
	# Mount path for images mounted with dnbd3-fuse
	dnbd3_fuse_mount_point="$TMPDIR/dnbd3fuse.mnt"
	mkdir -p "${dnbd3_fuse_mount_point}"
	# start dnbd3-fuse in subshell
	local dnbd3_tmplog="$TMPDIR/dnbd3fuse.log"
	local dnbd3_exitflag="$TMPDIR/dnbd3exit$RANDOM"
	local TIMEOUT vm_revision
	rm -f -- "$dnbd3_exitflag"
	(
		dnbd3-fuse -f -o allow_other,max_readahead=262144 -h "$SLX_DNBD3_SERVERS" -i "${SRC_IMG_RELATIVE}" "${dnbd3_fuse_mount_point}" > "$dnbd3_tmplog" 2>&1
		RET=$?
		touch "$dnbd3_exitflag"
		if [ "$RET" != "0" ]; then
			writelog "dnbd3-fuse stopped working (Exit code $RET)"
			slxlog "virt-dnbd3-fuse" "dnbd3-fuse stopped/refused serving '${SRC_IMG_RELATIVE}' from '${SLX_DNBD3_SERVERS}' with error code: $RET" "${dnbd3_tmplog}"
		fi
	) &
	# give it a bit of time
	usleep 250000
	# check if we have the image
	for TIMEOUT in 0.5 1 1 OUT; do
		if [ -r "${dnbd3_fuse_mount_point}/img" ]; then
			vm_revision="$(grep -m 1 "^Revision:" "${dnbd3_fuse_mount_point}/status" | cut  -d" " -f2)"
			VM_DISKFILE_RO="${dnbd3_fuse_mount_point}/img"
			writelog "DNBD3: $SRC_IMG_RELATIVE on $VM_DISKFILE_RO with rid $vm_revision"
			break
		fi
		[ "$TIMEOUT" = "OUT" -o -e "$dnbd3_exitflag" ] && break
		sleep "$TIMEOUT"
	done

	if [ -z "$VM_DISKFILE_RO" ]; then
		slxlog "virt-dnbd3" "No dnbd3 server for ${SRC_IMG_RELATIVE} found, trying NFS/CIFS..." "$dnbd3_tmplog"
		writelog "No working dnbd3 server found :-("
	fi
}

# See if we should setup dnbd3 image access at all
if ! which dnbd3-fuse; then
	writelog "Can't use dnbd3 as dnbd3-fuse binary is not in PATH"
elif [ -z "$SRC_IMG_RELATIVE" ]; then
	writelog "Can't use dnbd3 as SRC_IMG_RELATIVE is not set"
elif [ -z "$SLX_DNBD3_SERVERS" ] || [ "x$SLX_VM_DNBD3" != "xyes" ]; then
	writelog "Can't use dnbd3 as no servers are given in config, or SLX_VM_DNBD3 is not set to yes"
else
	setup_dnbd3
fi

# VM_DISKFILE_RO will be empty if dnbd3 is not used or failed to connect.
# Let's try to fall back to NFS/CIFS via file system
if [ -z "$VM_DISKFILE_RO" ]; then
	# Maybe we're reading a dnbd3 directory with RIDs encoded into the filename - use latest one
	rid_suffix=$(ls -1 "${SRC_IMG_ABSOLUTE}.r"* | grep -E -o '\.r[0-9]+$' | grep -o -E '[0-9]+$' | sort -n | tail -n 1)
	if [ -n "$rid_suffix" ]; then
		# found
		VM_DISKFILE_RO="${SRC_IMG_ABSOLUTE}.r${rid_suffix}"
	elif [ -e "$SRC_IMG_ABSOLUTE" ]; then
		# try name we got from xml in the first place
		VM_DISKFILE_RO="$SRC_IMG_ABSOLUTE"
	fi
fi

# Check if virtual machine container file exists
if [ -z "$VM_DISKFILE_RO" ] || ! [ -e "${VM_DISKFILE_RO}" ]; then
	slxlog "virt-image-missing" "VM image $VM_DISKFILE_RO not found!"
	writelog "Virtual machine image ${VM_DISKFILE_RO} not found!"
	error_user "Das Image für die gewählte Virtuelle Maschine konnte nicht gefunden werden.
Versuchen Sie zunächst, den Computer komplett neu zu starten. Sollte das Problem bestehen bleiben, wenden Sie sich bitte an den Support."
	cleanexit 1
fi

readonly VM_DISKFILE_RO

writelog "Disk file to use: $VM_DISKFILE_RO"

