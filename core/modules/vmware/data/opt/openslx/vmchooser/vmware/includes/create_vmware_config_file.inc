########################################
# Include: Create vmware startup files #
########################################

writelog "Creating vmware configuration file: $conffile"

MAXMEM="9999999"
MAXCORES="4"
shfolders="TRUE"
sound="es1371"

# define hardware configuration depending on the guest OS used
# this needs to be fixed and is the base for the creation of new VMs
case "$vmostype" in
	win31*|windows31*)
		vmostype="win31"
		shfolders="FALSE"
		sound="sb16"
		MAXMEM="32"
		MAXCORES="1"
	;;
	winnt*|windowsnt*)
		vmostype="winnt"
		shfolders="FALSE"
		sound="sb16"
		MAXMEM="1000"
		MAXCORES="2"
	;;
	win95*|windows95*)
		vmostype="win95"
		shfolders="FALSE"
		MAXMEM="96"
		MAXCORES="1"
	;;
	win98*|windows98*)
		vmostype="win98"
		MAXMEM="256"
		MAXCORES="1"
	;;
	winme*|windowsme*)
		vmostype="winme"
		MAXMEM="384"
		MAXCORES="1"
	;;
	win2000|windows2000|win2000pro*)
		vmostype="win2000pro"
		MAXMEM="4000"
		MAXCORES="2"
	;;
	win2000srv*|windows2000srv*|win2000serv*|windows2000serv*)
		vmostype="win2000serv"
		shfolders="FALSE"
		MAXMEM="4000"
		MAXCORES="4"
	;;
	win2000adv*|windows2000adv*|win2000dat*|windows2000dat*)
		vmostype="win2000advserv"
		shfolders="FALSE"
		MAXMEM="8000"
		MAXCORES="8"
	;;
	winnet*64|win*2003*64|windowsnet*64)
		vmostype="winnetstandard-64"
		MAXMEM="8000"
	;;
	winnet*|win*2003*|windowsnet*)
		vmostype="winnetstandard"
		MAXMEM="4000"
	;;
	winxphome*|windowsxphome*)
		vmostype="winxphome"
		MAXMEM="4000"
		MAXCORES="1"
	;;
	winxp*64|windowsxp*64)
		vmostype="winxppro-64"
		MAXMEM="8000"
		MAXCORES="2"
	;;
	winxp*|windowsxp*)
		vmostype="winxppro"
		MAXMEM="4000"
		MAXCORES="1"
	;;
	winvista-64)
		vmostype="winvista-64"
		MAXMEM="16000"
		MAXCORES="2"
	;;
	windows7-64)
		vmostype="windows7-64"
		MAXMEM="32000"
		MAXCORES="4"
	;;
	windows8-64)
		vmostype="windows8-64"
		MAXMEM="32000"
		MAXCORES="8"
	;;
	windows9-64)
		vmostype="windows9-64"
		MAXMEM="64000"
		MAXCORES="8"
	;;
	winvista)
		vmostype="winvista"
		MAXMEM="8000"
		MAXCORES="2"
	;;
	windows7)
		vmostype="windows7"
		MAXMEM="8000"
		MAXCORES="4"
	;;
	windows8)
		vmostype="windows8"
		MAXMEM="8000"
		MAXCORES="4"
	;;
	windows9)
		vmostype="windows9"
		MAXMEM="8000"
		MAXCORES="4"
	;;
	win*64)
		MAXMEM="16000"
		MAXCORES="4"
	;;
	win*)
		MAXMEM="8000"
	;;
	dos|msdos*|ms-dos*)
		vmostype="dos"
		shfolders="FALSE"
		MAXMEM="128"
	;;
	macos*64)
		vmostype="freebsd-64"
		MAXMEM="4000"
		MAXCORES="2"
	;;
	macos*)
		vmostype="freebsd"
		MAXMEM="4000"
		MAXCORES="1"
	;;
	beos*)
		vmostype="other"
		shfolders="FALSE"
	;;
	# Unknown guestOS setting in .xml - this encompasses linux too,
	# as there is a multitude of different distributions. Perhaps further
	# action will be needed if this leads to problems with exotic OSs.
	*64)
		vmostype="other-64"
		# shfolders="FALSE"
		MAXMEM="123456"
		MAXCORES="4"
	;;
	*)
		vmostype="other"
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

if [ "x$shfolders" != "xFALSE" ]; then
	ENABLE_SHARE='sharedFolder.option = "alwaysEnabled"'
else
	ENABLE_SHARE=''
fi

[ "$mem" -ge "$MAXMEM" ] && mem="$MAXMEM"
[ "$hwver" -lt "7" -a "$mem" -gt "3500" ] && mem="3500"

# set the appropriate permissions for the vmware config file
chmod u+rwx "${conffile}" >/dev/null 2>&1

