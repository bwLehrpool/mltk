###########################################
# Include: Set hardware related variables #
###########################################

get_vm_id() {
	local script=${BASH_SOURCE[-1]}
	[ -z "$script" ] && script="$0"
	if [ -n "$script" ]; then
		script=$(readlink -f "$script")
		if [ -n "$script" ] && [ -s "$script" ]; then
			#bingo
			VM_ID=$(ps ax | grep -F "$script" | grep -v 'grep' | grep -o -- "${script}.*\$" | sort -u | wc -l)
			if [ "$VM_ID" -gt 0 ]; then
				[ "${#VM_ID}" -eq 1 ] && VM_ID="0${VM_ID}"
				[ "${#VM_ID}" -gt 2 ] && VM_ID="${VM_ID:0:2}"
				[ "${#VM_ID}" -eq 2 ] && return
			fi
		fi
	fi
	# fallback: take last two digits of current pid...
	VM_ID=$(expr substr $$ $(expr ${#$} - 1) 2)
	[ "${#VM_ID}" -eq 1 ] && VM_ID="0${VM_ID}"
}

get_vm_id

# Make sure cpu_cores is not empty
cpu_cores=${cpu_cores:-"1"}

# Amount of memory for the VM. Be generous if diff is written to HDD
if mount | grep -q '^/dev/sd.*on.*/tmp'; then
	reserve=20
	min=768
	max=1800
else
	reserve=65
	min=768
	max=8192
fi

# Calculate absulute amount of RAM that should stay available to the host
reserve="$(( ( totalmem * reserve ) / 100 ))"
# Respect some upper and lower bounds for the host amount
[ "$reserve" -lt "$min" ] && reserve="$min"
[ "$reserve" -gt "$max" ] && reserve="$max"

# Get a result which can be divided by 4
mem="$(( ( ( totalmem - reserve ) / 4 ) * 4 ))"
if [ -n "$mainvirtmem" ]; then
	forcemem="$(( mainvirtmem / 4 * 4 ))"
	mem="$forcemem"
fi
hostmem="$(( totalmem - mem ))"

# Fill in VMID
macaddrsuffix=$(echo "$macaddrsuffix" | sed "s/%VMID%/${VM_ID}/")

if ! echo "$macaddrprefix" | grep -q -E '^[0-9A-F]{2}:[0-9A-F]{2}:[0-9A-F]{2}$'; then
	slxlog "virt-mac" "Could not properly generate mac address prefix (got $macaddrprefix)"
fi
if ! echo "$macaddrsuffix" | grep -q -E '^[0-9A-F]{2}:[0-9A-F]{2}:[0-9A-F]{2}$'; then
	slxlog "virt-mac" "Could not properly generate mac address suffix (got $macaddrsuffix)"
fi
macaddr="$macaddrprefix:$macaddrsuffix"

# Virtual fd/cd/dvd and drive devices, floppy b: for configuration
# if $floppy_0 from virtualization.conf set then fdtest="TRUE"
fdtest=${floppy_0:+"TRUE"}
# if $fdtest not set floppy0="FALSE", else "TRUE"
floppy0=${fdtest:-"FALSE"}
# if $cdrom_0 from virtualization.conf set then cdtest="TRUE"
cdtest=${cdrom_0:+"TRUE"}
# if $cdtest not set cdrom0="FALSE", else "TRUE"
cdrom0=${cdtest:-"FALSE"}
# if $cdrom_1 from virtualization.conf set then cdtest="TRUE"
cdtest=${cdrom_1:+"TRUE"}
# if $cdtest not set cdrom1="FALSE", else "TRUE"
cdrom1=${cdtest:-"FALSE"}

# Dynamically detect serial ports here instead of at boot time
# (virtualization.conf), since USB serial ports get quite common
# and might not be plugged in at boot time yet
serial_0=
#for port in $(awk '{ if ($1 ~ /^[0-9]+:/ && $2 != "uart:unknown") print "/dev/ttyS" sub(/:\$/, "", $1) }' /proc/tty/driver/serial); do
for port in $serial_ports /dev/ttyUSB*; do
	[ -c "$port" ] || continue
	serial_0="$port"
	break
done

parallel_0=
for port in /dev/parport*; do
	[ -c "$port" ] || continue
	parallel_0="$port"
	break
done

# RDP/VNC port (59001 - 59099)
remotedesktopport="590${VM_ID}"

# Add rw share for home dir
homesharepath="${HOME}/PERSISTENT"
homesharename="home"

# Add common share
commonsharepath="${HOME}/SHARE"
commonsharename="share"

# Set hostname: using original hostname and adding string
hostname="virt-$(hostname)"

writelog "\tVM Hostname:\t\t$hostname"

