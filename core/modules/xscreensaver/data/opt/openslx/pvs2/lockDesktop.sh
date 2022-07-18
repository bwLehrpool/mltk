#!/bin/ash

[ -z "$UID" ] && UID="$( id -u )"

usrname="$( < "/etc/passwd"  awk -v "uid=$UID" -F ':' '$3 == uid {print $5; exit}' )"
# Yes, this really checks if $usrname ends in @browser, and sets NEVER_LOCK to true if so
if [ "${usrname%"@browser"}" != "${usrname}" ]; then
	xmessage "Web-Basierte Logins koennen die Sitzung leider nicht sperren."
	exit 0
else
	. /opt/openslx/config
	if [ -n "$SLX_EXAM" ]; then
		xmessage "Im Klausurmodus nicht moeglich"
		exit 0
	fi
fi

# Any mouse-ungrab logic is embedded in our modded xscreensaver now,
# via external ungrab script

xscreensaver-command --lock

