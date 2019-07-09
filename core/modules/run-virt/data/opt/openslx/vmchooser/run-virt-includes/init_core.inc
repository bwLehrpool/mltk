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
# This variable is empty (but set!) if /tmp/virt isn't backed at all
# else it will be non-empty (check with helper function 'notempty').
declare -g TMPDIR_NOT_RAM=
if ! dir_on_tmpfs /tmp/virt ; then
	writelog "/tmp/virt is not in RAM, will allocate more RAM to VMs."
	TMPDIR_NOT_RAM=1
	readonly TMPDIR_NOT_RAM
fi

# Get a unique VM_ID for the current invocation
get_vm_id
