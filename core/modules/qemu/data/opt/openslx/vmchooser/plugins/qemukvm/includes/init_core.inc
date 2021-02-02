# -----------------------------------------------------------------------------
#
# Copyright (c) 2009..2018 bwLehrpool-Projektteam
#
# This program/file is free software distributed under the GPL version 2.
# See https://www.gnu.org/licenses/old-licenses/gpl-2.0-standalone.html
#
# If you have any feedback please consult https://bwlehrpool.de and
# send your feedback to bwlehrpool@hs-offenburg.de.
#
# General information about bwLehrpool can be found at https://bwlehrpool.de
#
# -----------------------------------------------------------------------------
# run-virt.include
#    - Include script for running the QEMU/Linux KVM virtual machine on an 
#      OpenSLX client via the run-virt.sh or run-vmgrid.sh
################################################################################

# check if kvm kernel module are available
if [ ! -c /dev/kvm ]; then
	writelog "KVM kernel modules not loaded!"
	EXIT_TYPE="internal" EXIT_REASON="QEMU/KVM Virtualisierer nicht initialisiert!" cleanexit 1
fi

# check if qemu binaries are available
if ! check_dep qemu-system-{i386,x86_64}; then
	writelog "QEMU binaries not available!"
	EXIT_TYPE="internal" EXIT_REASON="QEMU/KVM Virtualisierer nicht initialisiert!" cleanexit 1
fi

# set general purpose options here, mostly evaluates information 
# contained in /opt/openslx/vmchooser/config/virtualization.conf

# TODO some options should come from the server, in particular
# - disk controller type (virtio, scsi, ide...)
#	- arch to emulate (x86_64, i386, ...)
# - graphics mode?
# - sound dev?
# - tbd

# display name, remove blanks because of cmdline problems
declare -rg CMD_DISPLAYNAME=$(echo ${VM_DISPLAYNAME} | sed -e "s, ,-,g;s,(,[,g;s,),],g")
VIRTCMDOPTS+=( "-name" "${CMD_DISPLAYNAME}" )

# graphical start: vga, vmware, qxl, spice?
VIRTCMDOPTS+=( "-vga" "std" "-full-screen" )

# hot keys ALT+CTRL+SHIFT (does not work properly!?)
VIRTCMDOPTS+=( "-alt-grab" )

# TODO support other types of boot? any usecase for network boot?
# for now just boot from disk
VIRTCMDOPTS+=( "-boot" "c" )

