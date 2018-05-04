#!/bin/bash
##########################################################################
# Include: Determine limitations caused by hardware version and guest os #
##########################################################################
## Functions
set_vm_hardware_limits() {
	# Sanity checks
	if isempty VM_OS_TYPE; then
		writelog "VM_OS_TYPE is empty! This should have been parsed from the VM metadata."
		EXIT_TYPE="user" EXIT_REASON="Konnte Gastbetriebsystems der gewählten virtuellen Maschine nicht ermitteln!" cleanexit 1
	fi

	declare -g MAXMEM="9999999"
	declare -g MAXCORES="4"
	declare -g SHARED_FOLDERS="TRUE"
	declare -g SOUND_DEV="es1371"

	# define hardware configuration depending on the guest OS used
	# this needs to be fixed and is the base for the creation of new VMs
	case "$VM_OS_TYPE" in
		win31*|windows31*)
			VM_OS_TYPE="win31"
			SHARED_FOLDERS="FALSE"
			SOUND_DEV="sb16"
			MAXMEM="32"
			MAXCORES="1"
		;;
		winnt*|windowsnt*)
			VM_OS_TYPE="winnt"
			SHARED_FOLDERS="FALSE"
			SOUND_DEV="sb16"
			MAXMEM="1000"
			MAXCORES="2"
		;;
		win95*|windows95*)
			VM_OS_TYPE="win95"
			SHARED_FOLDERS="FALSE"
			MAXMEM="96"
			MAXCORES="1"
		;;
		win98*|windows98*)
			VM_OS_TYPE="win98"
			MAXMEM="256"
			MAXCORES="1"
		;;
		winme*|windowsme*)
			VM_OS_TYPE="winme"
			MAXMEM="384"
			MAXCORES="1"
		;;
		win2000|windows2000|win2000pro*)
			VM_OS_TYPE="win2000pro"
			MAXMEM="4000"
			MAXCORES="2"
		;;
		win2000srv*|windows2000srv*|win2000serv*|windows2000serv*)
			VM_OS_TYPE="win2000serv"
			SHARED_FOLDERS="FALSE"
			MAXMEM="4000"
			MAXCORES="4"
		;;
		win2000adv*|windows2000adv*|win2000dat*|windows2000dat*)
			VM_OS_TYPE="win2000advserv"
			SHARED_FOLDERS="FALSE"
			MAXMEM="8000"
			MAXCORES="8"
		;;
		winnet*64|win*2003*64|windowsnet*64)
			VM_OS_TYPE="winnetstandard-64"
			MAXMEM="8000"
			MAXCORES="8"
		;;
		winnet*|win*2003*|windowsnet*)
			VM_OS_TYPE="winnetstandard"
			MAXMEM="4000"
			MAXCORES="8"
		;;
		winxphome*|windowsxphome*)
			VM_OS_TYPE="winxphome"
			MAXMEM="4000"
			MAXCORES="2"
		;;
		winxp*64|windowsxp*64)
			VM_OS_TYPE="winxppro-64"
			MAXMEM="8000"
			MAXCORES="8"
		;;
		winxp*|windowsxp*)
			VM_OS_TYPE="winxppro"
			MAXMEM="4000"
			MAXCORES="4"
		;;
		winvista-64)
			VM_OS_TYPE="winvista-64"
			MAXMEM="16000"
			MAXCORES="4"
		;;
		windows7-64)
			VM_OS_TYPE="windows7-64"
			MAXMEM="32000"
			MAXCORES="8"
		;;
		windows8-64)
			VM_OS_TYPE="windows8-64"
			MAXMEM="32000"
			MAXCORES="8"
		;;
		windows9-64)
			VM_OS_TYPE="windows9-64"
			MAXMEM="64000"
			MAXCORES="8"
		;;
		winvista)
			VM_OS_TYPE="winvista"
			MAXMEM="8000"
			MAXCORES="2"
		;;
		windows7)
			VM_OS_TYPE="windows7"
			MAXMEM="8000"
			MAXCORES="4"
		;;
		windows8)
			VM_OS_TYPE="windows8"
			MAXMEM="8000"
			MAXCORES="4"
		;;
		windows9)
			VM_OS_TYPE="windows9"
			MAXMEM="8000"
			MAXCORES="4"
		;;
		win*64)
			MAXMEM="16000"
			MAXCORES="4"
		;;
		win*)
			MAXMEM="8000"
			MAXCORES="1"
		;;
		dos|msdos*|ms-dos*)
			VM_OS_TYPE="dos"
			SHARED_FOLDERS="FALSE"
			MAXMEM="128"
			MAXCORES="1"
		;;
		macos*64)
			VM_OS_TYPE="freebsd-64"
			MAXMEM="4000"
			MAXCORES="2"
		;;
		macos*)
			VM_OS_TYPE="freebsd"
			MAXMEM="4000"
			MAXCORES="1"
		;;
		beos*)
			VM_OS_TYPE="other"
			SHARED_FOLDERS="FALSE"
		;;
		# Unknown guestOS setting in .xml - this encompasses linux too,
		# as there is a multitude of different distributions. Perhaps further
		# action will be needed if this leads to problems with exotic OSs.
		*64)
			VM_OS_TYPE="other-64"
			# SHARED_FOLDERS="FALSE"
			MAXMEM="123456"
			MAXCORES="4"
		;;
		*)
			VM_OS_TYPE="other"
			# SHARED_FOLDERS="FALSE"
			MAXMEM="8000"
			MAXCORES="1"
		;;
	esac
	declare -rg CPU_CORES="${HW_THREADS:-1}"
	declare -rg HOST_CORE_COUNT="$CPU_CORES"
	[ "$CPU_CORES" -gt "$MAXCORES" ] && CPU_CORES="$MAXCORES"
	
	# It currently makes no sense to set the virtual number of cores
	# to a different value than the virtual number of cores per virtual CPU.
	declare -rg VM_CORES_PER_SOCKET="$CPU_CORES"
	
	if [ "x$SHARED_FOLDERS" != "xFALSE" ] && [ "$SHARE_REMAP_MODE" -gt 1 ]; then
		declare -rg HGFS_DISABLED="FALSE"
	else
		declare -rg HGFS_DISABLED="TRUE"
	fi
	
	[ "${VM_MEM}" -ge "${MAXMEM}" ] && VM_MEM="${MAXMEM}"
	[ "${VM_HW_VERSION}" -lt "7" -a "${VM_MEM}" -gt "3500" ] && VM_MEM="3500"
	return 0
}

## MAIN ##
call_post_source set_vm_hardware_limits
