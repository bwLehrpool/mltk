############################################
# Include: Get needed values from XML file #
############################################
## Functions ##
parse_xml() {
	# Sanity checks
	if ! check_dep get_xml; then
		writelog "Could not find 'get_xml'"
		EXIT_TYPE="internal" EXIT_REASON="Keine XML-Unterstützung!" cleanexit 1
	fi

	writelog "Parsing XML..."
	declare -rg VMSTORE_PATH=/mnt/vmstore
	declare -rg IMGUUID=$(get_xml "uuid")
	if isempty IMGUUID; then
		writelog "Error parsing XML: No value for 'uuid'."
		EXIT_TYPE="user" EXIT_REASON="Konnte keine ID für das ausgewählte Image ermitteln!" cleanexit 1
	fi

	# Relative / Absolute path to image
	declare -rg SRC_IMG_RELATIVE=$(get_xml "image_name")
	declare -g SRC_IMG_ABSOLUTE=$(get_xml "image_path")

	if isempty SRC_IMG_ABSOLUTE SRC_IMG_RELATIVE; then
		writelog "Error parsing XML: Neither relative nor absolute path for image found!"
		EXIT_TYPE="user" EXIT_REASON="Konnte kein Pfad zur ausgewählten virtuellen Maschine ermitteln!" cleanexit 1
	fi

	if notempty SRC_IMG_ABSOLUTE && [ "${SRC_IMG_ABSOLUTE:0:1}" != "/" ]; then
		writelog "Error parsing XML: Parsed value for absolute path doesn't start with '/': '$SRC_IMG_ABSOLUTE'"
		EXIT_TYPE="user" EXIT_REASON="Ungültiger asboluter Pfad zur virtuellen Maschine!" cleanexit 1
	fi

	notempty SRC_IMG_ABSOLUTE || SRC_IMG_ABSOLUTE="${VMSTORE_PATH}/${SRC_IMG_RELATIVE}"
	readonly SRC_IMG_ABSOLUTE

	declare -rg IMG_BASENAME=$(basename "$SRC_IMG_ABSOLUTE")

	VM_DISPLAYNAME=$(get_xml "short_description")
	notempty VM_DISPLAYNAME || VM_DISPLAYNAME="${IMG_BASENAME}"
	readonly VM_DISPLAYNAME

	# Define VM_CLEANNAME since VM_DISPLAYNAME can be long and contain weird characters
	declare -rg VM_CLEANNAME=$(echo "${VM_DISPLAYNAME:0:32}" | sed -r 's/[^0-9a-zA-Z_-\.]+/_/g')

	# image is for the following virtual machine
	declare -rg PLUGIN_ID=$(get_xml "virtualmachine")
	if isempty PLUGIN_ID; then
		writelog "Error parsing XML: No value for 'virtualmachine'."
		EXIT_TYPE="user" EXIT_REASON="Konnte kein Virtualisierer für das ausgewählte Image ermitteln." cleanexit 1
	fi

	# Extracting OS type (VM_OS_TYPE) from xml file. We don't care here whether VM_OS_TYPE is empty, as then
	# it will yield the default entries later on.
	declare -g VM_OS_TYPE=$(get_xml "os")

	# Print summary to log
	writelog "\tVirtualization plugin:  $PLUGIN_ID"
	writelog "\tVM filename:            $IMG_BASENAME"
	writelog "\tVM UUID:                $IMGUUID"
	writelog "\tVM name:                $VM_DISPLAYNAME"
	writelog "\tVM short name:          $VM_CLEANNAME"
	writelog "\tVM OS:                  $VM_OS_TYPE"
	writelog "Done parsing XML."
	return 0
}

## MAIN ##
call_post_source parse_xml
