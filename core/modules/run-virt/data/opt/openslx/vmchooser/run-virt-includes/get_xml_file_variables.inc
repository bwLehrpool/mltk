############################################
# Include: Get needed values from XML file #
############################################

writelog "Parsing XML..."

declare -rg VMSTORE_PATH=/mnt/vmstore

get_xml () {
	xmlextract "//settings/eintrag/${1}/@param" "${xmlfile}"
}

IMGUUID=$(get_xml "uuid")

# # Name of the virt image
SRC_IMG_ABSOLUTE=$(get_xml "image_path")
SRC_IMG_RELATIVE=$(get_xml "image_name")

if [ -z "${SRC_IMG_ABSOLUTE}${SRC_IMG_RELATIVE}" ]; then
	writelog "Neither relative nor absolute path for image found in xml"
	cleanexit 1
fi

if [ -n "$SRC_IMG_ABSOLUTE" ] && [ "${SRC_IMG_ABSOLUTE:0:1}" != "/" ]; then
	writelog "Error parsing XML for absolute image path: given value doesn't start with '/': '$SRC_IMG_ABSOLUTE'"
	cleanexit 1
fi

if [ -z "$SRC_IMG_ABSOLUTE" ]; then
	SRC_IMG_ABSOLUTE="${VMSTORE_PATH}/${SRC_IMG_RELATIVE}"
fi

IMG_BASENAME=$(basename "$SRC_IMG_ABSOLUTE")
writelog "Virtual image file name: $IMG_BASENAME"

VM_DISPLAYNAME=$(get_xml "short_description")
[ -z "$VM_DISPLAYNAME" ] && VM_DISPLAYNAME="${IMG_BASENAME}"

# Define VM_NAME_CLEAN since VM_DISPLAYNAME can be long and contain weird characters
VM_NAME_CLEAN=$(echo "${VM_DISPLAYNAME:0:32}" | sed -r 's/[^0-9a-zA-Z_-\.]+/_/g')

# image is for the following virtual machine
PLUGIN_ID=$(grep -o 'virtualmachine param=.*"' "${xmlfile}" \
	| sed -e "s/&.*;/; /g" | awk -F '"' '{print $2}')

# Extracting OS type (VM_OS_TYPE) from xml file. We don't care here whether VM_OS_TYPE is empty, as then 
# it will yield the default entries later on.
VM_OS_TYPE=$(get_xml "os")

readonly IMGUUID
readonly SRC_IMG_ABSOLUTE SRC_IMG_RELATIVE
readonly IMG_BASENAME
readonly VM_DISPLAYNAME VM_NAME_CLEAN
readonly PLUGIN_ID
readonly VM_OS_TYPE

writelog "VM UUID:                $IMGUUID"
writelog "Virtualization plugin:  $PLUGIN_ID"
writelog "VM name:                $VM_DISPLAYNAME"
writelog "VM short name:          $VM_NAME_CLEAN"
writelog "VM OS:                  $VM_OS_TYPE"
writelog "Done parsing XML."

