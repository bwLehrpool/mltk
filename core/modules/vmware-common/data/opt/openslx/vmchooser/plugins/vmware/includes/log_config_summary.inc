#!/bin/bash
##################################################
# Include: Print vm config summary into log file #
##################################################

log_config_summary() {
	# write all results to logfile
	# log disksetup
	writelog "Directories:"
	writelog "\tConfig dir:\t\t${VM_CONF_DIR}"
	writelog "\tConfig file:\t\t${VM_RUN_FILE}"
	writelog "\tRedo dir:\t\t${VM_REDO_DIR}"
	writelog "\tVM home:\t\t${VM_HOME}"
	writelog "\tDrag n' drop log:\t${VM_HOME}/dndlogs"
	writelog "\t/tmp info:\t\t$(grep "/tmp " /proc/mounts) $(df -h | grep " /tmp$" | awk '{print $2}')"
	
	# hw setup
	writelog "Hardware:"
	writelog "\tvCPUs:\t\t\t${CPU_CORES}"
	writelog "\tVM RAM:\t\t\t${VM_MEM} MB"
	writelog "\tMAC:\t\t\t${VM_MAC_ADDR}"
	if [ -n "${network_card}" ]; then
		writelog "\tNet Adaptor:\t\t${network_card}"
	fi
	
	writelog "\tCD-ROM 1:\t\t${CDROM0}"
	writelog "\tCD-ROM 2:\t\t${CDROM1}"
	writelog "\tFloppy A:\t\t${FLOPPY0}"
	# echo nur wenn HOST_MEM_REMAINING gesetzt
	if isset HOST_MEM_REMAINING; then
		writelog "\tRemaining host RAM:\t${HOST_MEM_REMAINING} MB"
	fi
	
	# image
	writelog "Disk image:"
	writelog "\tDisk file:\t\t${VM_DISKFILE_RO}"
	if isset VM_DISKFILE_RW; then
		writelog "\tDisk mode:\t\twritable"
		writelog "\tRW-Layer:\t\t${VM_DISKFILE_RW}"
	else
		writelog "\tDisk mode:\t\tread-only"
	fi
	writelog "\tHardware version:\t${VM_HW_VERSION}"
	writelog "\tGuest OS:\t\t${VM_OS_TYPE}"
	
	# misc
	writelog "Misc:"
	writelog "\tDisplayname:\t\t${VM_DISPLAYNAME}"
	if [ "${cap3d}" = "TRUE" -a "${enable3d}" = "TRUE" ]; then
		writelog "\t3D Graphics:\tenabled"
	fi
	if [ "${HGFS_DISABLED}" = "FALSE" ]; then
		writelog "\tShared folders:\t\tdisabled"
	else
		writelog "\tShared folders:\t\tenabled."
	fi
	# empty line at end
	writelog ""
}
call_post_source log_config_summary
