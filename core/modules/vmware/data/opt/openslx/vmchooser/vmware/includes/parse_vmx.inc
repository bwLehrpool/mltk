######################################################################
# Include: Parsing the non-legacy delivered vmxfile $TMPDIR/$IMGUUID #
######################################################################

writelog "Parsing virtual machine description file..."

# check for vmdk file marker %VM_DISK_PATH% and put vmdk path in it's place: also VM_DISK_MODE
# and VM_DISK_REDOLOGDIR.
# IMAGE=$(grep -io '<image_path.*"' "${xmlfile}" | sed -e "s/&.*;/; /g" | awk -F '"' '{ print $2 }')
sed -i 's#%VM_DISK_PATH%#'"$vm_diskfile"'#g' "$TMPDIR/$IMGUUID"
sed -i 's#%VM_DISK_MODE%#'"independent-nonpersistent"'#g' "$TMPDIR/$IMGUUID"
sed -i 's#%VM_DISK_REDOLOGDIR%#'"$redodir"'#g' "$TMPDIR/$IMGUUID"

# Ethernet: All we do is entering a generated MAC, as we do not want to interfere
# in the possible case no networking is wanted.
writelog "Guest MAC: $macaddr"
echo 'ethernet0.addressType = "static"'		>> "$TMPDIR/$IMGUUID"
echo 'ethernet0.address = "'"${macaddr}"'"'	>> "$TMPDIR/$IMGUUID"

# DVD, CDROM
# XXX: For now it's save to assume ide channel 1 is free, as we support only one HDD, and it it's IDE, it's on channel 0
cat >> "$TMPDIR/$IMGUUID" <<-HEREEND
ide1:0.present = "$cdrom0"
ide1:0.autodetect = "TRUE"
ide1:0.fileName = "auto detect"
ide1:0.deviceType = "cdrom-raw"
ide1:1.present = "$cdrom1"
ide1:1.autodetect = "TRUE"
ide1:1.fileName = "auto detect"
ide1:1.deviceType = "cdrom-raw"
HEREEND

# Floppies:
cat >> "$TMPDIR/$IMGUUID" <<-HEREEND
floppy0.present = "$floppy0"
floppy0.startConnected = "FALSE"
floppy0.autodetect = "TRUE"
floppy0.fileName = "auto detect"
floppy1.present = "$floppy1"
floppy1.startConnected = "TRUE"
floppy1.fileType = "file"
floppy1.fileName = "$floppy1name"
HEREEND

writelog "numvcpus = ${cpu_cores} - maxvcpus=${real_core_count}"

# RAM, CPUs
cat >> "$TMPDIR/$IMGUUID" <<-HEREEND
numvcpus = "$cpu_cores"
cpuid.coresPerSocket = "$cores_per_socket"
maxvcpus = "$real_core_count"
memsize = "$mem"
MemAllowAutoScaleDown = "FALSE"
MemTrimRate = "-1"
HEREEND

# USB
if [ -n "$SLX_EXAM" ]; then
	# Exam mode: No USB (TODO: Configurable)
	sed -i '/^usb\./d' "$TMPDIR/$IMGUUID"
	echo 'usb.present = "FALSE"' >> "$TMPDIR/$IMGUUID"
else
	cat >> "$TMPDIR/$IMGUUID" <<-HEREEND
	usb.present = "TRUE"
	usb.generic.autoconnect = "TRUE"
	HEREEND
fi

# shared folders
cat >> "$TMPDIR/$IMGUUID" <<-HEREEND
$ENABLE_SHARE
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

# Isolation tools: settings
# It should be verified whether and when entries necessary
cat >> "$TMPDIR/$IMGUUID" <<-HEREEND
isolation.tools.hgfs.disable = "FALSE"
isolation.tools.dnd.disable = "FALSE"
isolation.tools.copy.enable = "TRUE"
isolation.tools.paste.enabled = "TRUE"
HEREEND

# Serial, parallel: Empty, nothing is being currently set. TODO later.

# Graphics, GPU: 3D will be enabled (even if vmware doesn't support the chip) if we whitelisted it.
if [ -n "$SLX_VMWARE_3D" ]; then
	writelog "FORCE3D set - overriding 3D in vmx file."
	echo 'mks.gl.allowBlacklistedDrivers = "TRUE"'	>> "$TMPDIR/$IMGUUID"
	# We override... play safe and cap the hwVersion to 10, since some i915 chips goofed up with 12
	# Investigate if we might have to do this in other cases where we don't override
	if grep -qi '^mks.enable3d.*true' "$TMPDIR/$IMGUUID"; then
		vmw_cap_hw_version "10"
	fi
else
	writelog "FORCE3D not set - 3D will only work if GPU/driver is whitelisted by vmware."
fi

# Disable DPI scaling information passing via vmware tools
sed -i '/^gui.applyHostDisplayScaling/d' "$TMPDIR/$IMGUUID"
echo 'gui.applyHostDisplayScalingToGuest = "FALSE"' >> "$TMPDIR/$IMGUUID"

# Additinal exam mode settings
if [ -n "$SLX_EXAM" ]; then
	echo 'gui.restricted = "true"' >> "$TMPDIR/$IMGUUID"
fi

# Killing duplicate lines (output much nicer than sort -u):
awk '!a[$0]++' "${TMPDIR}/${IMGUUID}" > "${TMPDIR}/${IMGUUID}.tmp" && mv "${TMPDIR}/${IMGUUID}.tmp" "${TMPDIR}/${IMGUUID}"

# At last_ Let's copy it to $confdir/run-vmware.conf
cp -p "$TMPDIR/$IMGUUID" "$conffile" && writelog "Copied TMPDIR/IMGUUID ${TMPDIR}/${IMGUUID} to conffile ${conffile}" || \
	( writelog "Could not copy TMPDIR/IMGUUID -$TMPDIR/$IMGUUID- to conffile ${conffile}!"; cleanexit 1 )

