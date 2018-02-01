prepare_snapshot() {
	SNAPSHOT_UUID="34f617be-192a-46b3-a8ae-bce1029e093f"	# snapshot UUID is static
	
	# get UUID of VBox image
	HDD_UUID=$(${VBOX_BASE_DIR}/VBoxManage -q showvdiinfo ${VBOX_HDD_LINK} \
					| grep UUID | grep -v "Parent UUID" | awk '{print $2}' \
					| grep -v use )	# nec. for VBox >= 4

	# TODO support vmdk disks?
	VBOX_HDD_FORMAT="VDI"

	if isset VM_DISKFILE_RO; then
		# use temp disk as snapshot to get CoW
		VBOX_HDD_TYPE="Immutable"					# make disk immutable
		VBOX_HDD_UUID=${SNAPSHOT_UUID}
		< "${VBOX_PLUGIN_DIR}/empty-diff.vdi.gz" gunzip > "${VBOX_SNAPSHOT_DIR}/{${SNAPSHOT_UUID}}.vdi"
		# patch the disk file uuid into the snapshot vdi file:
		# We read from offset 424 in the source HDD and write the 16 bytes long UUID
		# starting at offset 392 in the snapshot.
		# TODO: This requires the image to be a vdi/vdh; Afaik, vbox supports vmdk,
		# so this code would break in that case. Can we use the diskuuid we got via
		# VBoxManage, convert to binary, and patch it
		dd if=${VBOX_HDD_LINK} of="${VBOX_SNAPSHOT_DIR}/{${SNAPSHOT_UUID}}.vdi" seek=424 \
			skip=392 bs=1 count=16 conv=notrunc || \
				writelog "Could not patch snapshot file"
	elif isset VM_DISKFILE_RW; then
		# Image is already RW - easy
		VBOX_HDD_TYPE="Normal"
		VBOX_HDD_UUID=${HDD_UUID}
	else
		writelog "Neither VM_DISKFILE_RO nor VM_DISKFILE_RW defined!"
		cleanexit 1
	fi
}

call_post_source prepare_snapshot
