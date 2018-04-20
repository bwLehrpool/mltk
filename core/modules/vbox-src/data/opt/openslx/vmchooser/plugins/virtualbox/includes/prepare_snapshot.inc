delete_snapshot() {
	rm -f "${VBOX_SNAPSHOT_DIR}/{${SNAPSHOT_UUID}}.vdi" || \
		writelog "Could not cleanup snapshot file: ${VBOX_SNAPSHOT_DIR}/{${SNAPSHOT_UUID}}.vdi"
}
prepare_snapshot() {
	declare -rg SNAPSHOT_UUID="34f617be-192a-46b3-a8ae-bce1029e093f"	# snapshot UUID is static
	
	# get UUID of disk image using od to get 16 bytes (starting at 392) and
	# order them according to the VDI UUID format with awk
	HDD_UUID="$(od -An -t x1 -j 392 -N 16 ${VBOX_HDD_LINK} | awk '{print $4$3$2$1"-"$6$5"-"$8$7"-"$9$10"-"$11$12$13$14$15$16}')"
	if isempty HDD_UUID; then
		writelog "Failed to extract VDI UUID of '${VBOX_HDD_LINK}'"
		EXIT_TYPE="internal" EXIT_REASON="Konnte UUID der virtuellen Festplatte nicht extrahieren." cleanexit 1
	fi

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
		add_cleanup delete_snapshot
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
