#######################################################################
# Include: Declaration of core variables and some sanity checks       #
#######################################################################
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

	# finally generate a random machine UUID, using some default if it fails.
	declare -g MACHINE_UUID="$(cat /proc/sys/kernel/random/uuid)"
	notempty MACHINE_UUID || MACHINE_UUID="00000000-0000-0000-0000-12345678"
	readonly MACHINE_UUID

	writelog "Directories:"
	writelog "\tConfig dir:\t\t$VBOX_ROOT"
	writelog "\tMachines dir:\t\t$VBOX_MACHINES_DIR"
	writelog "\tSnapshots dir:\t\t$VBOX_SNAPSHOT_DIR"
}

call_post_source init_core
