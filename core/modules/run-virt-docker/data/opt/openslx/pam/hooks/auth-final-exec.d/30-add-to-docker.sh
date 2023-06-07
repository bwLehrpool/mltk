#!/bin/ash

adduser "${PAM_USER}" "docker"

# create a location for user bind mount
# used in /opt/openslx/vmchooser/plugins/docker/includes/init-bind-mount.inc
DOCKER_TMP="/tmp/docker"
[ -e $DOCKER_TMP ] && rm -rf -- $DOCKER_TMP
[ ! -e $DOCKER_TMP ] && mkdir -p $DOCKER_TMP && chmod 0777 $DOCKER_TMP

# remove all leftovers in docker_home from previous users
DOCKER_HOME="/tmp/virt/docker"
rm -r -- "$DOCKER_HOME"/*

# This changes the subuid and subgid for the dockremap(user) to the current user and restarts the docker daemon.
# Because off this change in the docker daemon, for each userns will be a directory under /tmp/virt/docker/
# so new users cannot uses previously downloade images by other user.
# But it saves the next user from using images, created by the previous user.

sed -i "s/dockremap:[0-9]\+.65536/dockremap:$(id -u ${PAM_USER}):65536/g" /etc/subuid
sed -i "s/dockremap:[0-9]\+.65536/dockremap:$(id -g ${PAM_USER}):65536/g" /etc/subgid
systemctl --no-block try-restart docker.service

exit 0
