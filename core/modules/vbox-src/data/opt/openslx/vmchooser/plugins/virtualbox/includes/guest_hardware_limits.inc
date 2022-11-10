################################################################################
# Include: Set hardware limitations determined by the guest os type            #
################################################################################

set_hardware_limits() {

	# support shared folders by default, disable it if guest OS doesn't support it
	declare -g SHARED_FOLDERS="TRUE"
	
	# set some base configuration depending on the guest operating system
	# NOTE: list supported OS type with: VBoxManage list ostypes
	case "${VM_OS_TYPE}" in
		Windows31)
			MAXCORES="1"
			MAXMEM="32"
			MAXVRAM="16"
			SHARED_FOLDERS="FALSE"
			;;
		WindowsNT*)
			MAXCORES="4"
			MAXMEM="1024"
			MAXVRAM="32"
			SHARED_FOLDERS="FALSE"
			;;
		Windows95)
			MAXCORES="1"
			MAXMEM="128"
			MAXVRAM="32"
			SHARED_FOLDERS="FALSE"
			;;
		Windows98)
			MAXCORES="1"
			MAXMEM="256"
			MAXVRAM="64"
			;;
		WindowsMe)
			MAXCORES="1"
			MAXMEM="384"
			MAXVRAM="64"
			;;
		Windows2000|Windows2003)
			MAXCORES="4"
			MAXMEM="4000"
			MAXVRAM="128"
			;;
		Windows2003_64|WindowsXP_64)
			MAXCORES="4"
			MAXMEM="8000"
			MAXVRAM="128"
			;;
		WindowsXP)
			MAXCORES="4"
			MAXMEM="4000"
			MAXVRAM="128"
			;;
		WindowsVista)
			MAXCORES="4"
			MAXMEM="4000"
			MAXVRAM="128"
			;;
		WindowsVista_64)
			MAXCORES="4"
			MAXMEM="16000"
			MAXVRAM="128"
			;;
		Windows7|Windows8|Windows81|Windows10)
			MAXCORES="4"
			MAXMEM="4000"
			MAXVRAM="128"
			;;
		Windows7_64|Windows8_64|Windows81_64|Windows10_64)
			MAXCORES="4"
			MAXMEM="32000"
			MAXVRAM="128"
			;;
		ArchLinux|Debian|OpenSUSE|Fedora|Gentoo|Mandriva|RedHat|Turbolinux|Ubuntu)
			MAXCORES="4"
			MAXMEM="4000"
			MAXVRAM="128"
			;;
		ArchLinux_64|Debian_64|OpenSUSE_64|Fedora_64|Gentoo_64|Mandriva_64|RedHat_64|Turbolinux_64|Ubuntu_64)
			MAXCORES="8"
			MAXMEM="32000"
			MAXVRAM="128"
			;;
		*Linux*)
			MAXCORES="4"
			MAXMEM="1024"
			MAXVRAM="32"
			;;
		*Linux*64)
			MAXCORES="8"
			MAXMEM="8000"
			MAXVRAM="128"
			;;
		OS2eCS)
			MAXMEM="256"
			MAXCORES=1
	    ;;
	  *)
			MAXMEM="512"
			MAXCORES="1"
		;;
		# TODO: FreeBSD, NetBSD, MacOS*, Solaris, Oracle, ...
	esac

	# use HW_CORES not HW_THREADS since virtualbox does not
	# seem to handle hyperthreading all too well...
	declare -g CPU_CORES="${HW_CORES:-1}"
	(( CPU_CORES > MAXCORES )) && CPU_CORES="${MAXCORES}"
	(( VM_MEM > MAXMEM )) && VM_MEM="${MAXMEM}"

	# currently not used, keep the VRAMSize setting from the uploaded configuration
	declare -rg GUESTVRAM=$MAXVRAM

	# check if shared folders should be activated according
	# to SHARE_REMAP_MODE set by /opt/openslx/inc/shares
	VBOX_SHARED_FOLDERS=
	if [ "${SHARED_FOLDERS^^}" = "TRUE" ] && (( SHARE_REMAP_MODE > 1 )); then
		declare -rg VBOX_SHARED_FOLDERS="1"
	fi
	return 0
}

call_post_source set_hardware_limits
