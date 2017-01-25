#######################################################################
# Include: Declaration of core functions and variables of this plugin #
#######################################################################
# This core include just executes stuff! No post source calls!
# For now define helpers here, maybe use a dedicated include later on
vmw_cap_hw_version() {
	[ -z "$1" ] && writelog "cap_hw_version called without parameter!" && return 1
	[ "$1" -lt "$maxhardwareversion" ] && maxhardwareversion="$1"
}

# Temporary disk space for logs, etc...
declare -rg VM_REDO_DIR="/tmp/virt/vmware/${USER}.$$"
	
# Dir for configs and vmem file
declare -rg VM_CONF_DIR="/tmp/virt/vmware/${USER}.$$"
	
# The VMX file of the starting VM
declare -rg VM_RUN_FILE="${VM_CONF_DIR}/run-vmware.conf"
	
# Users vmware config folder
[ -z "${HOME}" ] && declare -rg HOME=$(getent passwd "$(whoami)" | awk -F ':' '{print $6}')
declare -rg VM_HOME="${HOME}/.vmware"
	
# mltk generates a vmware config file with several version infos for vmware/player, read it
$(safesource "${VMWARE_PLUGIN_DIR}/vmware.conf")
	
# VMware start options
# "-X": start in fullscreen
declare -rg VM_START_OPTIONS="-X"

# create vmware directories
mkdir -p "$VM_REDO_DIR" >/dev/null 2>&1
mkdir -p "$VM_CONF_DIR" >/dev/null 2>&1
mkdir -p "$VM_HOME/dndlogs" >/dev/null 2>&1
touch "$VM_HOME/dndlogs/dndlog.conf" >/dev/null 2>&1

# link to VM_RUN_FILE if VM_CONF_DIR != VM_REDO_DIR
[ "$VM_CONF_DIR" != "$VM_REDO_DIR" ] && ln -s "$VM_RUN_FILE" "$VM_REDO_DIR/run-vmware.conf" >/dev/null 2>&1

# own nvram. We need it for floppy drive b, default nvram has just drive a
if ! cp "${VMWARE_PLUGIN_DIR}/nvram" "$VM_CONF_DIR/nvram"; then
	slxlog "virt-vmware-nvram" "Could not copy nvram from '${VMWARE_PLUGIN_DIR}/nvram' '$VM_CONF_DIR/nvram'"
fi

