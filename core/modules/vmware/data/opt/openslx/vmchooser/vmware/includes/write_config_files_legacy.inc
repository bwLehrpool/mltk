###############################
# Include: Write config files #
###############################

write_config_files_legacy() {
	# create vmware directories
	mkdir -p "$redodir" >/dev/null 2>&1
	mkdir -p "$confdir" >/dev/null 2>&1
	mkdir -p "$vmhome/dndlogs" >/dev/null 2>&1
	touch "$vmhome/dndlogs/dndlog.conf" >/dev/null 2>&1
	
	# create preferences file ${vmhome}/preferences
	source "${VMWAREINCLUDEDIR}/create_vmhome_preferences_file.inc" && create_vmhome_preferences_file
	
	# create VMware config file (conffile)
	source "${VMWAREINCLUDEDIR}/create_vmware_config_file_legacy.inc" && create_vmware_config_file_legacy

	# link to conffile if confdir != redodir
	[ "$confdir" != "$redodir" ] && ln -s "$conffile" "$redodir/run-vmware.conf" >/dev/null 2>&1
	
	# sync is needed to ensure that data is really written to virtual disk
	# TODO: Move to general run-virt, right before exec of vm
	sync
	
	# own nvram. We need it for floppy drive b, default nvram has just drive a
	if ! cp "/opt/openslx/vmchooser/vmware/nvram" "$confdir/nvram"; then
		slxlog "virt-vmware-nvram" "Could not copy nvram from '/opt/openslx/vmchooser/vmware/nvram' '$confdir/nvram'"
	fi
}
