################################################################################
# Include: write final machine configuration file                              #
################################################################################

setup_disk_image() {
	# set read-only image path and uuid
	set_attr \
		"/VirtualBox/Machine/MediaRegistry/HardDisks/HardDisk" \
		"location" "${VBOX_HDD_LINK}"
	set_attr \
		"/VirtualBox/Machine/MediaRegistry/HardDisks/HardDisk" \
		"uuid" "{${HDD_UUID}}"
	

	# add a HardDisk node for the snapshot
	add_node \
		"/VirtualBox/Machine/MediaRegistry/HardDisks/HardDisk" "HardDisk" \
		"uuid={${SNAPSHOT_UUID}}" \
		"location=$VBOX_SNAPSHOT_DIR/{${SNAPSHOT_UUID}}.vdi" \
		"format=VDI" \
		"type=Normal"

	# set snapshot dir
	set_attr "/VirtualBox/Machine" "snapshotFolder" "${VBOX_SNAPSHOT_DIR}"

	# set snapshot uuid
	# there should only be exactly one <AttachedDevice> node from the xml
	# retrieved from the server

	set_attr \
		"/VirtualBox/Machine/StorageControllers/StorageController/AttachedDevice/Image" \
		"uuid" "{${SNAPSHOT_UUID}}"
}

setup_floppies() {
	# add storage controller and 2 floppies to it
	add_node \
		"/VirtualBox/Machine/StorageControllers" "StorageController" \
		"name=Floppy" \
		"type=I82078" \
		"PortCount=1" \
		"useHostIOCache=true"
	add_node \
		'/VirtualBox/Machine/StorageControllers/StorageController[@name="Floppy"]' "AttachedDevice" \
		"type=Floppy" \
		"hotpluggable=false" \
		"port=0" \
		"device=0"
	if notempty FLOPPY_0; then
		add_node \
			'/VirtualBox/Machine/StorageControllers/StorageController[@name="Floppy"]/AttachedDevice[@device="0"]' \
			"HostDrive" \
			"src=${FLOPPY_0}"
	fi
	add_node \
		'/VirtualBox/Machine/StorageControllers/StorageController[@name="Floppy"]' "AttachedDevice" \
		"type=Floppy" \
		"hotpluggable=false" \
		"port=0" \
		"device=1"

	# add the slx floppy to the second drive
	add_node \
		'/VirtualBox/Machine/StorageControllers/StorageController/AttachedDevice[@device="1"]' "Image" \
		"uuid={${SLX_FLOPPY_UUID}}"
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

	# set cpu cores and features
	set_attr "/VirtualBox/Machine/Hardware/CPU" "count" "${CPU_CORES}"

	# check if KVM is available and activate it if so
	if source /run/hwinfo && [ "${HW_KVM}" = "ENABLED" ]; then
		set_attr "/VirtualBox/Machine/Hardware/Paravirt" "provider" "KVM"
	fi

	# activate IOAPIC needed for multi core (?)
	if [ $CPU_CORES -gt 1 ]; then
		set_attr "/VirtualBox/Machine/Hardware/BIOS/IOAPIC" "enabled" "true"
	fi

	# PAE support?
	local PAE_SUPPORT="false"
	detect_cpu_flag "pae" && PAE_SUPPORT="true"
	set_attr "/VirtualBox/Machine/Hardware/CPU/PAE" "enabled" "${PAE_SUPPORT}"

	# LongMode?
	local LM_SUPPORT="false"
	detect_cpu_flag "lm" && LM_SUPPORT="true"
	set_attr "/VirtualBox/Machine/Hardware/CPU/LongMode" "enabled" "${LM_SUPPORT}"

	# Page size extensions?
	local PSE_SUPPORT="false"
	detect_cpu_flag "pse" && PSE_SUPPORT="true"
	set_attr "/VirtualBox/Machine/Hardware/CPU/HardwareVirtExLargePages" "enabled" "${PSE_SUPPORT}"
}

setup_usb() {
	## Currently not used
	# EXPERIMENTAL: check for USB 3 support and activate it
	local XHCI_OK="$(lsusb -t | grep xhci)"
	if [ -n "$XHCI_OK" ]; then
		# Afaik, there is no way to configure more than one USB controller
		# through the GUI, but it is technically possible through xml editing.
		# To avoid potential problems, we only add a xHCI controller
		# and delete the others, just as the GUI would configure it.
		if node_exists "/VirtualBox/Machine/Hardware/USB"; then
			del_node "/VirtualBox/Machine/Hardware/USB"
			add_node "/VirtualBox/Machine/Hardware/USB/Controllers" "Controller" "name=xHCI" "type=XHCI"
		fi
	fi
}

setup_sound() {
	# force ALSA driver for now and change when others become available
	set_attr "/VirtualBox/Machine/Hardware/AudioAdapter" "driver" "ALSA"
}

finalize_machine_config() {

	# set the generated machine uuid
	set_attr "/VirtualBox/Machine" "uuid" "{${MACHINE_UUID}}"

	setup_disk_image
	setup_floppies
	setup_ethernet
	setup_cpu_ram
	setup_sound
	notempty VBOX_SHARED_FOLDERS && setup_shared_folders

	if ! cp -p "$TMPCONFIG" "$VBOX_MACHINE_CONFIG"; then
		writelog "Could not copy '$TMPCONFIG' to '$VBOX_MACHINE_CONFIG'!"
	fi
}

call_post_source finalize_machine_config
