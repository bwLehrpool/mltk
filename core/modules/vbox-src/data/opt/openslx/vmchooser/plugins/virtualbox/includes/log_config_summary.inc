################################################################################
# Include: log a summary of the configuration state for this machine           #
################################################################################
log_config_summary() {
	writelog "\tSnapshots dir:\t\t$VBOX_SNAPSHOT_DIR"
	writelog "Diskimage:"
	writelog "\tDisk file:\t\t$VBOX_HDD_LINK"
	writelog "\tDisk type:\t\tVDI"
	writelog "\tGuest OS:\t\t$VM_OS_TYPE"
	writelog "\tMachine UUID:\t\t$MACHINE_UUID"
	writelog "\tDisk UUID:\t\t$VBOX_HDD_UUID"
	writelog "Virtual Hardware:"
	writelog "\tCPU cores:\t\t${CPU_CORES}\c"
	writelog ""
	[ "$HW_KVM" = "ENABLED" ] && writelog "\t(VT enabled CPU)"
	writelog "\tGuest RAM:\t\t${VM_MEM} MB"
	# echo nur wenn HOST_MEM_REMAINING gesetzt
	[ -n "${HOST_MEM_REMAINING}" ] && writelog "\tHost RAM:\t\t${HOST_MEM_REMAINING} MB"
	writelog "\tMAC address:\t\t$VM_MAC_ADDR"
	writelog "\tNetwork card:\t\t${vb_network_card}"
	writelog "\tNetwork kind:\t\t${network_kind}"
	writelog "\tBooting from:\t\t${boot}\c"
	writelog ""
	writelog "\tCD-ROM1:\t\t${CDROM0}"
	writelog "\tFloppy_A:\t\t${floppy0}"
	writelog "\tFloppy_B:\t\t${floppy1}"
	writelog "\tShared Folders '${sharename}':\t${sharepath}"
}

call_post_source log_config_summary
