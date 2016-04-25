######################################################
# Include: Declaration of run-virt default variables #
######################################################

VMCHOOSER_DIR="/opt/openslx/vmchooser"
VMCHOOSER_CONF_DIR="$VMCHOOSER_DIR/config"
LOGFILE="/var/log/openslx/run-virt.${USER}.$$.log"
USER="$(whoami)"
TMPDIR="/tmp/virt/${USER}/$$"

readonly VMCHOOSER_DIR VMCHOOSER_CONF_DIR LOGFILE TMPDIR USER

# include general configuration from vmchooser
[ -f "$VMCHOOSER_CONF_DIR/vmchooser.conf" ] && . "$VMCHOOSER_CONF_DIR/vmchooser.conf"
# load general virtualization information
[ -f "$VMCHOOSER_CONF_DIR/virtualization.conf" ] && . "$VMCHOOSER_CONF_DIR/virtualization.conf"
# Load general openslx config
[ -f "/opt/openslx/config" ] && . "/opt/openslx/config"
# Create temp dir
mkdir -p "$TMPDIR" # TODO error out if failed

