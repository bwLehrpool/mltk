#!/bin/ash

getent group vboxusers || addgroup -S vboxusers
adduser "${PAM_USER}" "vboxusers"

exit 0
