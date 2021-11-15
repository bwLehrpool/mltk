#!/bin/bash

## required vars
# TMPDIR=/tmp/virt/<CURRENT_USER>/<PID>/
# USER_CONTAINER_CONFIG
# NETSHARE_DIR

# vars
BIND_MOUNT_FILE="$CONFDIR/container_meta.json"
# TODO: Maybe make this an array to support spaces
BIND_MOUNT_STRING=""
VOLUME_MOUNT_INFO=""

function cleanup_container_mount()
{
	for directory in "$VOLUME_MOUNT_INFO"; do
		umount "$directory/dnbd3.fuse"
		find $directory -delete
	done
}

function prepare_container_mount() 
{
	local image_uuid="$BIND_MOUNT_SOURCE"
	local openslx_config="/opt/openslx/config"

	local data_container_dir="${CONFDIR}/${image_uuid}"
	local data_container_metadata="${data_container_dir}/metadata"
	local data_container_mount_info="${data_container_dir}/mount_info"
	mkdir -p "$data_container_dir"

	echo "MOUNT_TARGET=${BIND_MOUNT_TARGET}" > "${data_container_mount_info}"
	echo "MOUNT_OPTION=${BIND_MOUNT_OPTION}" >> "${data_container_mount_info}"

	# get SLX_PXE_SERVER_IP (sat ip)
	source "$openslx_config"
	curl --silent ${SLX_PXE_SERVER_IP}/vmchooser/lecture/${image_uuid}/imagemeta --output "$data_container_metadata"

	retval=$?
	[ "$retval" != "0" ] && writelog "+ request for data container metadata not successful" && return $retval

	if [[ -z "$VOLUME_MOUNT_INFO" ]]; then
		VOLUME_MOUNT_INFO+="${data_container_dir}"
	else
		VOLUME_MOUNT_INFO+=" ${data_container_dir}"
	fi
}


function prepare_default_mount()
{
	# USER_HOME is selected
	if [[ "$BIND_MOUNT_SOURCE" == "USER_HOME" ]]; then
		if [[ -d "$HOME/PERSISTENT" ]]; then
			BIND_MOUNT_STRING+=" --mount type=bind,source=$HOME/PERSISTENT,target=$BIND_MOUNT_TARGET"
		else
			# user has no PERSISTENT, maybe this is a demo user just mount $HOME
			BIND_MOUNT_STRING+=" --mount type=bind,source=$HOME,target=$BIND_MOUNT_TARGET"
		fi

	# If USER_TMP is used, create a locaten for client user and bind mount into container
	elif [[ "$BIND_MOUNT_SOURCE" == "USER_TMP" ]]; then
		# DOCKER_TMP created and cleand in opt/openslx/pam/hooks/auth-final-exec.d/30-add-to-docker.sh
		DOCKER_TMP="/tmp/docker"
		DOCKER_USER_TMP="$DOCKER_TMP/$(id -un)"
		[ ! -e $DOCKER_USER_TMP ] && mkdir -p $DOCKER_USER_TMP && chmod 0700 $DOCKER_USER_TMP
		BIND_MOUNT_STRING+=" --mount type=bind,source=$DOCKER_USER_TMP,target=$BIND_MOUNT_TARGET"

	elif [[ -z "${NETSHARE_DIR[$BIND_MOUNT_SOURCE]}" ]]; then
		writelog "+ no bind mount mapping for letter $BIND_MOUNT_SOURCE found!"
		continue
	else
		BIND_MOUNT_STRING+=" --mount type=bind,\\\"source=${NETSHARE_DIR[$BIND_MOUNT_SOURCE]}\\\",target=$BIND_MOUN  T_TARGET"
	fi
}

BIND_MOUNT_COUNT="$( cat "$BIND_MOUNT_FILE" | jq '.bind_mount_config | length' )"

for (( index=0; index < "$BIND_MOUNT_COUNT"; index++ )); do

	BIND_MOUNT_INFO="$( jq .bind_mount_config["$index"] "$BIND_MOUNT_FILE" )"

	# expecting source to be a MOUNT_LETTER, wich must replaced with the directory
	MOUNT_TYPE="$( jq -r .mount_type <<< "$BIND_MOUNT_INFO" )"
	BIND_MOUNT_SOURCE="$( jq -r .source <<< "$BIND_MOUNT_INFO" )"
	BIND_MOUNT_TARGET="$( jq -r .target <<< "$BIND_MOUNT_INFO" )"
	BIND_MOUNT_OPTION="$( jq -r .options <<< "$BIND_MOUNT_INFO" )"


	if [[ -z "$BIND_MOUNT_SOURCE" || -z "$BIND_MOUNT_TARGET" ]]; then
		writelog "+ no proper bind mount option provided!"
		continue
	fi

	if [[ -z "$MOUNT_TYPE" || "$MOUNT_TYPE" == "DEFAULT" ]]; then
		writelog "+ prepare bind mount option with SOURCE=$BIND_MOUNT_SOURCE (label) and target=$BIND_MOUNT_TARGET"
		prepare_default_mount
		continue
	elif [[ "$MOUNT_TYPE" == "CONTAINER_IMAGE" ]]; then
		writelog "+ prepare docker volume"
		prepare_container_mount
	fi

	if [[ -n "$VOLUME_MOUNT_INFO" ]]; then
		add_cleanup cleanup_gio_mount
	fi 
done

if [[ -n "${BIND_MOUNT_STRING}" ]]; then
	writelog "+ write final CONTAINER_BIND_MOUNT_STRING ${BIND_MOUNT_STRING} in config ${USER_CONTAINER_CONFIG}"
	echo "CONTAINER_BIND_MOUNT_STRING=\"${BIND_MOUNT_STRING}\"" >> ${USER_CONTAINER_CONFIG}
fi

if [[ -n "${VOLUME_MOUNT_INFO}" ]]; then
	writelog "+ write VOLUME_MOUNT_INFO=${VOLUME_MOUNT_INFO} in config"
	echo "VOLUME_MOUNT_INFO=\"${VOLUME_MOUNT_INFO}\"" >> ${USER_CONTAINER_CONFIG}
fi
