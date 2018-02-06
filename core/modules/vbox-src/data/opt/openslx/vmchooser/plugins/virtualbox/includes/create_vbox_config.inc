################################################################################
# Include: create main vbox config file and add machine, hdd, ... entries      #
################################################################################
create_vbox_config() {
	# generate random machine/floppies UUIDs
	declare -rg MACHINE_UUID="$(gen_uuid)"
	declare -rg SLX_FLOPPY_UUID="$(gen_uuid)"

	cat <<EOF > "${VBOX_ROOT}/VirtualBox.xml"
<VirtualBox xmlns="http://www.innotek.de/VirtualBox-settings" version="1.12-linux">
	<Global>
		<ExtraData>
			<ExtraDataItem name="GUI/Input/AutoCapture" value="true"/>
			<!-- Host Key: left ctrl + left winkey + left alt -->
			<ExtraDataItem name="GUI/Input/HostKeyCombination" value="65507,65513,65515"/>
			<ExtraDataItem name="GUI/LastVMSelected" value="${MACHINE_UUID}"/>
			<ExtraDataItem name="GUI/HostScreenSaverDisabled" value="false"/>
			<ExtraDataItem name="GUI/LicenseAgreed" value="1,2,3,4,5,6,7,8,9"/>
			<ExtraDataItem name="GUI/MaxGuestResolution" value="any"/> <!-- auto, width,height -->
			<ExtraDataItem name="GUI/UpdateCheckCount" value="2"/>
			<ExtraDataItem name="GUI/UpdateDate" value="never"/>
			<ExtraDataItem name="GUI/SuppressMessages" value=",remindAboutAutoCapture,confirmInputCapture,remindAboutWrongColorDepth,confirmGoingFullscreen,remindAboutMouseIntegrationOn,remindAboutMouseIntegrationOff,showRuntimeError.warning.DevATA_DISKFULL,remindAboutPausedVMInput,confirmVMReset,showRuntimeError.warning.HostAudioNotResponding,showRuntimeError.warning.ExtPackNoEhci,remindAboutMouseIntegration"/>
			<ExtraDataItem name="GUI/TrayIcon/Enabled" value="false"/>
		</ExtraData>
		<MachineRegistry>
			<MachineEntry uuid="{${MACHINE_UUID}}" src="Machines/${VM_CLEANNAME}/${VM_CLEANNAME}.xml"/>
		</MachineRegistry>
		<MediaRegistry>
			<HardDisks>
				<HardDisk uuid="{${HDD_UUID}}" location="${VBOX_HDD_LINK}" format="${VBOX_HDD_FORMAT}" type="${VBOX_HDD_TYPE}">
					<HardDisk uuid="{${SNAPSHOT_UUID}}" location="${VBOX_SNAPSHOT_DIR}/{${SNAPSHOT_UUID}}.vdi" format="VDI" autoReset="true"/>
				</HardDisk>
			</HardDisks>
			<FloppyImages>
				<Image uuid="{${SLX_FLOPPY_UUID}}" location="${SLX_FLOPPY_IMG}"/>
			</FloppyImages>
		</MediaRegistry>
		<NetserviceRegistry>
			<DHCPServers/>
		</NetserviceRegistry>
		<SystemProperties defaultMachineFolder="${VBOX_MACHINES_DIR}" defaultHardDiskFolder="${VBOX_HDD_DIR}" defaultHardDiskFormat="VDI" remoteDisplayAuthLibrary="VBoxAuth" webServiceAuthLibrary="VBoxAuth" LogHistoryCount="3"/>
		<USBDeviceFilters/>
	</Global>
</VirtualBox>
EOF
}

call_post_source create_vbox_config
