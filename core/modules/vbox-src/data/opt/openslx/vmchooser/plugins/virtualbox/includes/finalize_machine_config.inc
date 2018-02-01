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
	
	# Add a HardDisk node
	xmlstarlet ed -L -N x="http://www.virtualbox.org/" -s "//x:VirtualBox/x:Machine/x:MediaRegistry/x:HardDisks/x:HardDisk" -t elem -n HardDisk $TMPCONFIG
	# Add attributes:
	# 1) uuid attribute and point to the snapshotuuid from above
	# 2) location pointing to the snapshot file
	# 3) specify that format is VDI
	# 4) hdd type is "normal" 
	xmlstarlet ed -L -N x="http://www.virtualbox.org/" \
		-i "//x:VirtualBox/x:Machine/x:MediaRegistry/x:HardDisks/x:HardDisk/x:HardDisk" -t attr -n uuid -v "{${SNAPSHOT_UUID}}" \
		-i "//x:VirtualBox/x:Machine/x:MediaRegistry/x:HardDisks/x:HardDisk/x:HardDisk" -t attr -n location -v "$VBOX_SNAPSHOT_DIR/{${SNAPSHOT_UUID}}.vdi" \
		-i "//x:VirtualBox/x:Machine/x:MediaRegistry/x:HardDisks/x:HardDisk/x:HardDisk" -t attr -n format -v "VDI" \
		-i "//x:VirtualBox/x:Machine/x:MediaRegistry/x:HardDisks/x:HardDisk/x:HardDisk" -t attr -n type -v "Normal" \
			$TMPCONFIG
	# set the MAC address
	xmlstarlet ed -L -N x="http://www.virtualbox.org/" -u "//x:VirtualBox/x:Machine/x:Hardware/x:Network/x:Adapter/@MACAddress" -v "$VM_VDE_MAC_ADDR" $TMPCONFIG
	# delete USB controller - TODO fix
	xmlstarlet ed -L -N x="http://www.virtualbox.org/" -d "//x:VirtualBox/x:Machine/x:Hardware/x:USB"  $TMPCONFIG

	# TODO VT-x

	cp $TMPCONFIG /tmp/vbox-last-config
	cp $TMPCONFIG $VBOX_MACHINE_CONFIG
}

call_post_source finalize_machine_config
