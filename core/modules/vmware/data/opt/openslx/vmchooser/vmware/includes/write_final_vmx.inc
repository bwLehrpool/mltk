
# check for vmdk file marker %VM_DISK_PATH% and put vmdk path in it's place: also VM_DISK_MODE
# and VM_DISK_REDOLOGDIR.
sed -i 's#%VM_DISK_PATH%#'"$VM_DISKFILE_RO"'#g' "${TMPCONFIG}"
sed -i 's#%VM_DISK_MODE%#'"independent-nonpersistent"'#g' "${TMPCONFIG}"
sed -i 's#%VM_DISK_REDOLOGDIR%#'"$redodir"'#g' "${TMPCONFIG}"

# Ethernet: All we do is entering a generated MAC, as we do not want to interfere
# in the possible case no networking is wanted.
writelog "Guest MAC: $macaddr"
echo 'ethernet0.addressType = "static"'		>> "${TMPCONFIG}"
echo 'ethernet0.address = "'"${macaddr}"'"'	>> "${TMPCONFIG}"

# DVD, CDROM
# XXX: For now it's safe to assume ide channel 1 is free, as we support only one HDD, and if it's IDE, it's on channel 0
cat >> "${TMPCONFIG}" <<-HEREEND
ide1:0.present = "$cdrom0"
ide1:0.autodetect = "TRUE"
ide1:0.fileName = "auto detect"
ide1:0.deviceType = "cdrom-raw"
ide1:1.present = "$cdrom1"
ide1:1.autodetect = "TRUE"
ide1:1.fileName = "auto detect"
ide1:1.deviceType = "cdrom-raw"
HEREEND

if [ -n "$FLOPPYIMG" ]; then
	floppy1="TRUE"
else
	floppy1="FALSE"
fi

# Floppies:
cat >> "${TMPCONFIG}" <<-HEREEND
floppy0.present = "$floppy0"
floppy0.startConnected = "FALSE"
floppy0.autodetect = "TRUE"
floppy0.fileName = "auto detect"
floppy1.present = "$floppy1"
floppy1.startConnected = "TRUE"
floppy1.fileType = "file"
floppy1.fileName = "$FLOPPYIMG"
HEREEND

writelog "numvcpus = ${cpu_cores} - maxvcpus=${real_core_count}"

# RAM, CPUs
cat >> "${TMPCONFIG}" <<-HEREEND
numvcpus = "$cpu_cores"
cpuid.coresPerSocket = "$cores_per_socket"
maxvcpus = "$real_core_count"
memsize = "$mem"
MemAllowAutoScaleDown = "FALSE"
MemTrimRate = "-1"
HEREEND

# USB fallback: Only write usb config if there's none
if ! grep -q -i "^usb\.present" "${TMPCONFIG}"; then
	# Nothing found, go ahead
	if [ -n "$SLX_EXAM" ]; then
		# Exam mode: Default to no USB
		sed -i '/^usb\./Id' "${TMPCONFIG}"
		echo 'usb.present = "FALSE"' >> "${TMPCONFIG}"
	else
		cat >> "${TMPCONFIG}" <<-HEREEND
		usb.present = "TRUE"
		HEREEND
	fi
fi
sed -i '/^usb\.generic\.autoconnect/Id' "${TMPCONFIG}"
echo 'usb.generic.autoconnect = "TRUE"' >> "${TMPCONFIG}"
# USB 3.0 support changes quality and has different side effects
# with every minor release of vmware. Always force 2.0 for now.
# TODO: Get it fixed by vmware?
sed -i '/^ehci\.present/Id;/^usb_xhci\.present/Id' "${TMPCONFIG}"
echo 'ehci.present = "TRUE"' >> "${TMPCONFIG}"

# shared folders
if [ "$HGFS_DISABLED" = "FALSE" ]; then
	cat >> "${TMPCONFIG}" <<-HEREEND
	sharedFolder.option = "alwaysEnabled"
	sharedFolder0.present = "$shfolders"
	sharedFolder0.enabled = "$shfolders"
	sharedFolder0.expiration = "never"
	sharedFolder0.guestName = "$homesharename"
	sharedFolder0.hostPath = "$homesharepath"
	sharedFolder0.readAccess = "TRUE"
	sharedFolder0.writeAccess = "TRUE"
	sharedFolder1.present = "$shfolders"
	sharedFolder1.enabled = "$shfolders"
	sharedFolder1.expiration = "never"
	sharedFolder1.guestName = "$commonsharename"
	sharedFolder1.hostPath = "$commonsharepath"
	sharedFolder1.readAccess = "TRUE"
	sharedFolder1.writeAccess = "FALSE"
	sharedFolder.maxNum = "2"
	hgfs.mapRootShare = "TRUE"
	hgfs.linkRootShare = "TRUE"
	HEREEND
fi

# Isolation tools: settings
cat >> "${TMPCONFIG}" <<-HEREEND
isolation.tools.hgfs.disable = "$HGFS_DISABLED"
isolation.tools.dnd.disable = "FALSE"
isolation.tools.copy.enable = "TRUE"
isolation.tools.paste.enabled = "TRUE"
HEREEND

# Serial port
if [ -n "$serial_0" ]; then
	cat >> "${TMPCONFIG}" <<-HEREEND
	serial0.present = "TRUE"
	serial0.startConnected = "TRUE"
	serial0.fileType = "device"
	serial0.fileName = "$serial_0"
	serial0.tryNoRxLoss = "FALSE"
	serial0.autodetect = "FALSE"
	HEREEND
fi

# Parallel port
if [ -n "$parallel_0" ]; then
	cat >> "${TMPCONFIG}" <<-HEREEND
	parallel0.present = "TRUE"
	parallel0.startConnected = "TRUE"
	parallel0.fileType = "device"
	parallel0.fileName = "$parallel_0"
	parallel0.bidirectional = "TRUE"
	parallel0.autodetect = "FALSE"
	HEREEND
fi

# Graphics, GPU: 3D will be enabled (even if vmware doesn't support the chip) if we whitelisted it.
if [ -n "$SLX_VMWARE_3D" ]; then
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

# Apply $maxhardwareversion to final VMX
if [ -n "$HWVER" ] && [ "$HWVER" -gt "$maxhardwareversion" ]; then
	writelog "Hardware version capped to $maxhardwareversion (was $HWVER)"
	sed -i 's/^virtualHW\.version.*$/virtualHW.version = "'$maxhardwareversion'"/I' "${TMPCONFIG}"
	HWVER="$maxhardwareversion"
fi

# Enable nested virtualization if not specified in remote vmx
if [ -e "/run/hwinfo" ] && ! grep -qi '^vhv\.enable' "${TMPCONFIG}" \
		&& grep -qE '^flags\s*:.*\b(ept|npt)\b' "/proc/cpuinfo" \
		&& [ "$HWVER" -ge "9" ]; then
	. "/run/hwinfo"
	[ "${HW_KVM}" = "ENABLED" ] && echo 'vhv.enable = "TRUE"' >> "${TMPCONFIG}"
fi
# TODO: Need a way to check if supported by hardware before enabling!
#grep -qi '^vpmc\.enable' "${TMPCONFIG}" || echo 'vpmc.enable = "TRUE"' >> "${TMPCONFIG}"

# Disable space check warnings
sed -i '/^mainMem.freeSpaceCheck/Id' "${TMPCONFIG}"
echo 'mainMem.freeSpaceCheck = "FALSE"' >> "${TMPCONFIG}"

# See if there are any USB devices connected that we want to pass through immediately
get_usb_devices 'usb.autoConnect.deviceXXXXX = "0x%VENDOR%:0x%PRODUCT%"' \
	| sed -r 's/0x0+/0x/g' \
	| awk '{sub(/XXXXX/,NR-1)}1' \
	>> "${TMPCONFIG}"

# At last_ Let's copy it to $confdir/run-vmware.conf
if cp -p "${TMPCONFIG}" "$conffile"; then
	writelog "Copied TMPDIR/IMGUUID ${TMPCONFIG} to conffile ${conffile}"
else
	writelog "Could not copy TMPDIR/IMGUUID -${TMPCONFIG}- to conffile ${conffile}!"
	cleanexit 1
fi

