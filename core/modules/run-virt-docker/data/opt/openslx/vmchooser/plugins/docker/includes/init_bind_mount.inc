#!/bin/bash

## required vars
# TMPDIR=/tmp/virt/<CURRENT_USER>/<PID>/
# USER_CONTAINER_CONFIG
# NETSHARE_DIR

# vars
BIND_MOUNT_FILE="$CONFDIR/container_meta.json"
# TODO: Maybe make this an array to support spaces
BIND_MOUNT_STRING=""


BIND_MOUNT_COUNT="$( cat "$BIND_MOUNT_FILE" | jq '.bind_mount_config | length' )"

for (( index=0; index < "$BIND_MOUNT_COUNT"; index++ )); do

	BIND_MOUNT_INFO="$( jq .bind_mount_config["$index"] "$BIND_MOUNT_FILE" )"

	# expecting source to be a MOUNT_LETTER, wich must replaced with the directory
	BIND_MOUNT_SOURCE="$( jq -r .source <<< "$BIND_MOUNT_INFO" )"
	BIND_MOUNT_TARGET="$( jq -r .target <<< "$BIND_MOUNT_INFO" )"
	BIND_MOUNT_OPTION="$( jq -r .option <<< "$BIND_MOUNT_INFO" )"

	if [[ -z "$BIND_MOUNT_SOURCE" || -z "$BIND_MOUNT_TARGET" ]]; then
		writelog "+ no proper bind mount option provided!"
		continue
	fi

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
		DOCKER_USER_TMP="$DOCKER_TMP/$(id -u)"
		[ ! -e $DOCKER_USER_TMP ] && mkdir -p $DOCKER_USER_TMP && chmod 0700 $DOCKER_USER_TMP
		BIND_MOUNT_STRING+=" --mount type=bind,source=$DOCKER_USER_TMP,target=$BIND_MOUNT_TARGET"

	elif [[ -z "${NETSHARE_DIR[$BIND_MOUNT_SOURCE]}" ]]; then
		writelog "+ no bind mount mapping for letter $BIND_MOUNT_SOURCE found!"
		continue
	else
		BIND_MOUNT_STRING+=" --mount type=bind,\\\"source=${NETSHARE_DIR[$BIND_MOUNT_SOURCE]}\\\",target=$BIND_MOUNT_TARGET"
	fi

done

writelog "+ write final CONTAINER_BIND_MOUNT_STRING $BIND_MOUNT_STRING in config $USER_CONTAINER_CONFIG"
echo "CONTAINER_BIND_MOUNT_STRING=\"${BIND_MOUNT_STRING}\"" >> ${USER_CONTAINER_CONFIG}
