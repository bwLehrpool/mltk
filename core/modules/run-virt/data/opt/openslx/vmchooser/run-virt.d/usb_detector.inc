#!/bin/bash
##################################################
# Include: Detect and pass USB devices to the VM #
##################################################

# 2:* Communication devices, 2:6 is Ethernet
# 239 Misc
declare -g PASSTHROUGH_USB_DEVICES="2:2 2:13 2:14 0:5 0:6 0:7 0:14 0:16 0:17 239"
if ! is_on "$SLX_BRIDGE_OTHER_NICS" && ! is_on "$SLX_DHCP_OTHER_NICS"; then
	PASSTHROUGH_USB_DEVICES+=" 2:6"
fi

# include USB IDs explicitly given by SLX_PASSTHROUGH_USB_ID
# which is expected to be a space-separated list!
if [ -n "${SLX_PASSTHROUGH_USB_ID}" ]; then
	while IFS= read -ra USB_ID; do
		for ID in ${USB_ID[@]}; do
			# validate each entry and add them to the list if they pass
			if ! [[ "${ID}" =~ ^[0-9a-f]{4}:[0-9a-f]{4}$ ]]; then
				writelog "Rejected USB ID: '${ID}'."
				continue
			fi
			PASSTHROUGH_USB_DEVICES+=" ${ID}"
		done
	done <<< "${SLX_PASSTHROUGH_USB_ID}"
fi
readonly PASSTHROUGH_USB_DEVICES

# Helper function that will scan /dev/bus/usb for devices matching specific classes
# and then output the corresponding device ids. This can be used for selective
# handover of devices to a virtual machine
#
# $1: expression to fill with device information.
#     valid placeholders are:
#     %VENDOR%  - device vendor id
#     %PRODUCT% - device product id
# $2-n: device classes to include in output
get_usb_devices_int() {
	[ -z "$TMPDIR" ] && TMPDIR="/tmp"
	local EXP=$1
	shift
	if [ -z "$EXP" ]; then
		writelog --quiet "No ouput expression template passed to get_usb_devices"
		EXIT_TYPE="internal" EXIT_REASON="Fehler beim Erkennen der USB-Geräte." cleanexit 1
	fi
	if [ $# -eq 0 ]; then
		writelog --quiet "No device classes given to get_usb_devices"
		cleanexit 1
	fi
	local MATCH=';'
	while [ $# -gt 0 ]; do
		MATCH+="$1;"
		[[ "$1" != *:* ]] && MATCH+="0:$1;"
		shift
	done
	local dev=
	local key value trailing
	trailing=
	local tmp="${TMPDIR}/lsusb.$$.$RANDOM"
	for dev in /dev/bus/usb/*/*; do
		if ! lsusb -D "$dev" > "$tmp" 2>/dev/null; then
			writelog --quiet "Cannot lsusb $dev"
			continue
		fi
		local DC=
		local OK=
		local VENDOR=
		local PRODUCT=
		while read -r key value trailing || [ -n "$key" ]; do
			if [[ "$key" == "idVendor" ]]; then
				[[ "$value" == 0x* ]] && VENDOR="${value:2}"
			elif [[ "$key" == "idProduct" ]]; then
				[[ "$value" == 0x* ]] && PRODUCT="${value:2}"
			elif [ -z "$DC" ]; then
				# No bDeviceClass seen yet
				if [[ "$key" == "bDeviceClass" ]]; then
					DC="$value"
					[[ "$MATCH" == *";${DC};"* ]] && OK=yo
				fi
			else
				# #DeviceClass is generic, look at sub class
				if [[ "$key" == "bInterfaceClass" ]]; then
					[[ "$MATCH" == *";${DC}:${value};"* ]] && OK=yo
				fi
			fi
			if [ -n "$OK" -a -n "$VENDOR" -a -n "$PRODUCT" ] || \
				[[ "$MATCH" == *";${VENDOR}:${PRODUCT};"* ]]; then
				echo "$EXP" | sed "s/%VENDOR%/${VENDOR}/g;s/%PRODUCT%/${PRODUCT}/g"
				break
			fi
		done < "$tmp"
	done
	rm -f -- "$tmp"
}

get_usb_devices() {
	if which lsusb 2>/dev/null >&2 && lsusb --help 2>&1 | grep -q -- '-D' 2>/dev/null; then
		[ $# -eq 1 ] && set -- "$1" $PASSTHROUGH_USB_DEVICES # no quotes here!
		get_usb_devices_int "$@" | sort -u
	else
		writelog --quiet "Cannot scan usb bus: lsusb not found or doesn't support -D"
	fi
}

# kinda of a special case since we only define functions
# the handler will only check if the functions are properly defined
check_usb_detector() {
	if ! is_function get_usb_devices get_usb_devices_int; then
		# this should practically never happen
		writelog "'get_usb_devices[_int]' functions are not defined!"
		return 1
	fi
	return 0
}
reg_feature_handler "usb" "check_usb_detector"
