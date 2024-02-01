#!/bin/bash

ua_set_vars () {
	ua_set_vars () {
		:
	}
	if [ -z "$MLTK_INSTALL" ]; then
		# regular mltk mode, copy the current user-related files to TARGET_BUILD_DIR
		if [ -z "$TARGET_BUILD_DIR" ] || [ "$TARGET_BUILD_DIR" == "/" ]; then
			perror "Almost wrecked your local passwd, group and shadow file. phew."
		fi
		local prefix=0
		command useradd --help 2>&1 | grep -q -- "--prefix" && prefix=1
		declare -rg _PASSWD="${TARGET_BUILD_DIR}/etc/passwd"
		declare -rg _GROUP="${TARGET_BUILD_DIR}/etc/group"
		declare -rg _SHADOW="${TARGET_BUILD_DIR}/etc/shadow"
		if (( prefix == 0 )); then
			declare -rga _USER_EXTRA_OPTS=( "--root" "$TARGET_BUILD_DIR" )
		else
			declare -rga _USER_EXTRA_OPTS=( "--prefix" "$TARGET_BUILD_DIR" )
		fi
		# prepare files
		[ -s "${_PASSWD}" ] && [ -s "${_GROUP}" ] && [ -s "${_SHADOW}" ] && return
		pinfo "Creating users and groups based on local system...."
		mkdir -p "${_PASSWD%/*}" || perror "Could not mkdir '${_PASSWD%/*}'."
		cp -a "/etc/passwd" "$_PASSWD" || perror "Could not copy /etc/passwd"
		cp -a "/etc/group" "$_GROUP" || perror "Could not copy /etc/group"
		# remove local users from group file (TODO: currently assumes users have ids 1000-1999)
		local LOCALUSERS=$( awk -F ':' '$3 >= 1000 && $3 < 2000 {print $1}' "${_PASSWD}" )
		local USER
		for USER in $LOCALUSERS; do
			sed -r -i "s/([:,])${USER}(,|$)/\1/g;s/,$//" "${_GROUP}"
		done
		# remove all non-system groups (also assumes users have 1000-1999, so nogroup will be kept)
		awk -F ':' '$3 < 1000 || $3 >= 2000' "${_GROUP}" > "${_GROUP}.tmp"
		mv -f "${_GROUP}.tmp" "${_GROUP}" || perror "Move error ($_GROUP)"
		# same for users...
		awk -F ':' '$3 < 1000 || $3 >= 2000' "${_PASSWD}" > "${_PASSWD}.tmp"
		mv -f "${_PASSWD}.tmp" "${_PASSWD}" || perror "Move error ($_PASSWD)"
		# generate fresh shadow file
		awk -F ':' '{print $1":*:15555:0:99999:7:::"}' "${_PASSWD}" > "${_SHADOW}"
		# make sure shadow has group shadow
		chgrp shadow "$_SHADOW"
		chmod 0640 "$_SHADOW"
		if (( prefix == 0 )); then
			# all user-related tools that support "--root" option require nss libs in the
			# chroot target, thus we need to copy the libs over there.
			tarcopy "$(find /lib /lib64 /usr/lib /usr/lib64 -maxdepth 4 -name "libnss_files*")" "$TARGET_BUILD_DIR"
		fi
	else
		declare -rg _PASSWD="/etc/passwd"
		declare -rg _GROUP="/etc/group"
		declare -rg _SHADOW="/etc/shadow"
		declare -rga _USER_EXTRA_OPTS=()
	fi
}

useradd () {
	ua_set_vars
	declare -a opts=( "$@" )
	local _uid _gid name _nuid
	local sys=0
	local uid=
	local gid=
	local mkgrp=0
	local pw="#"
	while [ $# -gt 0 ]; do
		name="$1"
		case "$1" in
			--root|-R) perror "Must not use --root" ;;
			--prefix|-P) perror "Must not use --prefix" ;;
			--system|-r) sys=1 ;;
			--uid|-u) uid="$2"; shift ;;
			--gid|-g) gid="$2"; shift ;;
			--user-group|-U) mkgrp=1 ;;
			--password|-p) pw="$2"; shift ;;
		esac
		shift
	done
	# Existing user id, if any
	_nuid="$( getuid "$name" )"
	# Group checks
	if (( mkgrp == 1 )); then
		[ -n "$gid" ] && perror "$name: Cannot use -U and -g at the same time"
		_gid="$( getgid "$name" )"
	elif [ -n "$gid" ]; then
		_gid="$( getgid "$gid" )"
		[ -z "$_gid" ] && perror "$name: Cannot create user and add to $gid: Group not found"
	fi
	# User checks
	if [ -n "$uid" ]; then
		_uid="$( getuid "$uid" )"
		[ -n "$_uid" ] && [ "$_uid" != "$uid" ] && perror "$name: Requested uid already taken"
		[ -n "$_nuid" ] && [ "$_nuid" != "$uid" ] && perror "$name: User already exists with $_nuid"
	elif (( sys == 1 )) && [ -n "$_nuid" ] && (( _nuid >= 1000 )); then
		perror "$name: Requested as system user, but already has $_nuid"
	elif (( sys == 0 )) && [ -n "$_nuid" ] && (( _nuid < 1000 )); then
		perror "$name: Requested as normal user, but already has $_nuid"
	fi
	if [ -n "$_nuid" ]; then
		[ -n "$_gid" ] && [ "$( getusergroup "$name" )" != "$_gid" ] && perror "$name: Exists with group != $_gid"
		[ "$pw" != "#" ] && usr_setpw "$name" "$pw"
		return 0 # Nothing to do
	fi
	command useradd "${_USER_EXTRA_OPTS[@]}" "${opts[@]}" || perror "useradd ${opts[*]} failed"
	[ "$pw" != "#" ] && usr_setpw "$name" "$pw"
	true
}

groupadd () {
	ua_set_vars
	declare -a opts=( "$@" )
	local name _ngid
	local sys=0
	local gid=
	while [ $# -gt 0 ]; do
		name="$1"
		case "$1" in
			--root|-R) perror "Must not use --root" ;;
			--prefix|-P) perror "Must not use --prefix" ;;
			--system|-r) sys=1 ;;
			--gid|-g) gid="$2"; shift ;;
		esac
		shift
	done
	# Existing group id, if any
	_ngid="$( getgid "$name" )"
	if [ -n "$gid" ]; then # Want specific gid
		_gid="$( getgid "$gid" )"
		if [ -n "$_gid" ]; then # Requested gid exists
			[ -n "$_ngid" ] && (( _gid == _ngid )) && return 0 # Nothing to do
			perror "$name: gid $_gid already taken"
		fi
	elif [ -n "$_ngid" ]; then
		(( sys == 1 && _ngid >= 1000 )) && perror "$name: Already exists with non-system id $_ngid"
		(( sys == 0 && _ngid < 1000 )) && perror "$name: Already exists with system id $_ngid"
		return 0
	fi
	command groupadd "${_USER_EXTRA_OPTS[@]}" "${opts[@]}" || perror "groupadd ${opts[*]} failed"
	true
}

# Add user to given group. Not entirely safe as we don't wait for a lock,
# but should not be an issue during build time as system is usually offline.
# $1=user $2=group
add_to_group () {
	ua_set_vars
	grep -qP "^${2}:.*:.*[:,]${1}(,|$)" "$_GROUP" && return 0
	grep -q  "^${2}:" "$_GROUP" || perror "Cannot add '$1' to '$2' - no such group"
	sed -i -r "s/^${2}:[^:]*:[^:]*:.+$/&,$1/;s/^${2}:[^:]*:[^:]*:$/&$1/" "$_GROUP"
}

# Get numeric ID of given user (name or ID)
getuid () {
	ua_set_vars
	awk -F ':' -v p="$1" -v n= '{if ($1 == p) n = $3; if (n == "" && $3 == p) n = $3;}END{if (length(n)) print n}' "$_PASSWD"
}

# Get numeric ID of given group (name or ID)
getgid () {
	ua_set_vars
	awk -F ':' -v p="$1" -v n= '{if ($1 == p) n = $3; if (n == "" && $3 == p) n = $3;}END{if (length(n)) print n}' "$_GROUP"
}

# Get numeric ID of primary group of given user (name or ID)
getusergroup () {
	ua_set_vars
	awk -F ':' -v p="$1" -v n= '{if ($1 == p) n = $4; if (n == "" && $4 == p) n = $3;}END{if (length(n)) print n}' "$_PASSWD"
}

# usr_setpw username password
usr_setpw () {
	local PW=
	local pw="$2"
	if [ -z "$pw" ]; then
		PW="*"
	elif [ "${pw:0:1}" != '$' ] || [ "${pw:2:1}" != '$' ]; then
		PW="$( mkpasswd -m sha-512 "${pw}" )"
		[ -z "${PW}" ] && PW="$( openssl passwd -6 "${pw}" )"
		[ -z "${PW}" ] && PW="$( openssl passwd -1 "${pw}" )"
		[ -z "${PW}" ] && perror "Error generating hashed password for $1"
	else
		PW="$pw"
	fi
	local s r
	s="$( sed -r 's~[\[{(*+/^$?\\]~\\&~g' <<<"$1" )"
	r="$( sed -r 's~[&/\\]~\\&~g' <<<"$1:$PW" )"
	sed -i -r "s/$s:[^:]*:/$r:/" "${_SHADOW}"
}
