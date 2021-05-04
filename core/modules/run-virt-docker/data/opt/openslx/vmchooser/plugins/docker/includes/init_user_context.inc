#!/bin/bash


function process_container_meta()
{
	writelog "+ process container_meta.json"
	# Try to make a valid CONTAINER_IMAGE_NAME
	export CONTAINER_IMAGE_NAME=$( jq -r '.image_name'  "$CONFDIR/container_meta.json" | \
		tr  '[:upper:]' '[:lower:]' | sed -e  's/^[[:space:]]*//g' -e 's/[[:space:]]*$//g' | sed -e 's/[[:space:]]/_/g')
	export CONTAINER_RUN_OPTIONS=$( jq -r '.run_options'  "$CONFDIR/container_meta.json" )

	
	# set build context
	build_context_method=$( jq -r '.build_context_method'  "$CONFDIR/container_meta.json" )
	if [[ "$build_context_method" == "0" ]]; then
		writelog "+ container_build_context: dockerfile"
		export CONTAINER_BUILD_CONTEXT="$CONFDIR/"
	elif [[ "$build_context_method" == "1" ]]; then
		writelog "+ container_build_context: git url"
		export CONTAINER_BUILD_CONTEXT=$( jq -r '.build_context_url'  "$CONFDIR/container_meta.json" )
	else
		writelog "+ no proper build_context_method!"
		cleanexit 1
	fi
}


function setup_user_container_context()
{
	mkdir -p "$USER_CONTAINER_CONTEXT"
	# init user directory with scripts
	cp "$DOCKER_PLUGIN_DIR/docker-init" "$USER_CONTAINER_CONTEXT/"
	cp "$DOCKER_PLUGIN_DIR/remount" "$USER_CONTAINER_CONTEXT/"
	cp "$DOCKER_PLUGIN_DIR/.bwlp-user-conf" "$USER_CONTAINER_CONFIG"
	chmod u+x "$USER_CONTAINER_CONTEXT/docker-init"
	chmod u+x "$USER_CONTAINER_CONTEXT/remount"
}


function setup_user_container_autostart()
{
	# TODO: maybe there is a better way to load and start the container which is used in the current lecture
	mkdir -p "$HOME/.config/autostart/"
	cp -f "$DOCKER_PLUGIN_DIR/docker-init.desktop" "$HOME/.config/autostart/"
	sed -i "s:Exec=:Exec=$USER_CONTAINER_CONTEXT/docker-init:" "$HOME/.config/autostart/docker-init.desktop"
}


function init_user_container_config()
{
	if [[ ! -f "$USER_CONTAINER_CONFIG" ]]; then
		writelog "+ USER_CONTAINER_CONFIG: $USER_CONTAINER_CONFIG does not exist!"
		cleanexit 1
	fi

	process_container_meta

	writelog "+ init USER_CONTAINER_CONFIG: $USER_CONTAINER_CONFIG"
	sed -i "s#export DOCKER_PLUGIN_DIR=".*"#export DOCKER_PLUGIN_DIR=\"$DOCKER_PLUGIN_DIR\"#" "$USER_CONTAINER_CONFIG"
	sed -i "s#export DOCKER_INCLUDE_DIR=".*"#export DOCKER_INCLUDE_DIR=\"$DOCKER_INCLUDE_DIR\"#" "$USER_CONTAINER_CONFIG"
	sed -i "s#export TMPDIR=".*"#export TMPDIR=\"$TMPDIR\"#" "$USER_CONTAINER_CONFIG"

	sed -i "s#export VM_DISKFILE_RO=".*"#export VM_DISKFILE_RO=\"$VM_DISKFILE_RO\"#" "$USER_CONTAINER_CONFIG"

	sed -i "s#export CONTAINER_BUILD_CONTEXT=".*"#export CONTAINER_BUILD_CONTEXT=\"$CONTAINER_BUILD_CONTEXT\"#" "$USER_CONTAINER_CONFIG"
	sed -i "s#export CONTAINER_IMAGE_NAME=".*"#export CONTAINER_IMAGE_NAME=\"$CONTAINER_IMAGE_NAME\"#" "$USER_CONTAINER_CONFIG"
	sed -i "s#export CONTAINER_RUN_OPTIONS=".*"#export CONTAINER_RUN_OPTIONS=\"$CONTAINER_RUN_OPTIONS\"#" "$USER_CONTAINER_CONFIG"

	local RUNSCRIPT="$CONFDIR/runscript"
	# check if runscript file contains more than default line "ext=;visibility=1;soundMuted=-1"
	if [[ "$( < "$RUNSCRIPT"  wc -l )" -gt "1" ]]; then
		sed -i "s#export RUNSCRIPT=".*"#export RUNSCRIPT=\"$RUNSCRIPT\"#" "$USER_CONTAINER_CONFIG"
	fi

	sed -i "s#USER_CONTAINER_CONFIG=".*"#USER_CONTAINER_CONFIG=\"$USER_CONTAINER_CONFIG\"#" "$USER_CONTAINER_CONTEXT/docker-init"
}


function cleanup_user_container_context() 
{
	[ -d "$USER_CONTAINER_CONTEXT" ] && rm -rf -- "$USER_CONTAINER_CONTEXT"
	[ -f "$HOME/.config/autostart/docker-init.desktop" ] && rm "$HOME/.config/autostart/docker-init.desktop"

	# force remove all containers	
	[ -n "$(docker ps -aq)" ] && docker rm -f $(docker ps -aq) > /dev/null
}


setup_user_container_context
setup_user_container_autostart
init_user_container_config
add_cleanup cleanup_user_container_context
