#######################################################################
# Include: Declaration of core variables and some sanity checks       #
#######################################################################

# wrapper for xmlstarlet to add node with attributes and value
# to the machine configuration
# Usage:
#   add_node <parent> <newnode> <attrs...>
# E.g:
#   add_node "/VirtualBox" "Machine" "uuid=1234"
add_node() {
	if [ $# -lt 2 ]; then
		writelog "${FUNCNAME[0]} requires 2 or more args, $# given."
		return 1
	fi
	local PARENT="$1"
	shift
	if isempty PARENT; then
		writelog "${FUNCNAME[0]} empty parent given!"
		return 1
	fi
	local NODE="$1"
	shift
	if isempty NODE; then
		writelog "${FUNCNAME[0]} empty node given!"
		return 1
	fi
	# if parent does not exists, create it aswell
	if ! node_exists "${PARENT}"; then
		add_node "${PARENT%/*}" "${PARENT##*/}"
	fi
	# now go over the list of attributes and directly create
	# the xmlstarlet syntax string: -t attr -n <key> -v <value>
	local ATTRS_OPTS=
	while [ $# -ne 0 ]; do
		# expects key=value syntax
		local key="${1%=*}"
		local value="${1#*=}"
		shift
		if isempty key || isempty value; then
			writelog "${FUNCNAME[0]} expecting key=value, got: $1"
			cleanexit 1
		fi
		# the xmlstarlet internal newnode var references the newly created node
		ATTRS_OPTS+=" -i \$newnode -t attr -n ${key} -v ${value}"
	done
	# args parsed, now create the node using xmlstarlet
	# insert namespace to xpath expression
	PARENT="$(print_namespaced "x" "${PARENT}")"
	# create node and set the internal variable newnode to it
	# to then add the attributes to it
	local ret
	xmlstarlet ed -L -N x="${VBOX_NAMESPACE}" \
		-s "${PARENT}" -t elem -n "${NODE}" \
		--var newnode '$prev' \
		${ATTRS_OPTS} \
		"${TMPCONFIG}"
	ret=$?
	if [ "$ret" -ne 0 ]; then
		writelog "${FUNCNAME[0]} failed with '$ret' for args: $@"
	fi
	return $ret
}
# set_attr <node_xpath> <attr> <value>
set_attr() {
	if [ $# -ne 3 ]; then
		writelog "${FUNCNAME[0]} requires 3 args, $# given."
		return 1
	fi
	if [ -z "$1" -o -z "$2" -o -z "$3" ]; then
		writelog "${FUNCNAME[0]} empty arguments given: $@"
		return 1
	fi
	if ! node_exists "$1"; then
		add_node "${1%/*}" "${1##*/}" \
			"$2=$3"
	else
		local ret
		if ! attr_exists "$1" "$2"; then
			xmlstarlet ed -L -N x="${VBOX_NAMESPACE}" \
				-i "$(print_namespaced "x" "$1")" \
				-t attr -n "$2" -v "$3" \
				"${TMPCONFIG}"
			ret=$?
		else
			xmlstarlet ed -L -N x="${VBOX_NAMESPACE}" \
				-u "$(print_namespaced "x" "$1")/@$2" \
				-v "$3" \
				"${TMPCONFIG}"
			ret=$?
		fi
		if [ "$ret" -ne 0 ]; then
			writelog "${FUNCNAME[0]} failed with '$ret' for args: $@"
		fi
		return $ret
	fi
}
# del_node <node_xpath>
del_node() {
	if [ $# -ne 1 ]; then
		writelog "${FUNCNAME[0]} requires one arg, $# given."
		return 1
	fi
	if [ -z "$1" ]; then
		writelog "${FUNCNAME[0]} empty xpath expression given."
		return 1
	fi
	local ret
	xmlstarlet ed -L -N x="${VBOX_NAMESPACE}" \
		-d "$(print_namespaced "x" "$1")" \
		"${TMPCONFIG}"
	ret=$?
	if [ "$ret" -ne 0 ]; then
		writelog "${FUNCNAME[0]} failed with '$ret' for args: $@"
	fi
	return $ret
}
# attr_exists <node_xpath> <attr_name>
attr_exists() {
	if [ $# -ne 2 ]; then
		writelog "${FUNCNAME[0]} requires node and attribute as args, $# given."
		return 1
	fi
	if [ -z "$1" -o -z "$2" ]; then
		writelog "${FUNCNAME[0]} some arguments were empty: $@"
	fi
	xmlstarlet -q sel -N x="${VBOX_NAMESPACE}" \
		-t -v "$(print_namespaced "x" "$1")/@$2" \
		"${TMPCONFIG}"
	ret=$?
	if [ "$ret" -ne 0 ]; then
		writelog "${FUNCNAME[0]} failed with '$ret' for args: $@"
	fi
	return $ret
}
# node_exists <node_xpath>
node_exists() {
	if [ $# -ne 1 ]; then
		writelog "${FUNCNAME[0]} requires one arg, $# given."
		return 1
	fi
	if [ -z "$1" ]; then
		writelog "${FUNCNAME[0]} empty xpath expression given."
		return 1
	fi
	xmlstarlet -q sel -N x="${VBOX_NAMESPACE}" \
		-t -c "$(print_namespaced "x" "$1")" \
		"${TMPCONFIG}"
	ret=$?
	if [ "$ret" -ne 0 ]; then
		writelog "${FUNCNAME[0]} failed with '$ret' for args: $@"
	fi
	return $ret
}
# adds the vbox namespace to an xpath expression using
# the given placeholder for it. 
# e.g. print_namespaced "x" "/VirtualBox/Machine"
# would echo "/x:VirtualBox/x:Machine"
print_namespaced() {
	if [ $# -ne 2 ]; then
		writelog "${FUNCNAME[0]} expects 2 arguments, $# given."
		cleanexit 1
	fi
	# add namespace on single '/' not doubles!
	sed -E 's,(/)*/,\1/'"$1"':,g' <<< "$2"
}

gen_uuid() {
	local UUID="$(cat /proc/sys/kernel/random/uuid)"
	notempty UUID && echo $UUID && return 0
	# fallback using $RANDOM
	UUID=
	while [ ${#UUID} -lt 32 ]; do
		UUID="$UUID$RANDOM"
	done
	# sequence might be larger than 32, cut it and insert dashes
	echo -n "${UUID:0:32}" | \
		sed -E 's,(.{8})(.{4})(.{4})(.{4})(.{12}),\1-\2-\3-\4-\5,'
}

init_core() {
	# check for variables that should have been set by the generic run-virt
	if ! isset VM_CLEANNAME IMG_BASENAME SRC_IMG_ABSOLUTE VM_OS_TYPE; then
		local MSG="Incomplete A required variable was not set by generic run-virt."
		slxlog "virt-vbox-init" "${MSG}"
		writelog "${MSG}"
		error_user "Wichtige variablen, die das VBox Plugin benötigt, wurden nicht
			gesetzt. Kann die VM nicht starten."
		cleanexit 1
	fi

	# Check for vbox binaries in VBOX_BASE_DIR
	for BIN in VBoxManage VirtualBox VBoxHeadless; do
		if ! [ -x "${VBOX_BASE_DIR}/${BIN}" ]; then
			local MSG="${BIN} not found in ${VBOX_BASE_DIR}."
			writelog "${MSG}"
			slxlog "virt-vbox-bin" "${MSG}"
			cleanexit 1
		fi
	done

	# check if the systemd vbox setup service ran successfully
	if ! systemctl is-active vbox.service; then
		local STATUSDUMP=$(mktemp)
		systemctl status vbox.service > "$STATUSDUMP"
		slxlog "virt-vbox-env" "vbox.service failed to start properly" "$STATUSDUMP"
		EXIT_TYPE="internal" EXIT_REASON="VirtualBox wurde nicht richtig initialisiert!" cleanexit 1
	fi

	# validate that TMPCONFIG is actually an xml file
	if ! xmlstarlet val "$TMPCONFIG"; then
		slxlog "virt-vbox-noxml" "Downloaded machine description was not a valid xml." "$TMPCONFIG"
		EXIT_TYPE="internal" EXIT_REASON="Die VM-Konfiguration ist keine valide VirtualBox-Datei!
Ihren Satelliten scheint VirtualBox nicht zu unterstützen." cleanexit 1
	fi

	# Define which features the VMware plugin supports
	# Session specific dir for VM configs
	declare -rg VBOX_ROOT="/tmp/virt/${PLUGIN_ID}/${USER}.$$"
	declare -rg VBOX_MACHINES_DIR="${VBOX_ROOT}/Machines"
	declare -rg VBOX_SNAPSHOT_DIR="${VBOX_MACHINES_DIR}/${VM_CLEANNAME}/Snapshots"
	if ! mkdir -p "${VBOX_SNAPSHOT_DIR}"; then
		writelog "Failed to create '${VBOX_SNAPSHOT_DIR}'."
		cleanexit 1
	fi
	declare -rg VBOX_HDD_DIR="${VBOX_ROOT}/HardDisks"
	if ! mkdir -p "${VBOX_HDD_DIR}"; then
		writelog "Failed to create '${VBOX_HDD_DIR}'."
		cleanexit 1
	fi
	# Virtualbox configuration dir instead of $HOME/.VirtualBox
	export VBOX_USER_HOME="${VBOX_ROOT}"

	# xml namespace for vbox configs
	declare -rg VBOX_NAMESPACE="http://www.virtualbox.org/"

	# path to the main machine configuration file
	declare -rg VBOX_MACHINE_CONFIG="${VBOX_MACHINES_DIR}/${VM_CLEANNAME}/${VM_CLEANNAME}.xml"

	writelog "Directories:"
	writelog "\tConfig dir:\t\t$VBOX_ROOT"
	writelog "\tMachines dir:\t\t$VBOX_MACHINES_DIR"
	writelog "\tSnapshots dir:\t\t$VBOX_SNAPSHOT_DIR"
}

call_post_source init_core
