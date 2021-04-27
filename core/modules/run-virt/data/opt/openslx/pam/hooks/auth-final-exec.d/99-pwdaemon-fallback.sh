#!/bin/ash

[ -z "${SLX_PXE_CLIENT_IP}${SLX_KCL_SERVERS}" ] && . /opt/openslx/config

# Allow querying PW via UNIX Socket?
pw=0
[ "$SLX_PRINT_REUSE_PASSWORD" = "yes" ] && pw=1

USERNAME="${PAM_USER}" PASSWORD="${USER_PASSWORD}" PWSOCKET="${TEMP_HOME_DIR}/.pwsocket" \
	LOCAL_PW="$pw" pwdaemon --daemon "${USER_UID}"
exit 0

