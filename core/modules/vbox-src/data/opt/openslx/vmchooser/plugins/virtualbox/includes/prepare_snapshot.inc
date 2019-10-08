# Extracts the uuid of the given vdi.
# For VBox VDI Version 1.1, the 16-byte uuid is at offset 392
# See: https://forums.virtualbox.org/viewtopic.php?p=29267#p29267
extract_vdi_uuid() {
	[ -z "$1" ] && return
	od -An -t x1 -j 392 -N 16 "$1" | \
		awk '{print $4$3$2$1"-"$6$5"-"$8$7"-"$9$10"-"$11$12$13$14$15$16}'
}

# Cleanup hook when the session stops
delete_snapshot() {
	rm -f "${VBOX_SNAPSHOT_DIR}/{${SNAPSHOT_UUID}}.vdi" || \
		writelog "Could not cleanup snapshot file: ${VBOX_SNAPSHOT_DIR}/{${SNAPSHOT_UUID}}.vdi"
}
# TODO support snapshot for RW mode that are uploaded/on an NFS.
prepare_disk_image_rw() {
	# more of a placeholder function at this point :)
	declare -rg VBOX_HDD_UUID="$HDD_UUID"
}

prepare_disk_image_ro() {
	# extract the uuid of the snapshot...
	local snapshot_path="${VBOX_PLUGIN_DIR}/snapshot.vdi"
	local snapshot_tmp="${VBOX_SNAPSHOT_DIR}/${snapshot_path##*/}"
	if [ -f "${snapshot_path}.gz" ]; then
		gunzip -c "${snapshot_path}.gz" > "$snapshot_tmp"
	elif [ -f "$snapshot_path" ]; then
		cp -f -- "$snapshot_path" "$snapshot_tmp"
	else
		writelog "Could not find pre-generated snapshot file '$snapshot_path(|.gz)'."
		# TODO fallback on creating a snapshot for this vbox image
		# Note that this needs to be done after VBoxManage is aware of this VM...
	fi
	if [ -f "$snapshot_tmp" ]; then
		declare -rg SNAPSHOT_UUID="$(extract_vdi_uuid "$snapshot_tmp")"
	fi

	# use temp disk as snapshot to get CoW
	declare -rg VBOX_HDD_UUID="${SNAPSHOT_UUID}"

	cp -f -- "$snapshot_tmp" "${VBOX_SNAPSHOT_DIR}/{${SNAPSHOT_UUID}}.vdi"

	# TODO Use VBoxManage to determine the UUID of the disk image (making sure
	# it is a VDI image) and patch it into the prepared snapshot VDI.
	dd of="${VBOX_SNAPSHOT_DIR}/{${SNAPSHOT_UUID}}.vdi" \
		seek=424 bs=1 count=16 conv=notrunc <<< "$HDD_UUID" || \
			writelog "Could not patch snapshot file"

	# TODO handle other formats...
	# finally "register" the snapshot in the MediaRegistry
	xmlstarlet ed -L -N x="http://www.virtualbox.org/" \
		-s "//x:VirtualBox/x:Global/x:MediaRegistry/x:HardDisks/x:HardDisk[@uuid='{${HDD_UUID}}']" \
		-t elem -n "HardDisk" --var newnode '$prev' \
		-i '$newnode' -t attr -n "uuid" -v "{${SNAPSHOT_UUID}}" \
		-i '$newnode' -t attr -n "location" -v "${VBOX_SNAPSHOT_DIR}/{${SNAPSHOT_UUID}}.vdi" \
		-i '$newnode' -t attr -n "format" -v "VDI" \
		-i '$newnode' -t attr -n "autoReset" -v "true" \
		"${VBOX_ROOT}/VirtualBox.xml"

	add_cleanup delete_snapshot
}

# Prepares the disk image specified as VM_DISKFILE_R[OW]
# by run-virt's generic setup-image-access.inc
# * RO -> use a CoW snapshot using VBox internals
# * RW -> use it as is
prepare_disk_image() {

	# NOTE: either one is garanteed to be set by setup_image_access.inc
	local handler
	if [ -n "$VM_DISKFILE_RO" ]; then
		declare -rg VBOX_HDD_LINK="$VM_DISKFILE_RO"
		declare -rg VBOX_HDD_TYPE="Immutable"
		handler="prepare_disk_image_ro"
	elif [ -n "$VM_DISKFILE_RW" ]; then
		declare -rg VBOX_HDD_LINK="$VM_DISKFILE_RW"
		declare -rg VBOX_HDD_TYPE="Normal"
		handler="prepare_disk_image_rw"
	fi

	# TODO Determine HDD storage format using VBoxManage
	declare -rg VBOX_HDD_FORMAT="VDI"
	declare -rg HDD_UUID="$(extract_vdi_uuid ${VBOX_HDD_LINK})"
	if isempty HDD_UUID; then
		writelog "Failed to extract VDI UUID of '${VBOX_HDD_LINK}'"
		EXIT_TYPE="internal" EXIT_REASON="Konnte UUID der virtuellen Festplatte nicht extrahieren." cleanexit 1
	fi

	# add the hard disk element to the "global" VirtualBox.xml
	xmlstarlet ed -L -N x="http://www.virtualbox.org/" \
		-s "//x:VirtualBox/x:Global/x:MediaRegistry/x:HardDisks" \
		-t elem -n "HardDisk" --var newnode '$prev' \
		-i '$newnode' -t attr -n "uuid" -v "{${HDD_UUID}}" \
		-i '$newnode' -t attr -n "location" -v "$VBOX_HDD_LINK" \
		-i '$newnode' -t attr -n "format" -v "$VBOX_HDD_FORMAT" \
		-i '$newnode' -t attr -n "type" -v "$VBOX_HDD_TYPE" \
		"${VBOX_ROOT}/VirtualBox.xml"

	# now handle either RW or RO images
	isset handler && "$handler"
	return 0
}

call_post_source prepare_disk_image
