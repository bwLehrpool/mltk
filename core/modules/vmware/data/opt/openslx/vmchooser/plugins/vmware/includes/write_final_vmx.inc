#!/bin/bash
################################################
# Include: Create final VMX configuration file #
################################################


# check for vmdk file marker %VM_DISK_PATH% and put vmdk path in it's place: also VM_DISK_MODE
# and VM_DISK_REDOLOGDIR.
replace_placeholders() {
	sed -i 's#%VM_DISK_PATH%#'"$VM_DISKFILE_RO"'#g' "${TMPCONFIG}"
	sed -i 's#%VM_DISK_MODE%#'"independent-nonpersistent"'#g' "${TMPCONFIG}"
	sed -i 's#%VM_DISK_REDOLOGDIR%#'"$VM_REDO_DIR"'#g' "${TMPCONFIG}"
}

# Ethernet: All we do is entering a generated MAC, as we do not want to interfere
# in the possible case no networking is wanted.
setup_ethernet() {
	echo 'ethernet0.addressType = "static"'		>> "${TMPCONFIG}"
	echo 'ethernet0.address = "'"${VM_MAC_ADDR}"'"'	>> "${TMPCONFIG}"
}

# DVD, CDROM
setup_optical_drives() {
	# XXX: For now it's safe to assume ide channel 1 is free, as we support only one HDD.
	# If it's IDE, it's on channel 0
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
	local SLX_FLOPPY
	if isset SLX_FLOPPY_IMG; then
		SLX_FLOPPY="TRUE"
	else
		SLX_FLOPPY="FALSE"
	fi

	# Floppies:
	cat >> "${TMPCONFIG}" <<-HEREEND
	floppy0.present = "$FLOPPY0"
	floppy0.startConnected = "FALSE"
	floppy0.autodetect = "TRUE"
	floppy0.fileName = "auto detect"
	floppy1.present = "$SLX_FLOPPY"
	floppy1.startConnected = "TRUE"
	floppy1.fileType = "file"
	floppy1.fileName = "$SLX_FLOPPY_IMG"
	HEREEND
}

# CPU and RAM
setup_vcpu_ram() {
	#writelog "numvcpus = ${CPU_CORES} - maxvcpus=${HOST_CORE_COUNT}"
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
	# USB fallback: Only write usb config if there's none
	if ! grep -q -i "^usb\.present" "${TMPCONFIG}"; then
		# Nothing found, go ahead
		if [ -n "$SLX_EXAM" ]; then # TODO better handling of exam mode...
			# Exam mode: Default to no USB
			sed -i '/^usb\./Id' "${TMPCONFIG}"
			echo 'usb.present = "FALSE"' >> "${TMPCONFIG}"
		else
			echo 'usb.present = "TRUE"' >> "${TMPCONFIG}"
		fi
	fi
	sed -i '/^usb\.generic\.autoconnect/Id' "${TMPCONFIG}"
	echo 'usb.generic.autoconnect = "TRUE"' >> "${TMPCONFIG}"

	# USB 3.0 support changes quality and has different side effects
	# with every minor release of vmware. Always force 2.0 for now.
	# TODO: Get it fixed by vmware?
	sed -i '/^ehci\.present/Id;/^usb_xhci\.present/Id' "${TMPCONFIG}"
	echo 'ehci.present = "TRUE"' >> "${TMPCONFIG}"

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

# Serial, parallel: Empty, nothing is being currently set. TODO later.

setup_graphics() {
	# Graphics, GPU: 3D will be enabled (even if vmware doesn't support the chip) if we whitelisted it.
	if isset SLX_VMWARE_3D; then
		writelog "FORCE3D set - overriding 3D in vmx file."
		echo 'mks.gl.allowBlacklistedDrivers = "TRUE"'	>> "${TMPCONFIG}"
		# We override... play safe and cap the hwVersion to 10, since some i915 chips goofed up with 12
		# Investigate if we might have to do this in other cases where we don't override
		if grep -qi '^mks.enable3d.*true' "${TMPCONFIG}"; then
			vmw_cap_hw_version "10"
		fi
	else
		writelog "FORCE3D not set - 3D will only work if GPU/driver is whitelisted by vmware."
	fi
	
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
		1600x900|2560x1440|2880x1800|3200x1800)
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
	
	# Killing duplicate lines (output much nicer than sort -u):
	awk '!a[$0]++' "${TMPCONFIG}" > "${TMPCONFIG}.tmp" && mv -f "${TMPCONFIG}.tmp" "${TMPCONFIG}"
}

finalize_hardware() {
	# Apply $maxhardwareversion to final VMX
	if notempty VM_HW_VERSION && [ "$VM_HW_VERSION" -gt "$maxhardwareversion" ]; then
		writelog "Hardware version capped to $maxhardwareversion (was $VM_HW_VERSION)"
		sed -i 's/^virtualHW\.version.*$/virtualHW.version = "'$maxhardwareversion'"/I' "${TMPCONFIG}"
		VM_HW_VERSION="$maxhardwareversion"
	fi

	# Enable nested virtualization if not specified in remote vmx
	if [ -e "/run/hwinfo" ] && ! grep -qi '^vhv\.enable' "${TMPCONFIG}" \
			&& grep -qE '^flags\s*:.*\b(ept|npt)\b' "/proc/cpuinfo" \
			&& [ "$VM_HW_VERSION" -ge "9" ]; then
		. "/run/hwinfo"
		[ "${HW_KVM}" = "ENABLED" ] && echo 'vhv.enable = "TRUE"' >> "${TMPCONFIG}"
	fi

	# Disable space check warnings
	sed -i '/^mainMem.freeSpaceCheck/Id' "${TMPCONFIG}"
	echo 'mainMem.freeSpaceCheck = "FALSE"' >> "${TMPCONFIG}"

	# TODO: Need a way to check if supported by hardware before enabling!
	#grep -qi '^vpmc\.enable' "${TMPCONFIG}" || echo 'vpmc.enable = "TRUE"' >> "${TMPCONFIG}"
}


## MAIN ##
write_final_vmx() {
	replace_placeholders
	setup_ethernet
	setup_optical_drives
	setup_floppies
	setup_vcpu_ram
	setup_usb
	[ "x$HGFS_DISABLED" = "xFALSE" ] && setup_shared_folders
	setup_isolation
	setup_graphics
	finalize_hardware

	# At last: Let's copy it to $VM_CONF_DIR/run-vmware.conf
	if cp -p "${TMPCONFIG}" "${VM_RUN_FILE}"; then
		writelog "Copied '${TMPCONFIG}' to '${VM_RUN_FILE}'"
	else
		writelog "Could not copy TMPDIR/IMGUUID -${TMPCONFIG}- to VM_RUN_FILE ${VM_RUN_FILE}!"
		# cleanexit 1 # that seems not needed!
	fi
}
call_post_source write_final_vmx


