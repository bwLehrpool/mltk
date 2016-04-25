###########################################################
# Include: Setup dnbd for image access, nfs/cifs fallback #
###########################################################

writelog "Now in setup_image_access.inc ..."

# Try to use dnbd3 to access the image
unset vm_diskfile
unset vm_revision
unset dnbd3_fuse_mount_point
unset dnbd3_tmplog
if ! which dnbd3-fuse; then
	writelog "Can't use dnbd3 as dnbd3-fuse binary is not in PATH"
elif [ -z "$SLX_DNBD3_SERVERS" ] || [ "x$SLX_VM_DNBD3" != "xyes" ]; then
	writelog "Can't use dnbd3 as no servers are given in config, or SLX_VM_DNBD3 is not set to yes"
else
	# Mount path for images mounted with dnbd3-fuse
	dnbd3_fuse_mount_point="$TMPDIR/dnbd3fuse.mnt"
	mkdir -p "${dnbd3_fuse_mount_point}"
	# start dnbd3-fuse in subshell
	dnbd3_tmplog="$TMPDIR/dnbd3fuse.log"
	dnbd3_exitflag="$TMPDIR/dnbd3exit$RANDOM"
	rm -f -- "$dnbd3_exitflag"
	(
		dnbd3-fuse -f -o allow_other,max_readahead=262144 -h "$SLX_DNBD3_SERVERS" -i "${imgrelpath}" "${dnbd3_fuse_mount_point}" > "$dnbd3_tmplog" 2>&1
		RET=$?
		touch "$dnbd3_exitflag"
		if [ "$RET" != "0" ]; then
			writelog "dnbd3-fuse stopped working (Exit code $RET)"
			slxlog "virt-dnbd3-fuse" "dnbd3-fuse stopped/refused serving '${imgrelpath}' from '${SLX_DNBD3_SERVERS}' with error code: $RET" "${dnbd3_tmplog}"
		fi
		sleep 2
		rm -f -- "$dnbd3_tmplog" "$dnbd3_exitflag"
	) &
	# give it a bit of time
	usleep 250000
	# check if we have the image
	for TIMEOUT in 0.5 1 1 OUT; do
		if [ -r "${dnbd3_fuse_mount_point}/img" ]; then
			vm_revision="$(grep -m 1 "^Revision:" "${dnbd3_fuse_mount_point}/status" | cut  -d" " -f2)"
			vm_diskfile="${dnbd3_fuse_mount_point}/img"
			writelog "DNBD3: $imgrelpath on $vm_diskfile with rid $vm_revision"
			vmpath="$vm_diskfile"
			break
		fi
		[ "$TIMEOUT" = "OUT" -o -e "$dnbd3_exitflag" ] && break
		sleep "$TIMEOUT"
	done

	if [ -z "$vm_diskfile" ]; then
		slxlog "virt-dnbd3" "No dnbd3 server for ${imgrelpath} found, trying NFS/CIFS..." "$dnbd3_tmplog"
		writelog "No working dnbd3 server found :-("
	fi
fi

# $vm_diskfile will be empty if dnbd3 is not used or failed. Let's try to fall back to NFS/CIFS via file system
if [ -z "$vm_diskfile" ]; then
	new_vmpath=$(ls "${vmpath}.r"* | grep -E -o '\.r[0-9]+$' | grep -o -E '[0-9]+' | sort -n | tail -n 1)
	[ -n "$new_vmpath" ] && vmpath="${vmpath}.r${new_vmpath}"
	vm_diskfile="$vmpath"
fi

# Check if virtual machine container file exists
if ! [ -e "${vmpath}" ]; then
	slxlog "virt-image-missing" "VM image $vmpath not found!"
	writelog "Virtual machine image ${vmpath} not found!"
	cleanexit 1
fi

writelog "Virtual machine disk file: $vm_diskfile"

