#!/bin/bash
#######################################################################
# Include: Declaration of core functions and variables of this plugin #
#######################################################################
# This core include just runs commands directly

## Sanity checks for systemd-vmware_env
# /dev/vmmon should exist, missing vmnet's does not make vmplayer crash
if [ ! -c /dev/vmmon ]; then
	# would cause vmplayer to crash, so abort
	EXIT_TYPE="internal" EXIT_REASON="VMWare wurde nicht richtig initialisiert!" cleanexit 1
fi

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
[ -z "$UID" ] && UID=$(id -u)
[ -z "${HOME}" ] && declare -rg HOME=$(getent passwd "$UID" | awk -F ':' '{print $6}')
[ -z "${HOME}" ] && slxlog "run-virt-vmware-home" "Could not get home directory of user $(whoami) ($UID)"
declare -rg VM_HOME="${HOME}/.vmware"

# mltk generates a vmware config file with several version infos for vmware/player, read it
$(safesource "${VMWARE_PLUGIN_DIR}/vmware.conf")

# Determine maximum hw version
# Try to tell from the xml files in this dir, and hope nobody added any
gmax="$( find /etc/vmware/hostd/env/ -name "ws-*.xml" | grep -o '[0-9]*' | sort -nr | head -n 1 )"
if [ -z "$gmax" ]; then
	# Failed, VMware 16 doesn't have above xml files anymore, great. Back to hard coded lists
	gmax="$( grep -m 1 -F '.product.version' /etc/vmware/config | grep -oE '[0-9]+\.[0-9.]+' | head -n 1 )"
	case "$gmax" in
		16.*) gmax=18 ;;
		15.5.*|15.1.*) gmax=16 ;;
		*)
			# No hit, guess major version number == max hw version, which unfortunately VMware decided to fuck up with
			# VMware 15.1.0, which supports hw version 16
			gmax="${gmax%%.*}"
			;;
	esac
fi

[ -n "$gmax" ] && maxhardwareversion="$gmax"

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

