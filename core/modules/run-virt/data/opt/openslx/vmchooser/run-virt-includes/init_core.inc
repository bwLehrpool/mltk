################################################
# Include: Sets core variables and directories #
################################################
# Global variables needed for the core functionality
declare -rg VMCHOOSER_DIR="/opt/openslx/vmchooser"
declare -rg VMCHOOSER_CONF_DIR="$VMCHOOSER_DIR/config"
declare -rg USER="$(whoami)"
declare -rg LOGFILE="/var/log/openslx/run-virt.${USER}.$$.log"
declare -rg TMPDIR="/tmp/virt/${USER}/$$"

# Create temporary directory for current invocation
if check_dep mkdir && ! mkdir -p "$TMPDIR"; then
	writelog "Could not create temporary directory '$TMPDIR' for session"
	EXIT_TYPE="internal" EXIT_REASON="Konnte kein Arbeitsverzeichnis für die VM-Sitzung anlegen." cleanexit 1
fi

# Get a unique VM_ID for the current invocation
get_vm_id
