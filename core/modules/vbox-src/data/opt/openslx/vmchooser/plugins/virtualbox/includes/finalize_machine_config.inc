################################################################################
# Include: write final machine configuration file                              #
################################################################################

setup_disk_image() {
	# set disk image path and uuid
	set_attr \
		"/VirtualBox/Machine/MediaRegistry/HardDisks/HardDisk" \
		"location" "${VBOX_HDD_LINK}"
	set_attr \
		"/VirtualBox/Machine/MediaRegistry/HardDisks/HardDisk" \
		"uuid" "{${HDD_UUID}}"
	
	if isset SNAPSHOT_UUID; then
		# read-only mode, add snapshot to main disk
		add_node \
			"/VirtualBox/Machine/MediaRegistry/HardDisks/HardDisk" "HardDisk" \
			"uuid={${SNAPSHOT_UUID}}" \
			"location=$VBOX_SNAPSHOT_DIR/{${SNAPSHOT_UUID}}.vdi" \
			"format=VDI" \
			"type=Normal"

		# set snapshot dir
		set_attr "/VirtualBox/Machine" "snapshotFolder" "${VBOX_SNAPSHOT_DIR}"
	fi

	# "attach" VBOX_HDD_UUID (either the snapshot's or the disk's)
	# NOTE: exactly one <AttachedDevice> node should be present in the downloaded xml
	set_attr \
		"/VirtualBox/Machine//StorageControllers/StorageController/AttachedDevice/Image" \
		"uuid" "{${VBOX_HDD_UUID}}"
}

setup_floppies() {
	# add storage controller and 2 floppies to it
	if ! node_exists '/VirtualBox/Machine//StorageControllers/StorageController[@name="Floppy"]'; then
		add_node \
			"/VirtualBox/Machine//StorageControllers" "StorageController" \
			"name=Floppy" \
			"type=I82078" \
			"PortCount=1" \
			"useHostIOCache=true"
	fi
	add_node \
		'/VirtualBox/Machine//StorageControllers/StorageController[@name="Floppy"]' "AttachedDevice" \
		"type=Floppy" \
		"hotpluggable=false" \
		"port=0" \
		"device=0"
	if notempty FLOPPY_0; then
		add_node \
			'/VirtualBox/Machine//StorageControllers/StorageController[@name="Floppy"]/AttachedDevice[@device="0"]' \
			"HostDrive" \
			"src=${FLOPPY_0}"
	fi
	add_node \
		'/VirtualBox/Machine//StorageControllers/StorageController[@name="Floppy"]' "AttachedDevice" \
		"type=Floppy" \
		"hotpluggable=false" \
		"port=0" \
		"device=1"

	# add the slx floppy to the second drive
	add_node \
		'/VirtualBox/Machine//StorageControllers/StorageController/AttachedDevice[@device="1"]' "Image" \
		"uuid={${floppy_uuid}}"
}

setup_optical_drives() {
	if isempty CDROM_0 && isempty CDROM_1; then
		writelog "No host CD-ROM detected."
		return 1
	fi
	local controller='StorageController[@type="AHCI"]'
	local devnum=1
	if node_exists '/VirtualBox/Machine//StorageControllers/'"$controller"; then
		# TODO validating port count...
		writelog "AHCI controller found, will add CDROMs to it."
	else
		writelog "Adding SATA controller for CDROMs."
		add_node \
			"/VirtualBox/Machine//StorageControllers" "StorageController" \
			"name=SATA-CDROM" \
			"type=AHCI" \
			"PortCount=1" \
			"useHostIOCache=true"
		controller='StorageController[@name="SATA-CDROM"]'
		devnum=0
	fi
	# now add the devices
	for cdrom in $CDROM_0 $CDROM_1; do
		writelog "Adding passthrough CDROM"
		add_node \
			'/VirtualBox/Machine//StorageControllers/'"$controller" "AttachedDevice" \
			"type=DVD" \
			"passthrough=true" \
			"hotpluggable=false" \
			"port=1" \
			"device=$devnum"
		(( devnum ++ ))
		add_node \
			'/VirtualBox/Machine//StorageControllers/'"$controller"'/AttachedDevice[@type="DVD"]' "HostDrive" \
			"src=${cdrom}"
	done
}

setup_shared_folders() {
	# TODO common share through shared folders?
	if isempty SHARED_FOLDERS HOME_SHARE_NAME HOME_SHARE_PATH; then
		writelog "Missing information to setup shared folders."
		return 1
	fi
	add_node \
		"/VirtualBox/Machine/Hardware/SharedFolders" "SharedFolder" \
		"name=${HOME_SHARE_NAME}" \
		"hostPath=${HOME_SHARE_PATH}" \
		"writable=true" \
		"autoMount=true"
}

setup_ethernet() {
	# remove ':' from MAC address and set it to the main network adapter
	VM_MAC_ADDR="$(sed 's/://g' <<< ${VM_MAC_ADDR})"
	set_attr "/VirtualBox/Machine/Hardware/Network/Adapter" "MACAddress" "${VM_MAC_ADDR}"

	# TODO bridge other nics
}

setup_cpu_ram() {
	# set vm's memory
	set_attr "/VirtualBox/Machine/Hardware/Memory" "RAMSize" "${VM_MEM}"

	# Shouldn't be needed / should be set do "Default" anyways which does
	# the right thing depending on guest
	#set_attr "/VirtualBox/Machine/Hardware/Paravirt" "provider" "KVM"
	# Let vbox config decide about PAE etc.
	# (PAE is true by default on 64 bit hosts anyways)

	# But explicitly disable acceleration if KVM is not available
	local kvm="false"
	local nested="false"
	local cores="1"
	local ioapic="false"
	if source /run/hwinfo && [ "${HW_KVM}" = "ENABLED" ]; then
		ioapic="true"
		kvm="true"
		cores="$CPU_CORES"
		# KVM is there -- all good, but check for NPT/EPT
		if detect_cpu_flag "npt" || detect_cpu_flag "ept"; then
			nested="true"
		fi
	fi
	set_attr "/VirtualBox/Machine/Hardware/CPU/HardwareVirtEx" "enabled" "$kvm"
	set_attr "/VirtualBox/Machine/Hardware/CPU/HardwareVirtExNestedPaging" "enabled" "$nested"

	# set cpu cores and features
	set_attr "/VirtualBox/Machine/Hardware/CPU" "count" "$cores"

	# activate IOAPIC needed for SMP, but disable for no VT-x machines
	set_attr "/VirtualBox/Machine/Hardware/BIOS/IOAPIC" "enabled" "$ioapic"

	# Don't enable this since it's off by default on Linux due to performance concerns
	# See src/VBox/Main/xml/Settings.cpp Hardware::Hardware()
	# Maybe even explicitly disable this in case the VM was created on 64Bit Windows?
	#set_attr "/VirtualBox/Machine/Hardware/CPU/HardwareVirtExLargePages" "enabled" "${PSE_SUPPORT}"
}

setup_usb() {
	# In case a VBox with USB 3.0 configured doesn't have working USB on
	# machines without a USB 3.0 controller, downgrade to 2.0, but otherwise,
	# USB speed is now dicated via the downloaded VM config
	add_node "/VirtualBox/Machine/Hardware/USB" "DevicesFilters"
	local count=1
	while read -r vendor product stuff; do
		add_node "/VirtualBox/Machine/Hardware/USB/DeviceFilters" "DeviceFilter" \
			"name=filter$count" \
			"active=true" \
			"vendorId=$vendor" \
			"productId=$product"
		(( count++ ))
	done < <(get_usb_devices '%VENDOR% %PRODUCT%')
}

setup_sound() {
	# force ALSA driver for now and change when others become available
	set_attr "/VirtualBox/Machine/Hardware/AudioAdapter" "driver" "ALSA"
}

setup_displays() {
	# even though it's more than enough RAM for screens, just
	# use 128MB for one or 256MB for more screens.
	local vram_size="128"
	if [ "$DISPLAY_COUNT" -ge 2 ]; then
		vram_size="256"
	fi
	set_attr "/VirtualBox/Machine/Hardware/Display" "monitorCount" "${DISPLAY_COUNT}"
	set_attr "/VirtualBox/Machine/Hardware/Display" "VRAMSize" "$vram_size"
	# XXX 3D acceleration is very broken as of 7.0.6 - disable for now.
	# Re-evaluate some time in the future
	set_attr "/VirtualBox/Machine/Hardware/Display" "accelerate3D" "false"
}

finalize_machine_config() {
	# set the generated machine uuid
	set_attr "/VirtualBox/Machine" "uuid" "{${MACHINE_UUID}}"
	setup_usb
	setup_disk_image
	setup_floppies
	setup_optical_drives
	setup_ethernet
	setup_cpu_ram
	setup_sound
	setup_displays
	notempty VBOX_SHARED_FOLDERS && setup_shared_folders

	if ! cp -p "$TMPCONFIG" "$VBOX_MACHINE_CONFIG"; then
		writelog "Could not copy '$TMPCONFIG' to '$VBOX_MACHINE_CONFIG'!"
	fi
}

call_post_source finalize_machine_config
