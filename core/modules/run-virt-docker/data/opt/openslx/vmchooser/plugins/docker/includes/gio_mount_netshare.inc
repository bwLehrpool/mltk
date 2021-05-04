#!/bin/bash

# This script will be exectutet in docker lectures
# and provide the functonallity to mount netshares in the bwlp maxilinux system for the logged in user.
#

# VARS
#
NETSHAREFILE="$CONFDIR/netshares"
CONFIGFILE="$TMPDIR/configfile"

MOUNTS=()
DIRECTORY_LINKS=()

GVFS_MOUNTDIR="/run/user/$( id -u "$USER" )/gvfs"

declare -a ROHSHARES

function cleanup_gio_mount()
{
	for i in "${MOUNTS[@]}"; do
		gio mount -u "$i"
	done

	for i in "${DIRECTORY_LINKS[@]}"; do
		unlink "$i"
	done
}

function do_mount()
{
gio mount "$MOUNT_PREFIX$SHAREPATH" <<HEREDOC
$MOUNT_USER

$MOUNT_PASS
HEREDOC
	DO_MOUNT_RETVAL=$?
}


function provide_directory_links()
{
	# split sharepath into server address and share
	# expected: //server.name.de/share
	IFS='/' read -ra SHAREINFO <<< "$SHAREPATH" # str is read into an array as tokens separated by IFS

	# index 0 and 1 are zero 2 is SHARESERVER 3 is SHARENAME
	if [[ -z "${SHAREINFO[2]}" && -z "${SHAREINFO[3]}" ]]; then
		writelog "+ shareinfo not as expected, can not create dir links"
		return 1
	fi

	local SHARESERVER="${SHAREINFO[2]}"
	local SHARE="${SHAREINFO[3]}"

	local SHARE_MOUNT_POINT="$GVFS_MOUNTDIR/smb-share:server=${SHARESERVER},share=${SHARE}"
	
	if [[ ! -e "$SHARE_MOUNT_POINT" ]]; then
		writelog "+ mount directory for $SHAREPATH not found, can not create links!"
		return 1
	fi

	NETSHARE_DIR["$SHARELETTER"]="$SHARE_MOUNT_POINT"

	# create link do desktop and user home
	ln -sf "${SHARE_MOUNT_POINT}" "${HOME}/${SHARENAME}"
	mkdir -p "${HOME}/Desktop"
	ln -sf "${SHARE_MOUNT_POINT}" "${HOME}/Desktop/${SHARENAME}"

	# register cleanup function
	DIRECTORY_LINKS+=("${HOME}/${SHARENAME}")
	DIRECTORY_LINKS+=("${HOME}/Desktop/${SHARENAME}")
}


function mount_shares()
{
	for (( CONFIGROW = 1; CONFIGROW < ${#ROHSHARES[@]}; CONFIGROW++ )); do
		SHAREPATH=$(echo "${ROHSHARES[CONFIGROW]}" | cut -f 1 -d$'\t' | \
					sed 's:\\:/:g')
		SHARELETTER=$(echo "${ROHSHARES[CONFIGROW]}" | cut -f 2 -d$'\t' | \
					sed 's/://g')												# 	Laufwerksbuchstabe ohne :
		SHARENAME=$(echo "${ROHSHARES[CONFIGROW]}" | cut -f 3 -d$'\t' | sed 's/ /_/g')	# Leerzeichen raus.
		SHAREUSER=$(echo "${ROHSHARES[CONFIGROW]}" | cut -f 4 -d$'\t')		# Username, bei Userhome nicht vorhanden
		SHAREPASS=$(echo "${ROHSHARES[CONFIGROW]}" | cut -f 5 -d$'\t')		# User-PW, bei Userhome nicht vorhanden


		MOUNT_PREFIX="smb:"
		
		if [[ -z "$SHAREUSER" || -z "$SHAREPASS" ]]; then
			MOUNT_USER="$USER"
			MOUNT_PASS="$PW"
		else
			MOUNT_USER="$SHAREUSER"
			MOUNT_PASS="$SHAREPASS"
		fi

		writelog "+ mount netshare $MOUNT_PREFIX$SHAREPATH"
		do_mount > /dev/null

		if [[ "$DO_MOUNT_RETVAL" -eq 0 ]]; then
			writelog "+ ... mount was successfull"
			sleep 1
			provide_directory_links
			MOUNTS+=("$MOUNT_PREFIX$SHAREPATH")
		else
			writelog "+ ... mount faild"
		fi
	done

	unset MOUNT_USER MOUNT_PASS
}


function gio_mount()
{
	# CLEANUP
	rm -f -- "$CONFIGFILE"
	touch "$CONFIGFILE"
	# TODO existing gio mounts shouldn´t exist at this points
	# remove them anyway
	for location in "$GVFS_MOUNTDIR"/*; do
		[ -d "$location" ] && gio mount -u "$location"
	done
	sleep 1

	# Fill CONFIGFILE with pwdaemon info, how it is done in /opt/openslx/vmchooser/run-virt.d/setup_virtual_floppy.inc
	# TODO some checks if everthing run fine.
	pwdaemon --query "$HOME/.pwsocket" > "$CONFIGFILE"
	sed -i 's/^/192.168.101.1\t/' "$CONFIGFILE"

	# Attach netshares to CONFIGFILE
	cat "$NETSHAREFILE" >> "$CONFIGFILE"

	# With this preparetion of CONFIGFILE functions from /opt/openslx/vmchooser/data/linux/includes/
	# can be uesed. NATADDR, PORT, KEYTEMP, RAWKEYTEMP and BYTES  required in get_creds
	NATADDR=$( head -n 1 "$CONFIGFILE" | cut -f 1 -d$'\t' )
	PORT=$( head -n 1 "$CONFIGFILE" | cut -f 2 -d$'\t' )
	KEYTEMP="$( mktemp -t XXXXXXXXXX.dat )"
	RAWKEYTEMP="$( mktemp -t XXXXXXXXXX.dat )"
	BYTES=256 

	source /opt/openslx/vmchooser/data/linux/includes/10_functions.inc
	source /opt/openslx/vmchooser/data/linux/includes/20_get_creds.inc
	source /opt/openslx/vmchooser/data/linux/includes/30_get_shares.inc

	# getting user credentials
	get_creds

	# load shares from CONFIGFILE (../metadata/netshares) into ROHSHARES variable
	get_shares

	# check if required VARS for mounting are non zero
	if [[ -n "$ROHSHARES" && -n "$PW" && -n "$USER" ]]; then
		# mount each mountpoint
		writelog "+ initialize complete ... mount shares"
		mount_shares
	else
		writelog "+ initialize failed"
	fi
}

# check if size of NETSHAREFILE > 0
if [[ -s "$NETSHAREFILE" ]]; then
	writelog "+ NETSHAREFILE: ${NETSHAREFILE} contains informations for network drives... initialize gio mount"
	gio_mount
	add_cleanup cleanup_gio_mount
else
	writelog "+ NETSHAREFILE: ${NETSHAREFILE} empty ... nothing to mount"
fi

unset PW
rm -f -- "$KEYTEMP" "$RAWKEYTEMP" "$CONFIGFILE"
