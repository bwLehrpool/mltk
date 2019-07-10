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
	local bits=64
	case "${VM_OS_TYPE}" in
		beos*)
			bits=32
			SOUND_DEV="sb16"
			;;
		win3*|win*3*|Win*3*)
			bits=32
			SOUND_DEV="sb16"
			MAXMEM=32
			;;
		win95*|Win*95)
			bits=32
			MAXMEM=96
			;;
		win98)
			bits=32
			MAXMEM=256
			;;
		winme*|windowsme*)
			bits=32
			SOUND_DEV="ac97"
			MAXMEM=384
			;;
		winxp*64|windowsxp*64)
			SOUND_DEV="ac97"
			;;
		windows7|windows8|windows9)
			SOUND_DEV="ac97"
			MAXMEM="8000"
			MAXCORES="4"
			;;
		windows7-64|windows8-64|windows9-64)
			SOUND_DEV="ac97"
			MAXMEM="32000"
			MAXCORES="8"
			;;
		*64)
			MAXMEM="16000"
			MAXCORES="4"
			;;
		*)
			bits=32
			MAXMEM="8000"
			MAXCORES="1"
			;;
	esac
	declare -g VIRTCMD
	if [ "$bits" = 32 ]; then
		declare -rg VIRTCMD="qemu-system-i386"
		VIRTCMDOPTS+=( "-machine" "accel=tcg" ) # TODO: 32bit VMs can't use kvm!?
	else # 64 bit
		declare -rg VIRTCMD="qemu-system-x86_64"
		VIRTCMDOPTS+=( "-machine" "accel=kvm" )
	fi


	# check for allocated cores
	declare -rg HOST_CORE_COUNT="${CPU_CORES}"
	[ "${CPU_CORES}" -gt "${MAXCORES}" ] && CPU_CORES="${MAXCORES}"

	# check if memory set by the generic run-virt is above the threshold
	[ "${VM_MEM}" -gt "${MAXMEM}" ] && VM_MEM="${MAXMEM}"
	return 0
}

## MAIN ##
call_post_source set_vm_hw_limits
