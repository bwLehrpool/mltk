###################################
# qemu/kvm include: Network setup #
###################################
# This now makes use of the qemu's bridge helper
# which creates a tap device and adds it to the
# bridge corresponding to the network type
# TODO configurable network type
setup_network() {
	# list available models with:
	#		qemu-system-x86_64 -net nic,model=?
	# e.g. as of 2.0.0:
	#		ne2k_pci,i82551,i82557b,i82559er,rtl8139,e1000,pcnet,virtio
	declare -rg NIC_MODEL="e1000"

	# add MAC address and network card model
	VIRTCMDOPTS+=( "-device" "${NIC_MODEL},mac=${VM_MAC_ADDR},netdev=guestnet0" )

	# TODO support different network kinds for lectures in bwlehrpool-suite, just NAT for now
	declare -g NETWORK_MODE="nat"

	# detect if qemu's bridge helper binary is available
	declare -g QEMU_BRIDGE_HELPER=
	for HELPER_PATH in /usr/lib/qemu-bridge-helper /usr/local/libexec/qemu-bridge-helper; do
		if [ -x "${HELPER_PATH}" ] && [ -u "${HELPER_PATH}" ]; then
			QEMU_BRIDGE_HELPER="${HELPER_PATH}"
			readonly QEMU_BRIDGE_HELPER
			break
		fi
	done
	if isempty QEMU_BRIDGE_HELPER; then
		writelog "Could not find qemu-bridge-helper on this machine. Setting network mode to user."
		# Even though falling back to creating tap devices ourselves, we should instead
		# garantee the existance of qemu's helper on minilinux build time.
		# qemu's user network mode allows tcp/udp connections in a nat-fashion and
		# it allows access to the web which seems suffisant for a fallback.
		NETWORK_MODE="user"
	fi

	case "${NETWORK_MODE}" in
	nat*)
		VIRTCMDOPTS+=( "-netdev" "bridge,br=nat1,id=guestnet0,helper=${QEMU_BRIDGE_HELPER}" )
		;;
	bridge*)
		VIRTCMDOPTS+=( "-netdev" "bridge,br=br0,id=guestnet0,helper=${QEMU_BRIDGE_HELPER}" )
		;;
	host*)
		VIRTCMDOPTS+=( "-netdev" "bridge,br=vsw2,id=guestnet0,helper=${QEMU_BRIDGE_HELPER}" )
		;;
	user*|*)
		VIRTCMDOPTS+=( "-netdev" "user,id=guestnet0" )
		;;
	esac
}

## MAIN ##
call_post_source setup_network
