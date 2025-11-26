#!/bin/bash
################################################
# Include: Create final VMX configuration file #
################################################


# check for vmdk file marker %VM_DISK_PATH% and put vmdk path in it's place: also VM_DISK_MODE
# and VM_DISK_REDOLOGDIR.
replace_placeholders() {
	if [ -n "$VM_DISKFILE_RW" ]; then
		sed -i 's#%VM_DISK_PATH%#'"$VM_DISKFILE_RW"'#g' "${TMPCONFIG}"
		sed -i 's#%VM_DISK_MODE%#'"independent-persistent"'#g' "${TMPCONFIG}"
	else
		sed -i 's#%VM_DISK_PATH%#'"$VM_DISKFILE_RO"'#g' "${TMPCONFIG}"
		sed -i 's#%VM_DISK_MODE%#'"independent-nonpersistent"'#g' "${TMPCONFIG}"
	fi
	sed -i 's#%VM_DISK_REDOLOGDIR%#'"$VM_REDO_DIR"'#g' "${TMPCONFIG}"
	# Need this for CoW to work in dnbd3-fuse mount dir
	echo 'disk.locking = "FALSE"' >> "$TMPCONFIG"
}

# Ethernet: All we do is entering a generated MAC, as we do not want to interfere
# in the possible case no networking is wanted.
setup_ethernet() {
	echo 'ethernet0.addressType = "static"'		>> "${TMPCONFIG}"
	echo 'ethernet0.address = "'"${VM_MAC_ADDR}"'"'	>> "${TMPCONFIG}"
	if [ -n "$OTHER_NICS" ]; then
		local devtype mac cnt eth
		# Copy type of eth0, in case the others don't specify a type in the vmx, we'll use the same
		# so we know the drivers are there
		devtype=$(< "${TMPCONFIG}"  grep -m1 -i '^ethernet0\.virtualDev' | sed -r 's/^[^=]+//;s/^(\s|=|")*//;s/(\s|")+$//') # TODO: Make a helper like "getVmxOption"
		cnt=0
		for mac in $OTHER_NICS; do
			vmnet=$(( cnt + 10 ))
			cnt=$(( cnt + 1 ))
			eth="ethernet${cnt}"
			# Check device exists
			if ! [ -c "/dev/vmnet${vmnet}" ]; then
				slxlog "vmware-bridge-setup" "Cannot bridge NIC into VM: /dev/vmnet${vmnet} does not exist"
				continue
			fi
			# Remove lines that could break things
			sed -i "/^${eth}\.connectionType/Id;/^${eth}\.present/Id" "${TMPCONFIG}"
			# Set up devType
			if ! grep -q -i "^${eth}\.virtualDev" "${TMPCONFIG}" && [ -n "$devtype" ]; then
				devline="${eth}.virtualDev = "'"'"${devtype}"'"'
				echo "${devline}" >> "${TMPCONFIG}"
			fi
			cat >> "${TMPCONFIG}" <<-HEND
			${eth}.connectionType = "custom"
			${eth}.present = "TRUE"
			${eth}.vnet = "vmnet${vmnet}"
			${eth}.addressType = "static"
			${eth}.address = "${mac}"
			HEND
		done
	fi
}

# DVD, CDROM
setup_optical_drives() {
	# XXX: For now it's safe to assume ide channel 1 is free, as we support only one HDD.
	# If it's IDE, it's on channel 0
	sed -i '/^ide1:/d' "${TMPCONFIG}"
	cat >> "${TMPCONFIG}" <<-HEREEND
	ide1:0.present = "$CDROM0"
	ide1:0.autodetect = "TRUE"
	ide1:0.fileName = "auto detect"
	ide1:0.deviceType = "cdrom-raw"
	ide1:1.present = "$CDROM1"
	ide1:1.autodetect = "TRUE"
	ide1:1.fileName = "auto detect"
	ide1:1.deviceType = "cdrom-raw"
	HEREEND
}

setup_floppies() {
	local floppy_bool
	if isset floppy_img; then
		floppy_bool="TRUE"
	else
		floppy_bool="FALSE"
	fi
	(
	# Floppies:
	cat <<-HEREEND
		floppy0.present = "TRUE"
		floppy0.startConnected = "$FLOPPY0"
		floppy0.fileType = "device"
	HEREEND
	if [ -b "$FLOPPY_0" ]; then
		cat <<-HEREEND
			floppy0.autodetect = "FALSE"
			floppy0.fileName = "$FLOPPY_0"
		HEREEND
	else
		cat <<-HEREEND
			floppy0.autodetect = "TRUE"
			floppy0.fileName = "auto detect"
		HEREEND
	fi
	cat <<-HEREEND
		floppy1.present = "$floppy_bool"
		floppy1.startConnected = "TRUE"
		floppy1.fileType = "file"
		floppy1.fileName = "$floppy_img"
	HEREEND
	) >> "$TMPCONFIG"
}

setup_serial() {
	# Serial port
	if [ -n "$SERIAL0" ]; then
		cat >> "${TMPCONFIG}" <<-HEREEND
			serial0.present = "TRUE"
			serial0.startConnected = "TRUE"
			serial0.fileType = "device"
			serial0.fileName = "$SERIAL0"
			serial0.tryNoRxLoss = "FALSE"
			serial0.autodetect = "FALSE"
		HEREEND
	fi

	# Parallel port
	if [ -n "$PARALLEL0" ]; then
		cat >> "${TMPCONFIG}" <<-HEREEND
			parallel0.present = "TRUE"
			parallel0.startConnected = "TRUE"
			parallel0.fileType = "device"
			parallel0.fileName = "$PARALLEL0"
			parallel0.bidirectional = "TRUE"
			parallel0.autodetect = "FALSE"
		HEREEND
	fi
}

# CPU and RAM
setup_vcpu_ram() {
	#writelog "numvcpus = ${CPU_CORES} - maxvcpus=${HOST_CORE_COUNT}"
	if (( VM_HW_VERSION < 18 && CPU_CORES > 16 )); then
		writelog "Limiting VM to 16 cores since hwVersion < 18"
		CPU_CORES=16
	elif (( CPU_CORES > 32 )); then
		writelog "Limiting VM to 32 cores since it's the current maximum VMware supports"
		CPU_CORES=32
	fi
	if (( VM_HW_VERSION < 18 && VM_MEM > 65536 )); then
		writelog "Limiting VM to 64GB since hwVersion < 18"
		VM_MEM=65536
	elif (( VM_MEM > 131072 )); then
		writelog "Limiting VM to 128GB since it's the current maximum VMware supports"
		VM_MEM=131072
	fi
	cat >> "${TMPCONFIG}" <<-HEREEND
	numvcpus = "$CPU_CORES"
	cpuid.coresPerSocket = "$VM_CORES_PER_SOCKET"
	maxvcpus = "$HOST_CORE_COUNT"
	memsize = "${VM_MEM}"
	MemAllowAutoScaleDown = "FALSE"
	MemTrimRate = "-1"
	HEREEND
}

setup_usb() {
	# Force autoconnect
	sed -i '/^usb\.generic\.autoconnect/Id' "${TMPCONFIG}"
	echo 'usb.generic.autoconnect = "TRUE"' >> "${TMPCONFIG}"
	echo 'usb.mangleUsb3Speed = "TRUE"' >> "${TMPCONFIG}"
	# Remove stray autoconnect entries
	sed -i '/^usb\.autoConnect\.device/Id' "${TMPCONFIG}"

	# Workaround for old VMs
	if grep -q -i '^ehci\.present.*TRUE' "${TMPCONFIG}" \
		&& ! grep -q -i '^usb\.present' "${TMPCONFIG}"; then
		echo 'usb.present = "TRUE"' >> "${TMPCONFIG}"
	fi

	# See if there are any USB devices connected that we want to pass through immediately
	get_usb_devices 'usb.autoConnect.deviceXXXXX = "0x%VENDOR%:0x%PRODUCT%"' \
		| sed -r 's/0x0+/0x/g' \
		| awk '{sub(/XXXXX/,NR-1)}1' \
		>> "${TMPCONFIG}"
}

setup_shared_folders() {
	if ! notempty SHARED_FOLDERS HOME_SHARE_PATH HOME_SHARE_NAME COMMON_SHARE_PATH COMMON_SHARE_NAME; then
		writelog "Missing information to setup shared folders."
		return 1
	fi

	cat >> "${TMPCONFIG}" <<-HEREEND
	sharedFolder.option = "alwaysEnabled"
	sharedFolder0.present = "$SHARED_FOLDERS"
	sharedFolder0.enabled = "$SHARED_FOLDERS"
	sharedFolder0.expiration = "never"
	sharedFolder0.guestName = "$HOME_SHARE_NAME"
	sharedFolder0.hostPath = "$HOME_SHARE_PATH"
	sharedFolder0.readAccess = "TRUE"
	sharedFolder0.writeAccess = "TRUE"
	sharedFolder1.present = "$SHARED_FOLDERS"
	sharedFolder1.enabled = "$SHARED_FOLDERS"
	sharedFolder1.expiration = "never"
	sharedFolder1.guestName = "$COMMON_SHARE_NAME"
	sharedFolder1.hostPath = "$COMMON_SHARE_PATH"
	sharedFolder1.readAccess = "TRUE"
	sharedFolder1.writeAccess = "FALSE"
	sharedFolder.maxNum = "2"
	hgfs.mapRootShare = "TRUE"
	hgfs.linkRootShare = "TRUE"
	HEREEND
}

setup_isolation() {
	# Settings for isolation tools (drag & drop, copy & paste, etc...)
	cat >> "${TMPCONFIG}" <<-HEREEND
	isolation.tools.hgfs.disable = "$HGFS_DISABLED"
	isolation.tools.dnd.disable = "FALSE"
	isolation.tools.copy.enable = "TRUE"
	isolation.tools.paste.enabled = "TRUE"
	HEREEND
}

setup_vnc() {
	local passwd="$( cat "/tmp/remote-access-$DISPLAY/vnc-passwd" )"
	[ -z "$passwd" ] && return
	cat >> "${TMPCONFIG}" <<-HEREEND
	RemoteDisplay.vnc.enabled = "TRUE"
	RemoteDisplay.vnc.port = "5901"
	RemoteDisplay.vnc.password = "$passwd"
	RemoteDisplay.vnc.keymap = "de"
	HEREEND
}

# Serial, parallel: Empty, nothing is being currently set. TODO later.

setup_graphics() {
	# Force 3D on every gpu
	echo 'mks.gl.allowBlacklistedDrivers = "TRUE"'	>> "${TMPCONFIG}"
	
	# Disable DPI scaling information passing via vmware tools
	sed -i '/^gui.applyHostDisplayScaling/Id' "${TMPCONFIG}"
	echo 'gui.applyHostDisplayScalingToGuest = "FALSE"' >> "${TMPCONFIG}"
	
	# Additinal exam mode settings
	if [ -n "$SLX_EXAM" ]; then
		echo 'gui.restricted = "true"' >> "${TMPCONFIG}"
	fi
	
	# Hack resolution if we know the desired one is not in the default list of vmx_svga
	# For now, only do it on the odd ones, as we don't know if this has any side effects
	# This seems unnecessary on Win7 but is required on WinXP - need more research for other OSs
	case "$RESOLUTION" in
		1600x900|2560x1080|2560x1440|2880x1800|3200x1800|3440x1440)
			X=${RESOLUTION%x*}
			Y=${RESOLUTION#*x}
			BYTES=$(( ( ( X * Y * 4  + 65535 ) / 65536 ) * 65536 ))
			[ "$BYTES" -lt 16777216 ] && BYTES=16777216
			cat >> "${TMPCONFIG}" <<-EOF
			svga.autodetect = "FALSE"
			svga.vramSize = $BYTES
			svga.maxWidth = $X
			svga.maxHeight = $Y
			EOF
			;;
	esac
	if (( VM_HW_VERSION >= 11 )); then
		# Pimp VGPU memory if not set, or restrict if too large
		local vram max_vram exists
		# VM_MEM in MB
		max_vram="$(( (VM_MEM / 4) * 1024 ))"
		(( max_vram > 2097152 )) && max_vram=2097152
		vram=$( < "${TMPCONFIG}"  grep -m1 -i '^svga\.graphicsMemoryKB' \
			| sed -r 's/^[^=]+//;s/^(\s|=|")*//;s/(\s|")+$//' )
		existing="$vram"
		if [ -z "$vram" ]; then # Nothing, default to third of max
			vram="$(( max_vram / 3 ))"
		elif (( vram > max_vram )); then # Explicitly set, cap if too excessive
			vram="$max_vram"
		fi
		(( vram < 131072 )) && vram=131072
		# Subtract half of VRAM from main RAM to be safe
		VM_MEM="$(( VM_MEM - ( (vram / 2048) / 4 ) * 4 ))"
		if [ -z "$existing" ]; then
			echo 'svga.graphicsMemoryKB = "'"$vram"'"' >> "${TMPCONFIG}"
		else
			sed -i 's/^svga\.graphicsMemoryKB.*$/svga.graphicsMemoryKB = "'"$vram"'"/' "${TMPCONFIG}"
		fi
	fi
	if lsmod | grep -q ^nvidia; then
		echo 'mks.enableVulkanPresentation = "FALSE"' >> "${TMPCONFIG}"
		echo 'mks.vk.gpuHeapSizeMB = "0"' >> "${TMPCONFIG}"
	fi
}

finalize_hardware() {
	# Enable nested virtualization if not specified in remote vmx
	if [ -e "/run/openslx/hwinfo" ] && ! grep -qi '^vhv\.enable' "${TMPCONFIG}" \
			&& detect_cpu_flag "ept" "npt" \
			&& [ "$VM_HW_VERSION" -ge "9" ]; then
		. "/run/openslx/hwinfo"
		[ "${HW_KVM}" = "ENABLED" ] && echo 'vhv.enable = "TRUE"' >> "${TMPCONFIG}"
	fi

	# Disable space check warnings
	sed -i '/^mainMem.freeSpaceCheck/Id' "${TMPCONFIG}"
	echo 'mainMem.freeSpaceCheck = "FALSE"' >> "${TMPCONFIG}"
	# Need this to avoid VMware requiring as much free space in /tmp/virt as we have vmem
	# and no swap. Should never happen because of zram swap, but hey...
	sed -i '/^mainMem.backing/Id' "${TMPCONFIG}"
	echo 'mainMem.backing = "swap"' >> "${TMPCONFIG}"

	# Fix keymap fuckup when leaving VM
	sed -i '/^mks.x.resetModMap/Id' "${TMPCONFIG}"
	echo 'mks.x.resetModMap = "FALSE"' >> "${TMPCONFIG}"
	# Fix key-repeat getting stuck on VMware 17.5.x
	echo 'keyboard.allowBothIRQs = "FALSE"' >> "${TMPCONFIG}"

	# TODO: Need a way to check if supported by hardware before enabling!
	#grep -qi '^vpmc\.enable' "${TMPCONFIG}" || echo 'vpmc.enable = "TRUE"' >> "${TMPCONFIG}"
}


## MAIN ##
write_final_vmx() {
	# Apply $maxhardwareversion to final VMX
	if notempty VM_HW_VERSION && [ "$VM_HW_VERSION" -gt "$maxhardwareversion" ]; then
		writelog "Hardware version capped to $maxhardwareversion (was $VM_HW_VERSION)"
		sed -i 's/^virtualHW\.version.*$/virtualHW.version = "'$maxhardwareversion'"/I' "${TMPCONFIG}"
		VM_HW_VERSION="$maxhardwareversion"
	fi

	replace_placeholders
	setup_ethernet
	setup_optical_drives
	setup_floppies
	setup_serial
	setup_graphics # Graphics might reduce available VRAM
	setup_vcpu_ram
	setup_usb
	[ "x$HGFS_DISABLED" = "xFALSE" ] && setup_shared_folders
	setup_isolation
	[[ " $SLX_REMOTE_VNC " == *" vmware "* ]] && setup_vnc
	finalize_hardware

	# Killing duplicate entries; always keep last value:
	awk -F= 'NF{
		b = $0;
		gsub(/^[ \t]+|[ \t]+$/, "", $1);
		gsub(/^[ \t]*[^ \t]+[ \t]*=[ \t]*|[ \t]+$/, "", b);
		a[$1] = b;
	}
	END {
		for (k in a) printf "%s = %s\n", k, a[k];
	}' "${TMPCONFIG}" > "${TMPCONFIG}.tmp" \
		&& sort "${TMPCONFIG}.tmp" > "${TMPCONFIG}"

	# At last: Let's copy it to $VM_CONF_DIR/run-vmware.conf
	if cp -p "${TMPCONFIG}" "${VM_RUN_FILE}"; then
		writelog "Copied '${TMPCONFIG}' to '${VM_RUN_FILE}'"
	else
		writelog "Could not copy TMPDIR/IMGUUID -${TMPCONFIG}- to VM_RUN_FILE ${VM_RUN_FILE}!"
		# cleanexit 1 # that seems not needed!
	fi
}
call_post_source write_final_vmx


