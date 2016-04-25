###############################
# Include: Logging and stdout #
###############################

logging() {
	# log script information
	writelog "# File created by $0 (VMware version ${vmware_version})\n# on $(date)\n"
	## Edited for persistent mode.
	writelog "Starting with ${diskmode} mode ...\n"
	##
	
	# write all results to logfile
	# log disksetup
	writelog "Directories:"
	writelog "\tConfdir:\t${confdir}"
	writelog "\tConffile:\t${conffile}"
	writelog "\tRedodir:\t${redodir}"
	writelog "\tVMhome:\t\t${vmhome}"
	writelog "\t/tmp info: $(grep "/tmp " /proc/mounts) $(df -h | grep " /tmp$" | awk '{print $2}')"
	# hw setup
	writelog "Hardware:"
	writelog "\tMAC:\t\t00:50:56:$macaddrsuffix" # TODO: Make var at top for this, it's hard coded twice
	if [ -n "${network_card}" ]; then
		writelog "\tNet Adaptor:\t${network_card}"
	fi
	writelog "\tMem:\t\t${mem} MB"

	# echo nur wenn hostmem gesetzt
	[ -n "${hostmem}" ] && writelog "\tHostmem:\t${hostmem} MB"
	writelog "\tMax. res.:\t${xres}x${yres}"
	writelog "\tCD-ROM1:\t${cdrom0}"
	writelog "\tCD-ROM2:\t${cdrom1}"
	writelog "\tFloppy_A:\t${floppy0}"
	if [ "${serial}" = "TRUE" ]; then
		writelog "\tSerial Port:\t${serialdev}"
	fi
	if [ "${parallel}" = "TRUE" ]; then
		writelog "\tParallel Port:\t${paraldev}"
	fi

	writelog "Shared folders:"
	if [ "${ENABLE_SHARE}" = '' ]; then
		writelog "\tNo shared folders enabled."
	else
		writelog "\Shared folders enabled ($ENABLE_SHARE)"
	fi

	# image
	writelog "Diskimage:"
	writelog "\tDiskfile:\t${diskfile}"
	writelog "\tDisktype:\t${hddrv}"
	writelog "\tHWVersion:\t${hwver}"
	writelog "\tVMostype:\t${vmostype}"

	# misc
	writelog "Misc:"
	writelog "\tDisplayname:\t${displayname}"
	if [ "${cap3d}" = "TRUE" -a "${enable3d}" = "TRUE" ]; then
		writelog "\t3D Graphics:\tenabled"
	fi
	# empty line at end
	writelog ""

}
