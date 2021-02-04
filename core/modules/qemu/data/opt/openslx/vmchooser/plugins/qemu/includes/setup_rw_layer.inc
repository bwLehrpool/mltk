
setup_rw_layer() {
	if isempty VM_DISKFILE_RO; then
		writelog "No source diskfile found! This should be specified in the given XML file. Is it valid?"
		EXIT_TYPE="user" EXIT_REASON="Keine virtuelle Festplatte zu dieser Veranstaltung angegeben!" cleanexit 1
	fi

	# setup qcow2 backing file for that disk file
	writelog "Creating backing file for '${VM_DISKFILE_RO}'..."
	# TODO: WTF? This is fucked up, VM_DISKFILE_RW is set if we want to run in persistent mode,
	# this plugin shouldn't mess around with it....
	declare -rg VM_DISKFILE_RW="${TMPDIR}/$(basename ${VM_DISKFILE_RO}).qcow2"
	if qemu-img create -f qcow2 -b "${VM_DISKFILE_RO}" "${VM_DISKFILE_RW}"; then
		# all good, use it as main disk drive
		# TODO: determine the proper type of controller to use, ideally virtio?
		VIRTCMDOPTS+=("-drive" "if=virtio,format=qcow2,file=${VM_DISKFILE_RW}" )
	else
		writelog "Error creating backing file for '${VM_DISKFILE_RO}'"
		# TODO use -snapshot as fallback, test it!
		# How is this supposed to even work without a disk?
		VIRTCMDOPTS+=( "-snapshot" )
	fi
}

call_post_source setup_rw_layer
