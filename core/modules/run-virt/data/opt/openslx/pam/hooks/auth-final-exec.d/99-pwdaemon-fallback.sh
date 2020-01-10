#!/bin/ash

USERNAME="${PAM_USER}" PASSWORD="${USER_PASSWORD}" PWSOCKET="${TEMP_HOME_DIR}/.pwsocket" pwdaemon --daemon "${USER_UID}"
exit 0

