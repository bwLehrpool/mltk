#!/bin/bash
################################################
# Include: Sets core variables and directories #
################################################

# Create temporary directory for current invocation
if check_dep mkdir && ! mkdir -p "$TMPDIR" "$CONFDIR"; then
	writelog "Could not create temporary directories for session"
	EXIT_TYPE="internal" EXIT_REASON="Konnte kein Arbeitsverzeichnis für die VM-Sitzung anlegen." cleanexit 1
fi

# Check that /tmp/virt is not in RAM.
# Either mounted directly (e.g. NFS) or backed by hdd mounted /tmp.
# This variable will be true or false
declare -g TMPDIR_IN_RAM=false
if fs_path_isvolatile /tmp/virt ; then
	TMPDIR_IN_RAM=true
	writelog "/tmp/virt is in RAM, will allocate little RAM to VMs."
else
	writelog "/tmp/virt is not in RAM, will allocate more RAM to VMs."
fi
readonly TMPDIR_IN_RAM

# Get a unique VM_ID for the current invocation
get_vm_id
