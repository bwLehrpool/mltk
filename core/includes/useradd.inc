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

get_gid_for_group() {
	[ $# -ne 1 ] && perror "get_gid_for_group fail. want 1 argument."
	[ -z "${_GROUP}" ] && perror "group file not set."
	local _GID=$(grep -E "^$1:[^:]*:[0-9]+:" "${_GROUP}" | head -1 | awk -F ':' '{print $3}')
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

add_system_user() {
	SYSTEM_ENTITY="yes" add_user $@
}

add_user() {
	[ -z "${TARGET_BUILD_DIR}" ] && perror "add_user: TARGET_BUILD_DIR not set"
	if [ -z "${USER}" -a $# -eq 0 ]; then
		pwarning " ** add_user usage **"
		pwarning "add_user <username>"
		pwarning "OR"
		pwarning "USER=<username> [GROUP=<groupname>] [USERID=<userid>] [GROUPID=<groupid>] [USERHOME=<homedir>] [USERSHELL=<shell>] [PASSWORD=<pass>] add_user"
		perror "Aborting, please fix your script."
	fi

	# In install mode, only work on the system's files directly and do *not* copy to TARGET_BUILD_DIR
	declare -a _USERADD_OPTS
	if [ -z "$MLTK_INSTALL" ]; then
		# regular mltk mode, copy the current user-related files to TARGET_BUILD_DIR
		local _PASSWD="${TARGET_BUILD_DIR}/etc/passwd"
		local _GROUP="${TARGET_BUILD_DIR}/etc/group"
		local _SHADOW="${TARGET_BUILD_DIR}/etc/shadow"
		init_users_and_groups
		[ ! -f "${_PASSWD}" ] && perror "add_user: password file does not exist in target system. (build base first)"
		[ ! -f "${_GROUP}" ] && perror "add_user: group file does not exist in target system. (build base first)"
		[ ! -f "${_SHADOW}" ] && perror "add_user: shadow file does not exist in target system. (build base first)"

		# also add the --root options
		_USERADD_OPTS+=("--root" "$TARGET_BUILD_DIR")
	else
		local _PASSWD="/etc/passwd"
		local _GROUP="/etc/group"
		local _SHADOW="/etc/shadow"
	fi

	if [ "x$1" != "x" ]; then
		local USER="$1"
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

	[ -z "$USERID" ] && local USERID="$(generate_uid "${USER}")"
	[ -z "$USERID" ] && perror "add_user: could not generate a user id for $USER"
	[ -n "$USERID" ] && _USERADD_OPTS+=("--uid" "$USERID")
	if [ -z "$GROUP" ]; then
		[ -z "$GROUPID" ] && local GROUPID=$(get_gid_for_user "${USER}")
		[ -n "$GROUPID" ] && local GROUP=$(get_group_for_gid "${GROUPID}")
		[ -z "$GROUP" ] && local GROUP="$USER"
	fi

	if [ -n "$GROUP" ]; then
		GROUP=$(trim "$GROUP")
		if ! [[ $GROUP =~ $NAME_REGEX ]]; then
			perror "Invalid group: $GROUP"
		fi
		# swallow stdout output since only user id is expected to be echo'ed
		add_group "$GROUP" "$GROUPID" >/dev/null 2>&1
		_USERADD_OPTS+=("--no-user-group" "--gid" "$GROUP")
	fi

	if [ -z "${USERHOME}" ]; then
		local USERHOME=/nonexistent
	else
		_USERADD_OPTS+=("--create-home")
		# make sure the parent directory exists
		if [ -z "$MLTK_INSTALL" ]; then
			_udir="${TARGET_BUILD_DIR}/${USERHOME}"
		else
			_udir="$USERHOME"
		fi
		mkdir -p "${_udir%/*}"

	fi
	_USERADD_OPTS+=("--home-dir" "$USERHOME")

	[ -z "${USERSHELL}" ] && local USERSHELL=/bin/false
	_USERADD_OPTS+=("--shell" "$USERSHELL")

	# create password
	if [ -n "${PASSWORD}" ]; then
		pdebug "Hashing password '$PASSWORD' for '$USER'"
		local PW=$(mkpasswd -m sha-512 "${PASSWORD}")
		[ -z "${PW}" ] && PW=$(openssl passwd -1 "${PASSWORD}")
		[ -z "${PW}" ] && perror "Error generating hashed password for $USER"
		PASSWORD=$PW
	fi
	_USERADD_OPTS+=("--password" "${PASSWORD:-*}")

	[ -n "$SYSTEM_ENTITY" ] && _USERADD_OPTS+=("--system")
	# everything is ready, run useradd
	useradd "${_USERADD_OPTS[@]}" "$USER" >/dev/null 2>&1
	local ret=$?
	[ "$ret" -ne 0 -a "$ret" -ne 9 ] && perror "add_user: useradd failed for: ${_USERADD_OPTS[@]} $USER"
	echo "${USERID}"
}

add_system_group() {
	SYSTEM_ENTITY="yes" add_group $@
}

add_group () {
	[ $# -lt 1 ] && perror "add_group called without argument."
	[ -z "${TARGET_BUILD_DIR}" ] && perror "add_group: TARGET_BUILD_DIR not set"
	declare -a _GROUPADD_OPTS
	if [ -z "$MLTK_INSTALL" ]; then
		# regular mltk mode, copy the current user-related files to TARGET_BUILD_DIR
		local _PASSWD=${TARGET_BUILD_DIR}/etc/passwd
		local _GROUP=${TARGET_BUILD_DIR}/etc/group
		init_users_and_groups
		[ ! -f "${_PASSWD}" ] && perror "add_user: passwd file does not exist in target system. (build base first)"
		[ ! -f "${_GROUP}" ] && perror "add_user: group file does not exist in target system. (build base first)"

		# also add the --root options
		_GROUPADD_OPTS+=("--root" "$TARGET_BUILD_DIR")
	else
		local _PASSWD=/etc/passwd
		local _GROUP=/etc/group
	fi
	local GROUP=$1
	local GROUPID=""
	[[ $GROUP =~ $NAME_REGEX ]] || perror "Invalid group: $GROUP"
	[ $# -ge 2 ] && [ -n "$2" ] && _GROUPADD_OPTS+=("--gid" "$2")
	[ -n "$SYSTEM_ENTITY" ] && _GROUPADD_OPTS+=("--system")
	groupadd "${_GROUPADD_OPTS[@]}" "$GROUP" >/dev/null 2>&1
	local ret=$?
	[ "$ret" -ne 0 -a "$ret" -ne 9 ] && perror "add_group: groupadd failed: ${_GROUPADD_OPTS[@]}"
	[ -z "$GROUPID" ] && local GROUPID="$(get_gid_for_group "$GROUP")"
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
	mkdir -p "${PASSWD%/*}" || perror "Could not mkdir '${PASSWD%/*}'."
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
	# all user-related tools that support "--root" option require nss libs in the
	# chroot target, thus we need to copy the libs over there.
	tarcopy "$(find /lib /lib64 /usr/lib /usr/lib64 -maxdepth 4 -name "libnss_files*")" "$TARGET_BUILD_DIR"
}

