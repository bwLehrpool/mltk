#!/bin/bash
############################################
# Feature: Setup firewall rules for the VM #
############################################
## Functions
setup_firewall () {
	local LOGF="${TMPDIR}/firewall.log"
	local RET
	[ "$DISPLAY" = ":0" ] || return 0 # For now, to avoid conflicts, we only do this on display :0
	# dnsmasq.conf
	declare -rg DNSMASQ_CONF="$TMPDIR/dnsmasq.$RANDOM.conf"
	if ! touch "$DNSMASQ_CONF"; then
		slxlog "virt-firewall" "Error creating temporary config file for dnsmasq"
		return 1
	fi
	# Get free port
	local port try
	port=
	while [ -z "$port" ]; do
		try=$(( RANDOM % 40000 + 10000 ))
		( netstat -tuln || ss -tuln ) | grep -qP ":$port\\s" && continue
		port="$try"
	done
	# Run iptables helper
	slxfwtool "$IMGUUID" "$DNSMASQ_CONF" "$port" &> "$LOGF"
	RET=$?
	if [ "$RET" != "0" ]; then
		slxlog "virt-firewall" "Error setting up firewall rules for lecture $IMGUUID (Exit code $RET)" "$LOGF"
		return 1
	fi
	# Run dnsmasq if applicable
	if [ -s "$DNSMASQ_CONF" ]; then
		cat >> "$DNSMASQ_CONF" <<-DNSCONF
		keep-in-foreground
		pid-file=/tmp/dns-$RANDOM.$RANDOM.$RANDOM
		no-hosts
		no-resolv
		port=$port
		interface=lo
		bind-interfaces
		DNSCONF
		add_cleanup "cleanup_firewall"
		if ! dnsmasq --test --conf-file "$DNSMASQ_CONF" &> "${DNSMASQ_CONF}.tmp"; then
			cat "${DNSMASQ_CONF}.tmp" >> "${DNSMASQ_CONF}"
			rm -f -- "${DNSMASQ_CONF}.tmp"
			slxlog -s -d "virt-firewall" "Invalid dnsmasq.conf was generated" "$DNSMASQ_CONF"
			return 1
		fi
		# All seems well, launch for real
		run_dnsmasq_fw "$port"
		add_cleanup "cleanup_firewall"
	fi
	return 0
}

run_dnsmasq_fw () {
	(
	trap 'exit 0' INT TERM
	trap 'kill "$dnspid"' EXIT
	while [ -s "$DNSMASQ_CONF" ]; do
		dnsmasq --conf-file "$DNSMASQ_CONF" &
		dnspid=$!
		wait "$dnspid"
	done
	) &
	declare -rg DNSMASQ_PID=$!
}

cleanup_firewall () {
	rm -f -- "$DNSMASQ_CONF"
	kill "$DNSMASQ_PID"
	writelog "Killed dnsmasq"
}

## MAIN ##
# Sanity checks
if check_dep slxfwtool; then
	reg_feature_handler "firewall" "setup_firewall"
else
	writelog "Could not find 'slxfwtool' in PATH: $PATH. Netrules firewall will not work!"
	error_user "Fehler" "Keine Unterstützung für veranstaltungspezifische Firewall-Regeln!"
fi
