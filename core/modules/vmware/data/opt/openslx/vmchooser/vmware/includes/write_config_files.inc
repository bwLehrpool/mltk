###############################
# Include: Write config files #
###############################

writelog "Redo directory: ${redodir}"
writelog "Config directory: ${confdir}"
writelog "Config file: ${conffile}"
writelog "vmhome/dndlogs: ${vmhome}/dndlogs"

# create vmware directories
mkdir -p "$redodir" >/dev/null 2>&1
mkdir -p "$confdir" >/dev/null 2>&1
mkdir -p "$vmhome/dndlogs" >/dev/null 2>&1
touch "$vmhome/dndlogs/dndlog.conf" >/dev/null 2>&1

# create preferences file ${vmhome}/preferences
source "${VMWAREINCLUDEDIR}/create_vmhome_preferences_file.inc" && create_vmhome_preferences_file

# create VMware config file (conffile)
source "${VMWAREINCLUDEDIR}/create_vmware_config_file.inc"

# link to conffile if confdir != redodir
[ "$confdir" != "$redodir" ] && ln -s "$conffile" "$redodir/run-vmware.conf" >/dev/null 2>&1

# own nvram. We need it for floppy drive b, default nvram has just drive a
if ! cp "/opt/openslx/vmchooser/vmware/nvram" "$confdir/nvram"; then
	slxlog "virt-vmware-nvram" "Could not copy nvram from '/opt/openslx/vmchooser/vmware/nvram' '$confdir/nvram'"
fi

