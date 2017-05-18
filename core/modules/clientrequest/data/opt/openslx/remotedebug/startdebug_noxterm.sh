#!/bin/bash

declare -rg ROOT_DIR="/opt/openslx/remotedebug"
declare -rg UUID=$(cat /run/system-uuid)
declare -rg RCTEMPLATE="${ROOT_DIR}/.templaterc"
declare -rg PIDFILE="${ROOT_DIR}/vncpid"
declare -rg RCFILE="${ROOT_DIR}/.x11vncrc"
declare -rg SAT_URI="/slx-admin/api.php/?do=debugrequest"

function cleanup {
	trap - INT TERM EXIT
	echo -e "Cleaning up..."
	killall x11vnc
	rm -rf ${PASSWD_DIR}
	rm -rf ${RCFILE}
	exit 
}

function errc {
	"$@"
	local STATUS=$?
	if [ ${STATUS} -ne 0 ]; then
		echo -e "ERROR: ${FUNCNAME[1]} threw exit code ${STATUS}. Exiting."
		exit ${STATUS}
	fi
}

function set_opts {
	cp ${RCTEMPLATE} ${RCFILE}
	echo -e "rmflag ${PIDFILE}" >> ${RCFILE}
	echo -e "o ${VNC_LOG}" >> ${RCFILE}
	#echo -e "flag ${PORTFILE}" >> ${RCFILE}

	local CLIP=$(xrandr --current | grep primary | grep -o '[0-9]\+[x][0-9]\+[+][0-9]\+[+][0-9]\+')
	echo -e "clip ${CLIP}" >> ${RCFILE}
}

function send_request {
	#local RESPONSE=$(curl -k -X POST -w %{http_code} -d ${UUID}:0 https://${SAT}${SAT_URI})
	local RESPONSE=$(curl -k -X POST -d ${UUID}:0 https://${SAT}${SAT_URI})
	if [ "${RESPONSE}" = 403 ]; then
		echo -e "Your debugging request has been rejected by the local satellite server."
		echo -e "Either remote debugging is disabled or your client is not known to the satellite server."
		exit 1
	elif [ "${RESPONSE%=*}" != "PORT" ]; then
		echo -e "An error has occured when sending the debug request to the local satellite server."
		echo -e "Response: ${RESPONSE}"
		exit 1
	fi
	echo -e "connect ${SAT}:${RESPONSE#PORT=}" >> ${RCFILE}
}

function run_rdbg {
	source /opt/openslx/config
	declare -rg SAT=${SLX_PXE_SERVER_IP}
	set_opts
	errc send_request
	errc x11vnc -rc ${RCFILE}
}

#set -x
trap cleanup INT EXIT TERM
run_rdbg
