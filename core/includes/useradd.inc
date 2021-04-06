# This helper is stupid because it reimplements what useradd etc. do.
# They could do the job just fine by using the -R option - maybe change this
# dome day.
#

# Add a user to the system
#
# Usage:
# either do ID_OF_REQUESTED_USER=$(add_user herbert)
# or ID_OF_USER=$(USER=herbert GROUP=somegroup PASSWORD=secret123 add_user)
# Valid params are:
# USER, GROUP, USERID, GROUPID, PASSWORD, USERHOME, USERSHELL
# defaults are no password, home=/nonexistent, usershell=/bin/false
# IDs will be generated in the range of 5-999 if not explicitly given
# TODO: Make it possible to pass a range of IDs if you don't want one <1000 but don't care about the exact ID

declare -rg NAME_REGEX='^[a-z][-a-z0-9]*$'

# Generate a UID for a given USERNAME. Return existing UID if possible, generate new one otherwise
generate_uid()
{
	[ $# -ne 1 ] && perror "generate_uid fail. want 1 argument."
	[ -z "${_PASSWD}" ] && perror "passwd file not set."
	local _UID=$(grep -E "^$1:[^:]*:[0-9]+:" "${_PASSWD}" | head -1 | awk -F ':' '{print $3}')
	if [ "x${_UID}" = "x" ]
	then
		local _TRIES=0
		while [ ${_TRIES} -lt 50 ]
		do
			_TRIES=$[ ${_TRIES} + 1 ]
			_UID=$[ 5 + $RANDOM % 900 ]
			local _TEST=$(grep -E "^[^:]+:[^:]*:${_UID}:" "${_PASSWD}")
			[ "x${_TEST}" = "x" ] && break
		done
		[ ${_TRIES} -ge 50 ] && perror "Generating a UID failed."
	fi
	echo ${_UID}
}

# Echo gid of given user if existent, nothing otherwise
get_gid_for_user()
{
	[ $# -ne 1 ] && perror "get_gid_for_user fail. want 1 argument."
	[ -z "${_PASSWD}" ] && perror "passwd file not set."
	local _GID=$(grep -E "^$1:[^:]*:[^:]*:[0-9]+:" "${_PASSWD}" | head -1 | awk -F ':' '{print $4}')
	echo ${_GID}
}

# Echo group name of given gid, nothing if non-existent
get_group_for_gid()
{
	[ $# -ne 1 ] && perror "get_group_for_gid fail. want 1 argument."
	[ -z "${_GROUP}" ] && perror "group file not set."
	local _NAME=$(grep -E "^[^:]*:[^:]*:$1:" "${_GROUP}" | head -1 | awk -F ':' '{print $1}')
	echo ${_NAME}
}

# Generate a GID for a given GROUPNAME. Return existing GID if possible, generate new one otherwise
generate_gid()
{
	[ $# -ne 2 ] && perror "generate_gid fail. want 2 arguments."
	[ -z "${_GROUP}" ] && perror "group file not set."
	local _GID=$(grep -E "^$1:[^:]*:[0-9]+:" "${_GROUP}" | head -1 | awk -F ':' '{print $3}')
	if [ "x${_GID}" = "x" ]
	then
		# group does not exist, try to create
		local _TRIES=0
		_GID=$2 # try to use uid as gid if not taken yet
		while [ ${_TRIES} -lt 50 ]
		do
			_TRIES=$[ ${_TRIES} + 1 ]
			local _TEST=$(grep -E "^[^:]+:[^:]*:${_GID}:" "${_GROUP}")
			[ "x${_TEST}" = "x" ] && break
			_GID=$[ 5 + $RANDOM % 900 ] # using uid as gid not possible, generate new one
		done
		[ ${_TRIES} -ge 50 ] && perror "Generating a GID failed."
	fi
	echo ${_GID}
}

add_user() {
	[ -z "${TARGET_BUILD_DIR}" ] && perror "add_user: TARGET_BUILD_DIR not set"
	if [ -z "${USER}" -a $# -eq 0 ]
	then
		pwarning " ** add_user usage **"
		pwarning "add_user <username>"
		pwarning "OR"
		pwarning "USER=<username> [GROUP=<groupname>] [USERID=<userid>] [GROUPID=<groupid>] [USERHOME=<homedir>] [USERSHELL=<shell>] [PASSWORD=<pass>] add_user"
		perror "Aborting, please fix your script."
	fi
	local _PASSWD=${TARGET_BUILD_DIR}/etc/passwd
	local _GROUP=${TARGET_BUILD_DIR}/etc/group
	local _SHADOW=${TARGET_BUILD_DIR}/etc/shadow
	init_users_and_groups
	[ ! -f "${_PASSWD}" ] && perror "add_user: password file does not exist in target system. (build base first)"
	[ ! -f "${_GROUP}" ] && perror "add_user: group file does not exist in target system. (build base first)"
	[ ! -f "${_SHADOW}" ] && perror "add_user: shadow file does not exist in target system. (build base first)"
	if [ "x$1" != "x" ]
	then
		local USER=$1
		local GROUP=""
		local USERID=""
		local GROUPID=""
		local USERHOME=""
		local USERSHELL=""
		local PASSWORD=""
	fi
	USER=$(trim "$USER")
	if ! [[ $USER =~ $NAME_REGEX ]]; then
		perror "Invalid username: $USER"
	fi
	[ -z "$GROUPID" ] && local GROUPID=$(get_gid_for_user "${USER}")
	[ -z "$GROUP" -a -n "$GROUPID" ] && local GROUP=$(get_group_for_gid "${GROUPID}")
	[ -z "$GROUP" ] && local GROUP=$USER
	GROUP=$(trim "$GROUP")
	if ! [[ $GROUP =~ $NAME_REGEX ]]; then
		perror "Invalid group: $GROUP"
	fi
	[ "x$USERID" = "x" ] && local USERID=$(generate_uid "${USER}")
	USERID=$(trim "$USERID")
	[ "$USERID" -lt "0" -o "$USERID" -gt "65535" ] && perror "Invalid userid: $USERID"
	[ -z "$GROUPID" ] && local GROUPID=$(generate_gid "${GROUP}" "${USERID}")
	GROUPID=$(trim "$GROUPID")
	[ "$GROUPID" -lt "0" -o "$GROUPID" -gt "65535" ] && perror "Invalid groupid: $GROUPID"
	# all required variables have been set
	# does the desired username already exist? if so, check if UID matches, otherwise bail out
	local _UID=$(grep -E "^${USER}:[^:]*:[0-9]+:" "${_PASSWD}" | head -1 | awk -F ':' '{print $3}')
	[ -n "${_UID}" ] && [ "x${_UID}" != "x${USERID}" ] && perror "User ${USER}(${USERID}) already exists with UID ${_UID}"
	# do the same for the group
	local _GID=$(grep -E "^${GROUP}:[^:]*:[0-9]+:" "${_GROUP}" | head -1 | awk -F ':' '{print $3}')
	[ -n "${_GID}" ] && [ "x${_GID}" != "x${GROUPID}" ] && perror "Group ${GROUP}(${GROUPID}) already exists with GID ${_GID}"
	# if user already exists, check if he is in another group than the one requested. if so, bail out
	# (TODO: don't bail out and add user to the new group)
	if [ ! -z "${_UID}" ]
	then
		local _EXGID=$(grep -E "^${USER}:[^:]*:[0-9]+:" "${_PASSWD}" | head -1 | awk -F ':' '{print $4}')
		[ "x${GROUPID}" != "x${_EXGID}" ] && perror "Requested GID $GROUPID differs from existing GID $_EXGID"
	fi
	# if user does not exist, try to add it
	if [ -z "${_UID}" ]
	then
		local _TEST=$(grep -E "^[^:]+:[^:]*:${USERID}:" "${_PASSWD}")
		[ -n "${_TEST}" ] && perror "Cannot add $USER - desired UID $USERID already in use."
	fi
	if [ -z "${_GID}" ]
	then
		local _TEST=$(grep -E "^[^:]+:[^:]*:${GROUPID}:" "${_GROUP}")
		[ -n "${_TEST}" ] && perror "Cannot add $GROUP - desired GID $GROUPID already in use."
	fi
	[ -z "${USERHOME}" ] && local USERHOME=/nonexistent
	[ -z "${USERSHELL}" ] && local USERSHELL=/bin/false
	# create password
	if [ -z "${PASSWORD}" ]
	then
		local PASSWORD='*'
	else
		pdebug "Hashing password '$PASSWORD' for '$USER'"
		local PW=$(mkpasswd -m sha-512 "${PASSWORD}")
		[ -z "${PW}" ] && PW=$(openssl passwd -1 "${PASSWORD}")
		[ -z "${PW}" ] && perror "Error generating hashed password for $USER"
		PASSWORD=$PW
	fi
	# add user, or replace password
	if [ -z "${_UID}" ]; then
		# create user
		echo "${USER}:x:${USERID}:${GROUPID}:${USER}:${USERHOME}:${USERSHELL}" >> "${_PASSWD}"
		echo "${USER}:${PASSWORD}:15555:0:99999:7:::" >> "${_SHADOW}"
		pinfo "Created user $USER"
	elif [ "$PASSWORD" != "*" ]; then
		# update user's password
		sed -i -r "s#^${USER}:[^:]*:(.*)\$"'#'"${USER}:${PASSWORD}:\1#g" "${_SHADOW}"
		pinfo "Updated password of $USER"
	fi
	[ -z "${_GID}" ] && pinfo "Created group $GROUP" && echo "${GROUP}:x:${GROUPID}:" >> "${_GROUP}"
	echo "${USERID}"
}

add_group () {
	[ $# -lt 1 ] && perror "add_group called without argument."
	[ -z "${TARGET_BUILD_DIR}" ] && perror "add_group: TARGET_BUILD_DIR not set"
	local _PASSWD=${TARGET_BUILD_DIR}/etc/passwd
	local _GROUP=${TARGET_BUILD_DIR}/etc/group
	local _SHADOW=${TARGET_BUILD_DIR}/etc/shadow
	init_users_and_groups
	[ ! -f "${_GROUP}" ] && perror "add_user: group file does not exist in target system. (build base first)"
	local GROUP=$1
	local GROUPID="-"
	if ! [[ $GROUP =~ $NAME_REGEX ]]; then
		perror "Invalid group: $GROUP"
	fi
	[ $# -ge 2 ] && [ ! -z "$2" ] && GROUPID=$2
	local _GID=$(grep -E "^${GROUP}:[^:]*:[0-9]+:" "${_GROUP}" | head -1 | awk -F ':' '{print $3}')
	[ "x${_GID}" != "x" ] && [ "x$GROUPID" = "x-" -o "x$GROUPID" = "x${_GID}" ] && echo "${_GID}" && return # nothing to do, already exists
	[ "x${_GID}" != "x" ] && perror "Group $GROUP already exists with GID ${_GID}, but creation was requested with GID $GROUPID"
	if [ "x$GROUPID" = "x-" ]; then
		local _UID=$(grep -E "^${GROUP}:[^:]*:[0-9]+:" "${_PASSWD}" | head -1 | awk -F ':' '{print $3}')
		[ -z "${_UID}" ] && _UID=100
		GROUPID=$(generate_gid "$GROUP" "${_UID}")
	fi
	if ! [[ $GROUPID =~ [0-9]+ ]]; then
		perror "add_group: GROUPID not numeric (is '$GROUPID')"
	fi
	echo "${GROUP}:x:${GROUPID}:" >> "${_GROUP}"
	pinfo "Created group $GROUP"
	echo "${GROUPID}"
}

init_users_and_groups() {
	[ -z "$TARGET_BUILD_DIR" -o "$TARGET_BUILD_DIR" == "/" ] && perror "Almost wrecked your local passwd, group and shadow file. phew."
	local USER
	local PASSWD="$TARGET_BUILD_DIR/etc/passwd"
	local GROUP="$TARGET_BUILD_DIR/etc/group"
	local SHADOW="$TARGET_BUILD_DIR/etc/shadow"
	[ -s "${PASSWD}" -a -s "${GROUP}" -a -s "${SHADOW}" ] && return
	pinfo "Creating users and groups based on local system...."
	cp -a "/etc/passwd" "$PASSWD" || perror "Could not copy /etc/passwd"
	cp -a "/etc/group" "$GROUP" || perror "Could not copy /etc/group"
	cp -a "/etc/shadow" "$SHADOW" || perror "Could not copy /etc/shadow"
	# make sure shadow has group shadow (should be handled by cp -a but hey)
	chgrp shadow "$SHADOW"
	# remove local users from group file (TODO: currently assumes users have ids 1000-1999)
	local LOCALUSERS=$(grep -E '^[^:]+:x?:1[0-9]{3}:' "${PASSWD}" | awk -F ':' '{print $1}')
	for USER in $LOCALUSERS; do
		sed -r -i "s/([:,])${USER}/\1/g" "${GROUP}"
	done
	# fix syntax: remove trailing ',' in group file
	sed -r -i 's/,+$//g' "${GROUP}"
	sed -r -i 's/,+/,/g' "${GROUP}"
	sed -i 's/:,/:/g' "${GROUP}"
	# remove all non-system groups (also assumes users have 1000-1999, so nogroup will be kept)
	grep -v -E '^[^:]+:x?:1[0-9]{3}:' "${GROUP}" > "${GROUP}.tmp"
	mv "${GROUP}.tmp" "${GROUP}"
	# same for users...
	grep -v -E '^[^:]+:x?:1[0-9]{3}:' "${PASSWD}" > "${PASSWD}.tmp"
	mv "${PASSWD}.tmp" "${PASSWD}"
	# generate fresh shadow file
	awk -F ':' '{print $1":*:15555:0:99999:7:::"}' "${PASSWD}" > "${SHADOW}"
}

