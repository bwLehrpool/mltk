##################################################################
# qemu/kvm include: determine virtual hardware based on guest OS #
##################################################################

set_vm_hw_limits() {
	# FIXME when we have better metadata for qemu
	if isempty VM_OS_TYPE; then
		writelog "VM_OS_TYPE is empty! This should have been parsed from the XML file"
		EXIT_TYPE="user" EXIT_REASON="Konnte Gastbetriebsystems der virtuellen Maschine nicht ermitteln!" cleanexit 1
	fi

	# define global hardware-related variables
	declare -g MAXMEM="9999999"
	declare -g MAXCORES="4"
	declare -g SOUND_DEV="es1370"

	# determine if we need a 32bit or 64bit machine
	# FIXME depending on the guest OS for now
	# (sadly does not support all the exotic OSs the
	# the world has even seen like vmware plugin :<)
	declare -g VIRTCMD=
	case "${VM_OS_TYPE}" in
		beos*)
			VIRTCMD="qemu-system-i386"
			VIRTCMDOPTS="${VIRTCMDOPTS} -machine accel=tcg"
			SOUND_DEV="sb16"
			;;
		win3*|win*3*|Win*3*)
			VIRTCMD="qemu-system-i386"
			VIRTCMDOPTS="${VIRTCMDOPTS} -machine accel=tcg"
			SOUND_DEV="sb16"
			MAXMEM=32
			;;
		win95*|Win*95)
			VIRTCMD="qemu-system-i386"
			VIRTCMDOPTS="${VIRTCMDOPTS} -machine accel=tcg"
			MAXMEM=96
			;;
		win98)
			VIRTCMD="qemu-system-i386"
			VIRTCMDOPTS="${VIRTCMDOPTS} -machine accel=tcg"
			MAXMEM=256
			;;
		winme*|windowsme*)
			VIRTCMD="qemu-system-i386"
			VIRTCMDOPTS="${VIRTCMDOPTS} -machine accel=tcg"
			SOUND_DEV="ac97"
			MAXMEM=384
			;;
		winxp*64|windowsxp*64)
			VIRTCMD="qemu-system-x86_64"
			VIRTCMDOPTS="${VIRTCMDOPTS} -machine accel=kvm"
			SOUND_DEV="ac97"
			;;
		windows7|windows8|windows9)
			VIRTCMD="qemu-system-x86_64"
			VIRTCMDOPTS="${VIRTCMDOPTS} -machine accel=kvm"
			SOUND_DEV="ac97"
			MAXMEM="8000"
			MAXCORES="4"
			;;
		windows7-64|windows8-64|windows9-64)
			VIRTCMD="qemu-system-x86_64"
			VIRTCMDOPTS="${VIRTCMDOPTS} -machine accel=kvm"
			SOUND_DEV="ac97"
			MAXMEM="32000"
			MAXCORES="8"
			;;
		*64)
			VIRTCMD="qemu-system-x86_64"
			VIRTCMDOPTS="${VIRTCMDOPTS} -machine accel=kvm"
			MAXMEM="16000"
			MAXCORES="4"
			;;
		*)
			VIRTCMD="qemu-system-i386"
			VIRTCMDOPTS="${VIRTCMDOPTS} -machine accel=kvm"
			MAXMEM="8000"
			MAXCORES="1"
			;;
	esac

	# check for allocated cores
	declare -rg HOST_CORE_COUNT="${CPU_CORES}"
	[ "${CPU_CORES}" -gt "${MAXCORES}" ] && CPU_CORES="${MAXCORES}"

	# check if memory set by the generic run-virt is above the threshold
	[ "${VM_MEM}" -gt "${MAXMEM}" ] && VM_MEM="${MAXMEM}"
	return 0
}

## MAIN ##
call_post_source set_vm_hw_limits
