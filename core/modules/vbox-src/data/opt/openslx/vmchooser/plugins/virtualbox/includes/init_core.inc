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
		cleanexit 1
	fi
	local PARENT="$1"
	shift
	local NODE="$1"
	shift
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
	xmlstarlet ed -L -N x="${VBOX_NAMESPACE}" \
		-s "${PARENT}" -t elem -n "${NODE}" \
		--var newnode '$prev' \
		${ATTRS_OPTS} \
		"${TMPCONFIG}"
}
# edit_attr [--create] <node_xpath> <attr=value>
# --create would create the node if it does not exist
edit_attr() {
	local CREATE=
	if [ "$1" = "--create" ]; then
		CREATE=yo
		shift
	fi
	if [ $# -ne 3 ]; then
		writelog "${FUNCNAME[0]} requires 3 args, $# given."
		cleanexit 1
	fi
	if notempty CREATE && ! node_exists "$1"; then
		add_node "${1%/*}" "${1##*/}" \
			"$2=$3"
	else
		xmlstarlet ed -L -N x="${VBOX_NAMESPACE}" \
			-u "$(print_namespaced "x" "$1")/@$2" \
			-v "$3" \
			"${TMPCONFIG}"
	fi
}
del_node() {
	if [ $# -ne 1]; then
		writelog "${FUNCNAME[0]} requires one arg, $# given."
		cleanexit 1
	fi
	xmlstarlet ed -L -N x="${VBOX_NAMESPACE}" \
		-d "$(print_namespaced "x" "$1")" \
		"${TMPCONFIG}"
}
node_exists() {
	if [ $# -ne 1 ]; then
		writelog "${FUNCNAME[0]} requires one arg, $# given."
		cleanexit 1
	fi
	xmlstarlet -q sel -N x="${VBOX_NAMESPACE}" \
		-t -c "$(print_namespaced "x" "$1")" \
		"${TMPCONFIG}"
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

	# Define which features the VMware plugin supports
	declare -rg VBOX_ROOT="${TMPDIR}/vbox"	# dir for configs
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
	declare -rg VBOX_HDD_LINK="${VM_DISKFILE_RO}"
	export VBOX_USER_HOME="${VBOX_ROOT}"		# instead of $HOME/.VirtualBox

	# xml namespace for vbox configs
	declare -rg VBOX_NAMESPACE="http://www.virtualbox.org/"

	writelog "Directories:"
	writelog "\tConfig dir:\t\t$VBOX_ROOT"
	writelog "\tMachines dir:\t\t$VBOX_MACHINES_DIR"
	writelog "\tSnapshots dir:\t\t$VBOX_SNAPSHOT_DIR"
}

call_post_source init_core