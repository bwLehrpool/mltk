################################################################################
# Include: write final machine configuration file                              #
################################################################################
finalize_machine_config() {
	# Expected path to the final vbox file
	VBOX_MACHINE_CONFIG="${VBOX_MACHINES_DIR}/${VM_CLEANNAME}/${VM_CLEANNAME}.xml"

	# remove ':' from MAC addr for vbox and generate a VDE (virtual device ethernet)
	VM_MAC_ADDR="$(sed 's/://g' <<< ${VM_MAC_ADDR})"

	# set machine uuid
	set_attr "/VirtualBox/Machine" "uuid" "{${MACHINE_UUID}}"

	# set read-only image path and uuid
	set_attr \
		"/VirtualBox/Machine/MediaRegistry/HardDisks/HardDisk" \
		"location" "${VBOX_HDD_LINK}"
	set_attr \
		"/VirtualBox/Machine/MediaRegistry/HardDisks/HardDisk" \
		"uuid" "{${HDD_UUID}}"
	
	# set snapshot dir
	set_attr "/VirtualBox/Machine" "snapshotFolder" "${VBOX_SNAPSHOT_DIR}"

	# Add a HardDisk node for the snapshot
	add_node \
		"/VirtualBox/Machine/MediaRegistry/HardDisks/HardDisk" "HardDisk" \
		"uuid={${SNAPSHOT_UUID}}" \
		"location=$VBOX_SNAPSHOT_DIR/{${SNAPSHOT_UUID}}.vdi" \
		"format=VDI" \
		"type=Normal"

	# set snapshot uuid to the storage controller
	set_attr \
		"/VirtualBox/Machine/StorageControllers/StorageController/AttachedDevice/Image" \
		"uuid" "{${SNAPSHOT_UUID}}"


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


	# Add a node for the SharedFolder
	add_node "/VirtualBox/Machine/Hardware" "SharedFolders"
	if [ -n "${HOME_SHARE_NAME}" -a -n "${HOME_SHARE_PATH}" -a -d "${HOME_SHARE_PATH}" ]; then
		add_node \
			"/VirtualBox/Machine/Hardware/SharedFolders" "SharedFolder" \
			"name=${HOME_SHARE_NAME}" \
			"hostPath=${HOME_SHARE_PATH}" \
			"writable=true" \
			"autoMount=true"
	fi

	# set the MAC address
	set_attr "/VirtualBox/Machine/Hardware/Network/Adapter" "MACAddress" "${VM_MAC_ADDR}"

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

	detect_cpu_flag() {
		grep -m1 '^flags\s*:' /proc/cpuinfo | grep -qw -e "$1"
	}

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

	# EXPERIMENTAL: check for USB 3 support
	local XHCI_OK="$(lsusb -t | grep xhci)"
	if [ -n "$XHCI_OK" ]; then
		del_node "/VirtualBox/Machine/Hardware/USB"
		add_node "/VirtualBox/Machine/Hardware/USB/Controllers" "Controller" "name=xHCI" "type=XHCI"
	fi

	# EXPERIMENTAL: ALSA-driven sound is buggy as of vbox 5.2.8
	# we still force alsa as we only have this driver in the current minilinux
	set_attr "/VirtualBox/Machine/Hardware/AudioAdapter" "driver" "ALSA"

	cp $TMPCONFIG $VBOX_MACHINE_CONFIG
}

call_post_source finalize_machine_config
