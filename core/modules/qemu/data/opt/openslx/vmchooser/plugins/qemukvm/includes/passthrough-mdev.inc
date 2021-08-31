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
# Utils and functions to setup mediated device passthrough (Intel GVT-g)
# -----------------------------------------------------------------------------

# Function to get state of passthrough
# Return     : 0 (true) if passthrough is enabled, otherwise 1 (false)
function passthrough_mdev_enabled() {
	local passthrough_iommu="$(kernel_cmdln_parse_option "iommu")"
	local intel_gvt="$(kernel_cmdln_parse_option "i915.enable_gvt")"
	if [ "${passthrough_iommu}" == "pt" ] && [ "${intel_gvt}" == "1" ]; then
		return 0
	else
		return 1
	fi
}

# Function to check validitiy of the passthrough configuration
# Return     : 0 (true) if passthrough is configured properly, otherwise 1 (false)
function passthrough_mdev_check() {
	local passthrough_iommu_intel="$(kernel_cmdln_parse_option "intel_iommu")"
	local passthrough_iommu_amd="$(kernel_cmdln_parse_option "amd_iommu")"
	if [ "${passthrough_iommu_intel}" != "on" ] && [ "${passthrough_iommu_amd}" != "on" ]; then
		return 1
	else
		return 0
	fi
}

# Function to list all physical devices that support mediated device passthrough
# Return     : list of PCI addresses from devices that support mediated device passthrough
function passthrough_mdev_devices() {
	mdevctl types --dumpjson | jq -r ".[] | keys | .[]"
}

# Function to check if a physical device, that supports mediated device passthrough, is an Intel integrated GPU
# Parameter 1: PCI address of the physical device that supports mediated device passthrough
# Return     : 0 (true) if physical device is an Intel integrated GPU, otherwise 1 (false)
function passthrough_mdev_device_is_intel_igd() {
	local pt_mdev_device_pci_info="$(lspci -vmm -n -s "${1}")"
	local pt_mdev_device_pci_vendor="$(awk '$1 = /^Vendor:/ {print $2}' <<< "${pt_mdev_device_pci_info}")"
	local pt_mdev_device_pci_class="$(awk '$1 = /^Class:/ {print $2}' <<< "${pt_mdev_device_pci_info}")"
	if [ "${pt_mdev_device_pci_vendor}" = "8086" ] && [ "${pt_mdev_device_pci_class}" = "0300" ]; then
		return 0;
	else
		return 1;
	fi
}

# Function to get the first physical device that supports mediated device passthrough and is an Intel integrated GPU
# Return     : PCI address of the physical device that supports mediated device passthrough and is an Intel integrated GPU
function passthrough_mdev_device_get_intel_igd() {
	local pt_mdev_devices_addresses=($(passthrough_mdev_devices))
	for pt_mdev_device_address in ${pt_mdev_devices_addresses[@]}; do
		# check if device is an Intel integrated GPU
		if passthrough_mdev_device_is_intel_igd "${pt_mdev_device_address}"; then
			echo "${pt_mdev_device_address}"
			return 0;
		fi
	done

	return 1;
}

# Function to list all mediated device instance types of a specified physical device
# Parameter 1: PCI address of the physical device that supports mediated device passthrough
# Return     : list of mediated device instance types supported by the specified physical device
function passthrough_mdev_types() {
	mdevctl types --dumpjson | jq -r ".[] | .\"${1}\" | .[] | keys | .[]"
}

# Function to get the first mediated device instance type of a specified physical device
# Parameter 1: PCI address of the physical device that supports mediated device passthrough
# Return     : first mediated device instance type supported by the specified physical device
function passthrough_mdev_type_get() {
	local pt_mdev_device_types=($(passthrough_mdev_types "${1}"))
	if [ "${#pt_mdev_device_types[@]}" -gt 0 ]; then
		echo "${pt_mdev_device_types[0]}"
		return 0;
	else
		return 1;
	fi
}

# Function to get the description of a specified mediated device instance type
# Parameter 1: PCI address of the physical device that supports mediated device passthrough
# Parameter 2: mediated device instance type supported by the specified physical device
# Return     : description of the mediated device instance type supported by the specified physical device
function passthrough_mdev_type_description() {
	mdevctl types --dumpjson | jq -r ".[] | .\"${1}\" | .[] | select(.\"${2}\") | .[] | .\"description\""
}

# Function to get the available instance count of a specified mediated device instance type
# Parameter 1: PCI address of the physical device that supports mediated device passthrough
# Parameter 2: mediated device instance type supported by the specified physical device
# Return     : available instances of the mediated device instance type supported by the specified physical device
function passthrough_mdev_type_num_instances() {
	mdevctl types --dumpjson | jq -r ".[] | .\"${1}\" | .[] | select(.\"${2}\") | .[] | .\"available_instances\""
}

# Function to create a new mediated device instance
# Parameter 1: PCI address of the physical device that supports mediated device passthrough
# Parameter 2: mediated device instance type supported by the specified physical device
# Parameter 3: UUID for the created mediated device instance
function passthrough_mdev_instance_create() {
	mdevctl start --parent "${1}" --type "${2}" --uuid "${3}"
}

# Function to delete an existing mediated device instance
# Parameter 1: UUID of the existing mediated device instance
function passthrough_mdev_instance_delete() {
	mdevctl stop --uuid "${1}"
}

# Function to obtain mediated device UUID of a globally created mediated device (Intel GVT-g) instance for passthrough
# Return     : mediated device UUID of a globally created mediated device (Intel GVT-g) instance
function passthrough_mdev_instance_get() {
	local passthrough_mdev_instances=($(mdevctl list | cut -d ' ' -f 1))
	if [ "${#passthrough_mdev_instances[@]}" -eq 1 ]; then
		echo "${passthrough_mdev_instances[0]}"
		return 0;
	else
		return 1;
	fi
}

# Function to setup mediated device passthrough
# Return     : mediated device ID of the specified and configured mediated device (Intel GVT-g instance) for passthrough
function passthrough_mdev_setup() {
	# check if passthrough is enabled
	if passthrough_mdev_enabled; then
		# check if passthrough is configured properly
		if ! passthrough_mdev_check; then
			return 1;
		fi

		# obtain created mediated device for passthrough
		local pt_gpu_mdev_id="$(passthrough_mdev_instance_get)"

		# check if mediated device ID could be obtained
		if [ -z "${pt_gpu_mdev_id}" ]; then
			return 2;
		fi

		echo "${pt_gpu_mdev_id}"
	fi

	return 0;
}
