###########################################
# Include: Set hardware related variables #
###########################################

# take last two digits of current pid...
VM_ID=$(expr substr $$ $(expr ${#$} - 1) 2)

# Make sure cpu_cores is not empty
cpu_cores=${cpu_cores:-"1"}

# Amount of memory for the VM. Be generous if diff is written to HDD
if mount | grep -q '^/dev/sd.*on.*/tmp'; then
	reserve=20
	min=768
	max=1536
else
	reserve=65
	min=768
	max=8192
fi

# Calculate absulute amount of RAM that should stay available to the host
reserve="$[ ( $totalmem * $reserve ) / 100 ]"
# Respect some upper and lower bounds for the host amount
[ "$reserve" -lt "$min" ] && reserve="$min"
[ "$reserve" -gt "$max" ] && reserve="$max"

# Get a result which can be divided by 4
mem="$[ ( ( $totalmem - $reserve ) / 4 ) * 4 ]"
if [ -n "$mainvirtmem" ]; then
	forcemem="$[ "$mainvirtmem" / 4 * 4 ]"
	mem="$forcemem"
fi
hostmem="$[ $totalmem - $mem ]"

# TODO: We should only generate the suffix here (last 3 octets) as the first 3 are
# dependant on the virtualizer/emulator. Check if any run-virt.include still relies on
# $macguestpart/$macaddr. If so, fix it to use its specific first 3 bytes
# and append $macaddrssuffix
macguestpart="00:50:56:${VM_ID}"
machostpart=$(echo "${hostmacaddr}" | awk -F ":" '{print $(NF-1)":"$NF}')
macaddr=$(echo "${macguestpart}:${machostpart}" | tr "[a-z]" "[A-Z]")
macaddrsuffix=$(echo "${VM_ID}:${machostpart}" | tr "[a-z]" "[A-Z]")

if ! echo "$macaddrsuffix" | grep -q -E '^[0-9A-F]{2}:[0-9A-F]{2}:[0-9A-F]{2}$'; then
	slxlog "virt-mac" "Could not properly generate mac address suffix (got $macaddrsuffix)"
fi

# Virtual fd/cd/dvd and drive devices, floppy b: for configuration
# if $floppy_0 from virtualization.conf set then fdtest="TRUE"
fdtest=${floppy_0:+"TRUE"}
# if $fdtest not set floppy0="FALSE", else "TRUE"
floppy0=${fdtest:-"FALSE"}
floppy1="TRUE"
floppy1name="$VMCHOOSER_DIR/data/loopimg/fd.img"
# if $cdrom_0 from virtualization.conf set then cdtest="TRUE"
cdtest=${cdrom_0:+"TRUE"}
# if $cdtest not set cdrom0="FALSE", else "TRUE"
cdrom0=${cdtest:-"FALSE"}
# if $cdrom_1 from virtualization.conf set then cdtest="TRUE"
cdtest=${cdrom_1:+"TRUE"}
# if $cdtest not set cdrom1="FALSE", else "TRUE"
cdrom1=${cdtest:-"FALSE"}
# IDE is expected default, test for the virtual disk image type should
# be done while creating the runscripts ...
# TODO enable SCSI support instead of just disabling it.
ide="TRUE"
scsi="FALSE"
hddrv="ide"
audio="true"
remotedesktopport="590${VM_ID}"


# Enable 3D
enable3d=$(grep -i -o "<enable3d param=.*" "${xmlfile}" | awk -F '"' '{print $2}' | rv_clean_string)

# Add rw share for home dir
homesharepath="${HOME}/PERSISTENT"
homesharename="home"

# Add common share
commonsharepath="${HOME}/SHARE"
commonsharename="share"

# Set hostname: using original hostname and adding string
hostname="virt-$(hostname)"

writelog "\tVM Hostname:\t\t$hostname"

