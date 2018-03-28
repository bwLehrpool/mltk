################################################################################
# Include: write final machine configuration file                              #
################################################################################
finalize_machine_config() {
	# Expected path to the final vbox file
	VBOX_MACHINE_CONFIG="${VBOX_MACHINES_DIR}/${VM_CLEANNAME}/${VM_CLEANNAME}.xml"

	# remove ':' from MAC addr for vbox and generate a VDE (virtual device ethernet)
	# MAC addr from the first one (put in prefix 00DE)
	VM_MAC_ADDR=$(echo ${VM_MAC_ADDR} | sed 's/://g')
	VM_VDE_MAC_ADDR=$(echo ${VM_MAC_ADDR} | sed 's/^..../00DE/g')

	# translate network kinds (nat, bridged, host-only)
	# TODO: Server should prepare this in returned xml
	case "${network_kind}" in
		bridge*)
			network_kind='HostOnlyInterface name="vboxnet0"'
			;;
		host*)
			network_kind='HostOnlyInterface name="vboxnet2"'
			;;
		*)
			network_kind='HostOnlyInterface name="vboxnet1"'
	esac

	sed -i "s,%VM_DISK_REDOLOGDIR%,$VBOX_SNAPSHOT_DIR,g" $TMPCONFIG
	sed -i "s,%VM_DISK_PATH%,${VBOX_HDD_LINK},g" $TMPCONFIG
	sed -i "s/#OpenSLX_MUUID_place_holder/{${MACHINE_UUID}}/g" $TMPCONFIG
	sed -i "0,/#OpenSLX_HDDUUID_0_placeholder/ s/#OpenSLX_HDDUUID_0_placeholder/{${HDD_UUID}}/" $TMPCONFIG
	sed -i "0,/#OpenSLX_HDDUUID_0_placeholder/ s/#OpenSLX_HDDUUID_0_placeholder/{${SNAPSHOT_UUID}}/" $TMPCONFIG
	sed -i 's,#OpenSLX_CPU_place_holder,'"${CPU_CORES}"',g' $TMPCONFIG
	sed -i 's,#OpenSLX_MEMORY_place_holder,'"${VM_MEM}"',g' $TMPCONFIG
	
	# Add a node for the SharedFolder
	add_node "/VirtualBox/Machine/Hardware" "SharedFolders"
	add_node \
		"/VirtualBox/Machine/Hardware/SharedFolders" "SharedFolder" \
		"name=${HOME_SHARE_NAME}" \
		"hostPath=${HOME_SHARE_PATH}" \
		"writable=true" \
		"autoMount=true"
	# Add a HardDisk node for the snapshot
	add_node \
		"/VirtualBox/Machine/MediaRegistry/HardDisks/HardDisk" "HardDisk" \
		"uuid={${SNAPSHOT_UUID}}" \
		"location=$VBOX_SNAPSHOT_DIR/{${SNAPSHOT_UUID}}.vdi" \
		"format=VDI" \
		"type=Normal"
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

	# set Virtual Memory value
	edit_attr "/VirtualBox/Machine/Hardware/Display" "VRAMSize" "${GUESTVRAM}"
	edit_attr --create "/VirtualBox/Machine/Hardware/Display" "accelerate2DVideo" "true"

	# set the MAC address
	edit_attr "/VirtualBox/Machine/Hardware/Network/Adapter" "MACAddress" "${VM_VDE_MAC_ADDR}"

	# check if KVM is available and activate it if so
	if source /run/hwinfo && [ "${HW_KVM}" = "ENABLED" ]; then
		edit_attr --create "/VirtualBox/Machine/Hardware/Paravirt" "provider" "KVM"
	fi
	# activate IOAPIC needed for multi core (?)
	if [ $CPU_CORES -gt 1 ]; then
		edit_attr --create "/VirtualBox/Machine/Hardware/BIOS/IOAPIC" "enabled" "true"
	fi

	cp $TMPCONFIG /tmp/vbox-last-config
	cp $TMPCONFIG $VBOX_MACHINE_CONFIG
}

call_post_source finalize_machine_config
