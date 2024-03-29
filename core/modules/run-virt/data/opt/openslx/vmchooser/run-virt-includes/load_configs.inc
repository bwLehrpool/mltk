###################################################
# Include: Declaration of run-virt core variables #
###################################################
## Functions ##
check_base_url() {
	if isempty SLX_VMCHOOSER_BASE_URL; then
	  writelog "SLX_VMCHOOSER_BASE_URL is not set! Was /opt/openslx/config sourced?"
	  EXIT_TYPE="internal" EXIT_REASON="msg.vmchooser.url-missing Keine URL zur Schnittstelle des bwLehrpool-Satelliten gefunden!" cleanexit 1
	fi
	return 0
}
load_configs() {
	writelog "Loading configs..."

	# Include general configuration from vmchooser
	$(safesource --exit "$VMCHOOSER_CONF_DIR/vmchooser.conf")

	# Load general virtualization information
	$(safesource --exit "$VMCHOOSER_CONF_DIR/virtualization.conf")

	# Load config for resource URLs
	if [ -s "$VMCHOOSER_CONF_DIR/resource_urls.conf" ]; then
		$(safesource "$VMCHOOSER_CONF_DIR/resource_urls.conf")
	else
		writelog "No resource URL overrides file provided..."
	fi
	if [ -n "$url_lecture_config" ]; then
		writelog "URL for lecture config was set to $url_lecture_config"
	else
		check_base_url
		url_lecture_config="${SLX_VMCHOOSER_BASE_URL}/lecture/%UUID%"
	fi
	if [ -n "$url_lecture_metadata" ]; then
		writelog "URL for lecture metadata was set to $url_lecture_metadata"
	else
		check_base_url
		url_lecture_metadata="${SLX_VMCHOOSER_BASE_URL}/lecture/%UUID%/metadata"
	fi
	if [ -n "$url_lecture_netshares" ]; then
		writelog "URL for lecture netshares was set to $url_lecture_netshares"
	else
		check_base_url
		url_lecture_netshares="${SLX_VMCHOOSER_BASE_URL}/lecture/%UUID%/netshares"
	fi
	if [ -n "$url_lecture_runscript" ]; then
		writelog "URL for lecture runscript was set to $url_lecture_runscript"
	else
		check_base_url
		url_lecture_runscript="${SLX_VMCHOOSER_BASE_URL}/lecture/%UUID%/runscript"
	fi
}

## MAIN ##
call_post_source load_configs
