#!/bin/ash

# Cannot lock guest session, as the user probably doesn't know
# demo's password
if [ "$GUEST_SESSION" = "True" ]; then
	xmessage "Gastsitzungen koennen nicht gesperrt werden."
	exit 0
fi

# Browser-based login sessions cannot be unlocked for technical reasons,
# so disallow locking
[ -z "$UID" ] && UID="$( id -u )"
usrname="$( < "/etc/passwd"  awk -v "uid=$UID" -F ':' '$3 == uid {print $5; exit}' )"
# Yes, this really checks if $usrname ends in @browser, and sets NEVER_LOCK to true if so
if [ "${usrname%"@browser"}" != "${usrname}" ]; then
	xmessage "Web-Basierte Logins koennen die Sitzung leider nicht sperren."
	exit 0
fi

# Don't allow locking in exam mode
. /opt/openslx/config
if [ -n "$SLX_EXAM" ]; then
	xmessage "Im Pruefungsmodus nicht moeglich"
	exit 0
fi

# Any mouse-ungrab logic is embedded in our modded xscreensaver now,
# via external ungrab script

xscreensaver-command --lock

