# -----------------------------------------------------------------------------
#
# Copyright (c) 2009..2021 bwLehrpool-Projektteam
#
# This program/file is free software distributed under the GPL version 2.
# See https://www.gnu.org/licenses/old-licenses/gpl-2.0-standalone.html
#
# If you have any feedback please consult https://bwlehrpool.de and
# send your feedback to support@bwlehrpool.de.
#
# General information about bwLehrpool can be found at https://bwlehrpool.de
#
# -----------------------------------------------------------------------------
# Utils and functions to parse Linux kernel command line options
# -----------------------------------------------------------------------------

# Function to parse an option's value from the kernel command line
# Parameter 1: name of the kernel command line option
# Return     : value of the kernel command line option
function kernel_cmdln_parse_option() {
	grep -o "\b${1}=[^ ]*" /proc/cmdline | cut -d '=' -f 2
}

# Function to parse VFIO PCI-IDs from the kernel command line
# Return     : PCI-IDs of the kernel command line as space separated string
function kernel_cmdln_parse_pci_ids() {
	local passthrough_pci_ids="$(kernel_cmdln_parse_option "vfio-pci.ids")"
	echo "${passthrough_pci_ids}" | tr ',' ' '
}
