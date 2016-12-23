########################################
# Include: Determine hw limitations depending on hwver and guest os #
########################################

MAXMEM="9999999"
MAXCORES="4"
shfolders="TRUE"
sound="es1371"

# define hardware configuration depending on the guest OS used
# this needs to be fixed and is the base for the creation of new VMs
case "$VM_OS_TYPE" in
	win31*|windows31*)
		VM_OS_TYPE="win31"
		shfolders="FALSE"
		sound="sb16"
		MAXMEM="32"
		MAXCORES="1"
	;;
	winnt*|windowsnt*)
		VM_OS_TYPE="winnt"
		shfolders="FALSE"
		sound="sb16"
		MAXMEM="1000"
		MAXCORES="2"
	;;
	win95*|windows95*)
		VM_OS_TYPE="win95"
		shfolders="FALSE"
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
		shfolders="FALSE"
		MAXMEM="4000"
		MAXCORES="4"
	;;
	win2000adv*|windows2000adv*|win2000dat*|windows2000dat*)
		VM_OS_TYPE="win2000advserv"
		shfolders="FALSE"
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
		shfolders="FALSE"
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
		shfolders="FALSE"
	;;
	# Unknown guestOS setting in .xml - this encompasses linux too,
	# as there is a multitude of different distributions. Perhaps further
	# action will be needed if this leads to problems with exotic OSs.
	*64)
		VM_OS_TYPE="other-64"
		# shfolders="FALSE"
		MAXMEM="123456"
		MAXCORES="4"
	;;
	*)
		VM_OS_TYPE="other"
		# shfolders="FALSE"
		MAXMEM="8000"
		MAXCORES="1"
	;;
esac

real_core_count="$cpu_cores"
[ "$cpu_cores" -gt "$MAXCORES" ] && cpu_cores="$MAXCORES"

# It currently makes no sense to set the virtual number of cores
# to a different value than the virtual number of cores per virtual CPU.
cores_per_socket="$cpu_cores"

if [ "x$shfolders" != "xFALSE" ] && [ "$SHARE_REMAP_MODE" != 1 ]; then
	HGFS_DISABLED="FALSE"
else
	HGFS_DISABLED="TRUE"
fi

[ "$mem" -ge "$MAXMEM" ] && mem="$MAXMEM"
[ "$HWVER" -lt "7" -a "$mem" -gt "3500" ] && mem="3500"

