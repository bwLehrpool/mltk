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
# Utils and functions to setup PCI passthrough
# -----------------------------------------------------------------------------

# Function to get state of passthrough
# Return     : 0 (true) if passthrough is enabled, otherwise 1 (false)
function passthrough_pci_enabled() {
	local passthrough_iommu="$(kernel_cmdln_parse_option "iommu")"
        local passthrough_pci_ids=($(kernel_cmdln_parse_pci_ids))
	if [ "${passthrough_iommu}" == "pt" ] && ["${#passthrough_pci_ids[@]}" -gt 0 ]; then
		return 0
	else
		return 1
	fi
}

# Function to check validitiy of the passthrough configuration
# Return     : 0 (true) if passthrough is configured properly, otherwise 1 (false)
function passthrough_pci_check() {
	local passthrough_iommu_intel="$(kernel_cmdln_parse_option "intel_iommu")"
	local passthrough_iommu_amd="$(kernel_cmdln_parse_option "amd_iommu")"
	if [ "${passthrough_iommu_intel}" != "on" ] && [ "${passthrough_iommu_amd}" != "on" ]; then
		return 1
	else
		return 0
	fi
}

# Function to get PCI address of a PCI device specified by its vendor and product ID
# Parameter 1: vendor and product ID with the following pattern: "<VENDOR ID>:<PRODUCT ID>"
# Return     : PCI address of the specified PCI device of form "<PCI DOMAIN>:<PCI BUS>:<PCI DEVICE>.<PCI FUNCTION>"
function passthrough_pci_lookup_address() {
	local passthrough_pci_addresses="$(lspci -n -D -d "${1}" | cut -d ' ' -f 1)"
	passthrough_pci_addresses=($(echo "${passthrough_pci_addresses}" | tr '\n' ' '))
	if [ "${#passthrough_pci_addresses[@]}" -eq 1 ]; then
		echo "${passthrough_pci_addresses[0]}"
		return 0
	else
		return 1
	fi
}

# Function to setup PCI passthrough
# Return     : PCI-IDs and addresses of the specified passthrough PCI devices as tuples of form
#              "<VENDOR ID>:<PRODUCT ID>,<PCI DOMAIN>:<PCI BUS>:<PCI DEVICE>.<PCI FUNCTION>"
function passthrough_pci_setup() {
        # check if passthrough is enabled
        if passthrough_enabled; then
		# check if passthrough is configured properly
		if ! passthrough_check; then
			return 1;
		fi

                writelog "Passthrough of PCI devices is enabled successfully"

		# parse PCI-IDs from the kernel command line
		local passthrough_pci_ids=($(kernel_cmdln_parse_pci_ids))

		# lookup PCI address of each specified PCI-ID
		local device_pci_address
		local pt_gpu_pci_ids
		for device_vendor_id in "${passthrough_pci_ids[@]}"; do
			device_pci_address="$(passthrough_pci_lookup_address "${device_vendor_id}")"
			if [ "${?}" -eq 0 ]; then
				pt_gpu_pci_ids+=("${device_vendor_id},${device_pci_address}")
			else
				writelog "Failed to lookup PCI address for '${device_vendor_id}'"
			fi
		done

                echo "${pt_gpu_pci_ids[@]}"
        fi

        return 0;
}
