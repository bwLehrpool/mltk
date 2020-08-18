#!/bin/bash
####################################################
# Include: Start windowmanager for easier handling #
####################################################
## Functions ##
# Some problems may arise with windows opening in background when
# using eg. vmware without a window manager.
start_wm() {
	# Sanity checks
	unset RUNVIRT_DM
	# xfwm4 and metacity make the VM appear frozen after openslx.exe
	# tries to change the resolution
	for DM in "$@" openbox blackbox kwin; do
		if type "${DM}" >/dev/null 2>&1 ; then
			declare -g RUNVIRT_DM="${DM}"
			break
		fi
	done
	if ! isset RUNVIRT_DM; then
		# not fatal, so do not exit but report via slxlog
		slxlog "runvirt-wm" "Could not find any window manager to use!"
		notify_user "Konnte keinen Window Manager finden. (Das ist schlecht!)"
		return 1
	fi

	# start RUNVIRT_DM determined above
	writelog "Starting ${RUNVIRT_DM}."
	declare -a OPTS=()
	case "$RUNVIRT_DM" in
	openbox|kwin|xfwm4|metacity)
		OPTS+=( "--replace" )
		;;
	blackbox)
		# Don't replace background, toolbar not always on top
		OPTS+=( "-rc" "/opt/openslx/vmchooser/config/blackbox.rc" )
		;;
	fvwm2)
		echo "EdgeScroll 0 0" > "${TMPDIR}/fvwm"
		OPTS+=( "-replace" "-f" "${TMPDIR}/fvwm" )
		;;
	esac
	stop_wm
	{
		trap '[ -n "$wpid" ] && kill "$wpid"; exit' EXIT TERM INT
		dc=0
		while true; do
			s="$( date +%s )"
			"${RUNVIRT_DM}" "${OPTS[@]}" &> /dev/null &
			wpid=$!
			wait
			ret=$?
			wpid=
			e="$( date +%s )"
			duration="$(( e - s ))"
			case "$ret" in
			0|129|130|143) exit 0 ;;
			esac
			if [ "$duration" -ge 3 ]; then
				dc=0
			elif [ "$(( dc++ ))" -gt 5 ]; then
				notify_user "$RUNVIRT_DM crasht immer wieder."
				exit 1
			fi
		done
	} &
	declare -g RUNVIRT_DM_PID="$!"
	if isempty HAVE_WM_CLEANUP_HOOK; then
		add_cleanup stop_wm
		declare -rg HAVE_WM_CLEANUP_HOOK=yes
	fi
	return 0
}

stop_wm() {
	isempty RUNVIRT_DM_PID && return 0
	kill "$RUNVIRT_DM_PID"
	usleep 100000
	kill -9 "$RUNVIRT_DM_PID"
	declare -g RUNVIRT_DM_PID=
}

## MAIN ##
call_post_source start_wm
