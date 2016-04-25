#####################################################################
# Include: Declaration of vmware run-virt-include default variables #
#####################################################################

set_vmware_include_variables() {
	[ "$mem" -gt 3800 -a "$(uname -m)" != "x86_64" ] && mem=3800
	
	# temporary disk space for logs, etc...
	redodir="/tmp/virt/vmware/${USER}.$$"
	
	# dir for configs and vmem file
	confdir="/tmp/virt/vmware/${USER}.$$"
	
	# configfile
	conffile="${confdir}/run-vmware.conf"
	
	# diskfile
	diskfile="${vmpath}"
	
	# users vmware config folder
	[ -z "${HOME}" ] && HOME=$(getent passwd "$(whoami)" | awk -F ':' '{print $6}')
	vmhome="${HOME}/.vmware"
	
	# get several version infos for vmware/player
	source /opt/openslx/vmchooser/vmware/vmware.conf
	
	# VMware start options
	# "-X": start in fullscreen
	vmopt="-X"
}
