#!/bin/ash

# Stuff we might wanna know
PWENT=
[ -n "$UID" ] && PWENT=`getent passwd "$UID"`
[ -z "$PWENT" ] && [ -n "$PAM_USER" ] && PWENT=`getent passwd "$PAM_USER"`
if [ -n "$PWENT" ]; then
	export USER=`echo "$PWENT" | awk -F ':' '{print $1}'`
	export GID=`echo "$PWENT" | awk -F ':' '{print $4}'`
	export HOME=`echo "$PWENT" | awk -F ':' '{print $6}'`
	export GROUP=`id -gn`
	export LOGNAME=$USER
	export HOSTNAME=`hostname`
fi
