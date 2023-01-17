#!/bin/bash
###########################################
# Include: Set hardware related variables #
###########################################
# New way of handling MAC address prefixes to support
# nested and concurrent run-virt invocations.
# Using a list of predefined MAC prefixes to use for VMs,
# check if the host's MAC address prefix is in that list.
# If not, use the first prefix in the list. If it is
# use the next prefix in the list. If the host's prefix
# is the last in the list, use the first one again.
# This way we support up to 10 nested/concurrent VMs.
# Use: 00:FF:00 for firtual machines ;)

## Functions ##
# Import /run/hwinfo variables
import_hwinfo() {
	$(safesource "/run/hwinfo")
}
# Sets the VM's hostname to the original hostname prefixed with a fixed string and its ID 
set_virt_hostname() {
	declare -rg HOSTNAME="virt${VM_ID}-$(hostname)"
	writelog "\tVM Hostname:\t\t$HOSTNAME"
}
# Derives the amount of memory allocated to the VM from the
# host's total memory (previously determined by systemd-run_virt_env)
set_virt_memory() {
	# Make sure we have a VM_ID and HOST_MEM_TOTAL (read from virtualization.conf)
	if isempty HOST_MEM_TOTAL; then
		writelog "HOST_MEM_TOTAL is empty! Was '$VMCHOOSER_CONF_DIR/virtualization.conf' sourced?"
		EXIT_TYPE="internal" EXIT_REASON="Konnte die Größe des Arbeitsspeichers nicht ermitteln!" cleanexit 1
	fi

	# Calculare amount of memory for the VM. Be generous if diff is written to HDD.
	# First, determine how much should be left to the host system. Minimum is 1GB.
	local min=1000
	local base=777
	local reserve max disk_swap
	if $TMPDIR_IN_RAM; then
		# tmp in RAM, so reserve a lot for the system since that is where the HDD diff goes.
		reserve=65
		max=8192 # But max. 8GB
	else
		# We have a backing store for the HDD diff, be generous
		if lsmod | grep -q '^nvidia'; then
			# With nvidia cards, the host needs a bit more RAM.
			max=3300
			reserve=18
		else
			max=2000
			reserve=10
		fi
	fi
	# Non-zram swap (KB)
	disk_swap="$( gawk 'BEGIN{a=0} $1 ~ /^\/dev\// && $1 !~ /^\/dev\/zram/ {a+= $3} END{print a}' /proc/swaps )"
	if (( disk_swap > 2500000 )); then # > 2.5GB
		reserve=$(( (reserve * 3) / 4 ))
		max=$(( max - 200 ))
	fi

	# Calculate absolute amount of RAM that should stay available to the host
	reserve="$(( ( (HOST_MEM_TOTAL - 4000) * reserve ) / 100 + base ))"
	# Respect some upper and lower bounds for the host amount
	(( reserve >= min )) || reserve="$min"
	(( reserve <= max )) || reserve="$max"

	# Get a result which can be divided by 4
	declare -g VM_MEM="$(( ( ( HOST_MEM_TOTAL - reserve ) / 4 ) * 4 ))"
	declare -g HOST_MEM_REMAINING="$(( HOST_MEM_TOTAL - VM_MEM ))"
	# NOTE: removed old code that evaluated 'mainvirtmem'
	# as it did nothing and we don't know what the idea was... 
}

# New way to generate MAC addresses:
# MAC_PREFIXES is a statically declared list of prefixes which
# can be used for VMs. To support nested VMs, we just look for
# the MAC prefix of the host and if it is present in the list,
# we just take the next one in the list. Currently support up to 10.
# If none are found, we just take the first prefix.
# Suffix template is taken from /opt/openslx/vmchooser/config/virtualization.conf
# and will have the VMID inserted as first byte.
set_virt_mac() {
	local i mac byte
	# First, determine prefix of the host's MAC address
	isset HOSTMACADDR || writelog "Host's MAC address was not defined in ${VMCHOOSER_CONF_DIR}/virtualizer.conf"
	isset MACADDRPREFIX || writelog "No MAC address prefix was defined in ${VMCHOOSER_CONF_DIR}/virtualizer.conf"

	# Fill in VM_ID
	local MACADDRSUFFIX=${MACADDRSUFFIX//%VMID%/"${VM_ID}"}
	
	if ! echo "$MACADDRPREFIX" | grep -q -E '^[0-9A-F]{2}:[0-9A-F]{2}:[0-9A-F]{2}$'; then
		slxlog "virt-mac" "Could not properly generate mac address prefix (got $MACADDRPREFIX)"
	fi
	if ! echo "$MACADDRSUFFIX" | grep -q -E '^[0-9A-F]{2}:[0-9A-F]{2}:[0-9A-F]{2}$'; then
		slxlog "virt-mac" "Could not properly generate mac address suffix (got $MACADDRSUFFIX)"
	fi
	declare -g VM_MAC_ADDR="$MACADDRPREFIX:$MACADDRSUFFIX"
	declare -g OTHER_NICS=
	# Set up additional NICs if enabled
	if [ "$SLX_BRIDGE_OTHER_NICS" = "yes" ]; then
		for i in /sys/class/net/br-nic-*/address; do
			[ -s "$i" ] || continue
			mac=$(tr '[a-f]' '[A-F]' < "$i")
			# Calc checksum of mac by summing the bytes mod 256
			byte=$(( $(<<<$mac sed 's/^/ ( 0x/;s/:/ + 0x/g;s/$/ ) % 256/') ))
			byte=$(printf "%02X" "$byte")
			# (PR:EF:IX):LA:ST:CH - use general prefix for VMs, then last two octets of source mac, plus the checksum
			mac="${MACADDRPREFIX}:${mac:12:5}:${byte}"
			OTHER_NICS+=" $mac"
		done
	fi
}

# Virtual fd/cd/dvd and drive devices, floppy b: for configuration
# If the variables in virtualization.conf is set, "TRUE" is assigned,
# "FALSE" otherwise.
check_optical_drives() {
	# TODO This should only handle exam mode here, maybe even have one function
	# that prepares all exam mode related stuff. Then create the TRUE/FALSE
	# variables somewhere in the vmware plugin, since it's specific to that
	# plugin.
	FLOPPY0="FALSE"
	CDROM0="FALSE"
	CDROM1="FALSE"
	if [ -z "$SLX_EXAM" ]; then
		if [ -n "$FLOPPY_0" ]; then
			FLOPPY0="TRUE"
		fi
		if [ -n "$CDROM_0" ]; then
			CDROM0="TRUE"
		fi
		if [ -n "$CDROM_1" ]; then
			CDROM1="TRUE"
		fi
	fi
	readonly FLOPPY0 CDROM0 CDROM1
}

# Dynamically detect serial ports here instead of at boot time
# (virtualization.conf), since USB serial ports get quite common
# and might not be plugged in at boot time yet
set_serial_ports() {
	declare -g SERIAL0=
	#for port in $(awk '{ if ($1 ~ /^[0-9]+:/ && $2 != "uart:unknown") print "/dev/ttyS" sub(/:\$/, "", $1) }' /proc/tty/driver/serial); do
	for PORT in $SERIAL_PORTS /dev/ttyUSB*; do
		[ -c "$PORT" ] || continue
		SERIAL0="$PORT"
		break
	done

	declare -g PARALLEL0=
	for PORT in /dev/parport*; do
		[ -c "$PORT" ] || continue
		PARALLEL0="$PORT"
		break
	done

	readonly SERIAL0 PARALLEL0
}

get_displays() {
	local list=$( mktemp )
	beamergui -d | grep -v '^#' > "$list"
	# These are logical screens, ie. cloned outputs count only once
	declare -g DISPLAY_COUNT="$( < "$list"  wc -l || echo 1 )"
	declare -g RESOLUTIONS="$( < "$list"  awk '{printf "%dx%d ", $3, $4}' )"
	RESOLUTIONS=${RESOLUTIONS% } # trim
	readonly RESOLUTIONS DISPLAY_COUNT
	rm -f -- "$list"
}

## MAIN ##
call_post_source \
	import_hwinfo \
	set_virt_memory \
	set_virt_mac \
	set_virt_hostname \
	set_serial_ports \
	check_optical_drives \
	get_displays

