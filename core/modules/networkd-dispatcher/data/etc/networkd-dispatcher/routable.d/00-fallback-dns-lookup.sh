#!/bin/bash
#
# This script is meant as a hook in /etc/networkd-dispatcher/routable.d
# It receives information on the state of the interface per environment,
# in particular:
# * $IFACE -> network interface that triggered this script
# * $ADDR  -> IP address configured on that interface

check_dns() {
	local fqdn host_out
	for timeout in 1 1 1 2 2 3 5 5 END; do
		if [ "$timeout" = "END" ]; then
			echo "Falling back to IP address if needed."
			fqdn=
			break
		fi
		host_out="$(host $ADDR)"
		if [ $? -ne 0 ]; then
			echo "Failed to retrieve FQDN through reverse lookup on '$ADDR'."
			sleep $timeout
		else
			local fqdn="$(<<< $host_out grep -E "$(awk -F. '{print $4"."$3"."$2"."$1}' <<< $ADDR )" | awk '{sub(".$",""); print $5}')"
			break
		fi
	done

	# check domain
	local domain="${fqdn#*.}"
	if [ -n "$domain" ] && ! grep -qE "^(search|domain).*$domain.*" /etc/resolv.conf; then
		echo "Adding DNS domain '$domain' to systemd-resolved configuration"
		mkdir -p /etc/systemd/resolved.conf.d 
		cat <<- EOF > /etc/systemd/resolved.conf.d/dns-domain.conf
			[Resolve]
			Domains=$domain
		EOF
		systemctl restart systemd-resolved
	fi
	# Check if we received a hostname from the DHCP
	local dhcp_hostname
	for lease in /run/systemd/netif/leases/*; do
		grep -qE "^ADDRESS=$ADDR" "$lease" || continue 
		dhcp_hostname="$(awk -F= '$1 = /HOSTNAME/ {print $2}' "$lease" )"
		break
	done
	if [ -n "$dhcp_hostname"  ]; then
		# Prefer DHCP hostname over anything else
		if [ "$dhcp_hostname" != "$(hostname)" ]; then
			echo "Current hostname differs from DHCP, forcing DHCP hostname: '$dhcp_hostname'"
			echo "systemd-networkd should have set it but did not. Check your configuration."
			hostnamectl set-hostname "$dhcp_hostname"
		fi
		return 0
	fi

	# No DHCP hostname, check DNS hostname
	local dns_hostname="${fqdn%%.*}"
	if [ -n "$dns_hostname" ]; then
		if [ "$dns_hostname" != "$(hostname)" ]; then
			echo "Current hostname differs from DNS, forcing DNS hostname: '$dns_hostname'"
			hostnamectl set-hostname "$dns_hostname"
		fi
	else
		echo "Neither DHCP nor DNS provided a hostname, use IP address as fallback."
		hostnamectl set-hostname "${ADDR//./-}"
	fi
	return 0
}

if [ ! -e /opt/openslx/config ]; then
	echo "No OpenSLX configuration found - aborting."
	exit 1
fi

. /opt/openslx/config

if [ "$IFACE" != "${SLX_BRIDGE:-$SLX_PXE_NETIF}" ]; then
	echo "Ignoring $IFACE."
	exit 0
fi

# Wait until the interface is fully configured to make sure DNS lookups will work
# This works around the cases of a routable but configuring state which makes not sense at all...
for timeout in 1 1 1 2 2 3 5 5 END; do
	if [ "$timeout" = "END" ]; then
		echo "$IFACE did reach routable and configured state within 20sec, giving up..."
		exit 1
	fi
	if networkctl status "$IFACE" | grep -qE "^\s*State:\s*routable\s*\(configured\)"; then
		echo "$IFACE routable and configured, going on..."
		break
	fi
	sleep "$timeout"
done

sleep 1

check_dns
