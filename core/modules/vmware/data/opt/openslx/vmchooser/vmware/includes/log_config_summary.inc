##################################################
# Include: Print vm config summary into log file #
##################################################

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
writelog "\tMAC:\t\t$macaddr"
if [ -n "${network_card}" ]; then
	writelog "\tNet Adaptor:\t${network_card}"
fi
writelog "\tMem:\t\t${mem} MB"

# echo nur wenn hostmem gesetzt
[ -n "${hostmem}" ] && writelog "\tHostmem:\t${hostmem} MB"
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
if [ "${HGFS_DISABLED}" = "FALSE" ]; then
	writelog "\tNo shared folders enabled."
else
	writelog "\Shared folders enabled."
fi

# image
writelog "Diskimage:"
writelog "\tDiskfile:\t${VM_DISKFILE_RO}"
writelog "\tHWVersion:\t${HWVER}"
writelog "\tVMostype:\t${VM_OS_TYPE}"

# misc
writelog "Misc:"
writelog "\tDisplayname:\t${VM_DISPLAYNAME}"
if [ "${cap3d}" = "TRUE" -a "${enable3d}" = "TRUE" ]; then
	writelog "\t3D Graphics:\tenabled"
fi
# empty line at end
writelog ""

