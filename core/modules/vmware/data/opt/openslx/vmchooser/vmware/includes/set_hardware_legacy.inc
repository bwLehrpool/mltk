#########################################
# Include: Hardware checks and settings #
#########################################

set_hardware_legacy() {

# use different network card (default e1000, vlance, vmxnet)
if [ -n "${network_card}" ]; then
	network_virtualDev="ethernet0.virtualDev = \"${network_card}\""
else
	network_virtualDev='ethernet0.virtualDev = "e1000"'
fi

# use different network card (default e1000, vlance, vmxnet)
hostdev="/dev/vmnet1"
case "${network_kind}" in
	bridge|bridged)
		hostdev="/dev/vmnet0"
	;;
	nat)
	        hostdev="/dev/vmnet1"
	;;
	hostonly|host-only)
        	hostdev="/dev/vmnet2"
	;;
esac

# set standard sound card, overwrite depending on OS (options sb16, es1371, hdaudio)
sound="es1371"

# set sound card explicitly if there is more than one card in the host system
sound_fileName='sound.fileName = "-1"
sound.autodetect = "TRUE"'

# check for 3D configuration setting
case "$enable3d" in
	true|yes)
		enable3d="TRUE"
	;;
	*)
		enable3d="FALSE"
	;;
esac

# check for whitelisted HW-3D
FORCE3D=""
if [ -n "$SLX_VMWARE_3D" ]; then
	FORCE3D='mks.gl.allowBlacklistedDrivers = "TRUE"'
fi

# serial/parallel port defined (e.g. "ttyS0, lp0" or "autodetect")
case "$serial" in
	tty*)
		if [ -e "/dev/$serial" ]; then
			serialdev="serial0.filename = \"/dev/${serial}\""
			serial="TRUE"
		else
			serialdev="# no serial port configured"
			serial="FALSE"
		fi
	;;
	auto*)
		serialdev="serial0.autodetect = \"TRUE\""
		serial="TRUE"
	;;
	*)
		serialdev="# no serial port configured"
		serial="FALSE"
	;;
esac

case "$parallel" in
	lp*|parport*)
		if [ -e "/dev/$parallel" ]; then
			paraldev="parallel0.filename = \"/dev/${parallel}\""
			paralbidi="TRUE"
			parallel="TRUE"
		else
			paraldev="# no parallel port configured"
			paralbidi="FALSE"
			parallel="FALSE"
		fi
	;;
	auto*)
		paraldev="parallel0.autodetect = \"TRUE\""
		paralbidi="TRUE"
		parallel="TRUE"
	;;
	*)
		paraldev="# no parallel port configured"
		paralbidi="FALSE"
		parallel="FALSE"
	;;
esac

# check if ide/scsi and hwver of image
# read only the first 30 lines to be sure
imghead=$(head -n 30 "${diskfile}")

## Added to handle persistent snapshots.
if [[ "$originalVMDKFilePath" ]]; then
	echo "Select \"${originalVMDKFilePath}\" as information base for \"${diskfile}\"."
	imghead=$(head -n 30 "$originalVMDKFilePath") && \

	# Support suspend mode.
	stateFilePath="$(readlink -f "$(dirname "$diskfile")/"*.vmss)" && \
	if [ -f "$stateFilePath" ]; then
		echo "Found  state file \"$stateFilePath\"." && \
		stateFileConfiguration="
		checkpoint.vmState = \"$stateFilePath\""
	fi
	POSTRUN="stateFilePath=\"\$(readlink -f \"${confdir}/\"*.vmss)\" && memoryFilePath=\"\$(readlink -f \"${confdir}/\"*.vmem)\" && [ -f \"\$stateFilePath\" ] && echo \"Saving state and memory file \\\"\$stateFilePath\\\" and \\\"\$memoryFilePath\\\".\" && mv -f \"\$stateFilePath\" \"$(dirname "$diskfile")/\" && mv -f \"\$memoryFilePath\" \"$(dirname "$diskfile")/\""
fi

##
hwver=$(echo "${imghead}" | grep -m1 -ia "ddb.virtualHWVersion" | awk -F '"' '{print $2}')


if [ -z "$override_hddtype" ]; then
        hddrv=$(echo "${imghead}" | grep -m1 -ia "ddb.adapterType" | awk -F '"' '{print $2}')
else
        hddrv=$override_hddtype
fi

PCIE=
case "${hddrv}" in
	ide)
		ide="TRUE"
		scsi="FALSE"
	;;
	lsisas*)
		ide="FALSE"
		scsi="TRUE"
		PCIE='pciBridge4.present = "TRUE"
		pciBridge4.virtualDev = "pcieRootPort"
		pciBridge4.functions = "8"'
	;;
	lsilogic|buslogic)
		ide="FALSE"
		scsi="TRUE"
	;;
	scsi)
		ide="FALSE"
		scsi="TRUE"
		hddrv="lsilogic"
	;;
	*)
		slxlog "virt-vmware-hdd" "vmware: Unknown HDD adapter type $hddrv"
	;;
esac

}
