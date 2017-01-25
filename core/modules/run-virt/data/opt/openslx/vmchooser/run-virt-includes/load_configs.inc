###################################################
# Include: Declaration of run-virt core variables #
###################################################
## Functions ##
load_configs() {
	writelog "Loading configs..."

	# Include general configuration from vmchooser
	$(safesource --exit "$VMCHOOSER_CONF_DIR/vmchooser.conf")

	# Load general virtualization information
	$(safesource --exit "$VMCHOOSER_CONF_DIR/virtualization.conf")
	
	# Load general openslx config
	$(safesource --exit "/opt/openslx/config")
}

## MAIN ##
call_post_source load_configs
