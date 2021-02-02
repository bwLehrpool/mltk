##################################################
# Include: Print vm config summary into log file #
##################################################

log_config_summary() {
	writelog "Directories:"
	writelog "\tTMPDIR:\t\t\t$TMPDIR"
	writelog "Disk image:"
	writelog "\tSource disk:\t\t$VM_DISKFILE_RO"
	if [ -e "${VM_DISKFILE_RW}" ]; then
		writelog "\tDisk mode:\t\twritable"
		writelog "\tRW-Layer:\t\t$VM_DISKFILE_RW"
	else
		writelog "\tDisk mode:\t\tread-only"
	fi
	
	writelog "Virtual Hardware:"
	writelog "\tvCPU cores:\t\t${CPU_CORES}"
	writelog "\tGuest RAM:\t\t${VM_MEM} MB"
	
	# echo nur wenn HOST_MEM_REMAINING gesetzt
	if isset HOST_MEM_REMAINING; then
		writelog "\tHost RAM:\t\t${HOST_MEM_REMAINING} MB"
	fi
	writelog "\tMAC address:\t\t${VM_MAC_ADDR}"
	writelog "\tNetwork card:\t\t${NIC_MODEL}"
	writelog "\tNetwork kind:\t\t${NETWORK_MODE}"
	writelog "\tCD-ROM1:\t${CDROM_0}"
	writelog "\tCD-ROM2:\t${CDROM_1}"
	writelog "\tFloppy_A:\t${FLOPPY_0}"
	writelog "\tFloppy_B:\t${SLX_FLOPPY_IMG}"
	#writelog "\tShared Folders 'home':\t/home/${USER}"
}

call_post_source log_config_summary
