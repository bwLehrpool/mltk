#!/bin/bash
################################################
# Include: Sets core variables and directories #
################################################
# Global variables needed for the core functionality
declare -rg VMCHOOSER_DIR="/opt/openslx/vmchooser"
declare -rg VMCHOOSER_CONF_DIR="$VMCHOOSER_DIR/config"
declare -rg USER="$(whoami)"
declare -rg TMPDIR="/tmp/virt/${USER}/$$"

# Check if the path to the logfile was given externally (by the wrapper)
unset WRAPPED
if notempty LOGFILE; then
	if [ ! -w "${LOGFILE}" ]; then
		# given logfile is not writeable or non-existant
		# so handle it ourself (so do not set WRAPPED)
		rm -f "${LOGFILE}"
		unset LOGFILE
	else
		# given logfile was ok, mark that we are wrapped
		# (for cleanexit to not slxlog the log since
		# the wrapper is handling that for us)
		declare -rg WRAPPED=aye
	fi
fi
isset LOGFILE || declare -rg LOGFILE="/var/log/openslx/run-virt.${USER}.$$.log"

# Create temporary directory for current invocation
if check_dep mkdir && ! mkdir -p "$TMPDIR"; then
	writelog "Could not create temporary directory '$TMPDIR' for session"
	EXIT_TYPE="internal" EXIT_REASON="Konnte kein Arbeitsverzeichnis für die VM-Sitzung anlegen." cleanexit 1
fi

# Check that /tmp/virt is not in RAM.
# Either mounted directly (e.g. NFS) or backed by hdd mounted /tmp.
# This variable is empty (but set!) if /tmp/virt isn't backed at all
# else it will be non-empty (check with helper function 'notempty').
declare -g TMPDIR_NOT_RAM=
if ! dir_on_tmpfs /tmp/virt ; then
	writelog "/tmp/virt is not in RAM, will allocate more RAM to VMs."
	TMPDIR_NOT_RAM=1
	readonly TMPDIR_NOT_RAM
fi

# Get a unique VM_ID for the current invocation
get_vm_id
