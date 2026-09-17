#!/bin/bash

jqitmp=$( mktemp )
trap 'rm -f -- "$jqitmp"' EXIT

jqi() {
	< "/etc/firefox/policies/policies.json" jq "$@" > "$jqitmp"
	cat "$jqitmp" > "/etc/firefox/policies/policies.json"
}

# Custom cert so satellite.bwlehrpool works with HTTPS
for i in /etc/ssl/certs/ca-*.crt; do
	[ -s "$i" ] || continue
	jqi --arg file "$i" '.policies.Certificates.Install += [$file]'
done

exit 0
