#!/bin/bash
####################################################
# Include: Start windowmanager for easier handling #
####################################################
## Functions ##
# Some problems may arise with windows opening in background when
# using eg. vmware without a window manager.
start_wm() {
	# xfwm4 and metacity make the VM appear frozen after openslx.exe
	# tries to change the resolution. for xfwm4 the fix is to disable
	# composition. xfwm4 is better suited for mutli-screen. openbox
	# sometimes puts both virtual screens on the same physical screen,
	# vmware outright acts up if the window manager doesn't expose the
	# _NET_WM_FULLSCREEN_MONITORS atom (which openbox doesn't).
	if [ "$DISPLAY_COUNT" -gt 1 ]; then
		set -- "$@" xfwm4 openbox
	else
		set -- "$@" openbox xfwm4
	fi
	local i dm
	dm=
	for i in "$@" blackbox kwin; do
		if command -v "${i}" &> /dev/null ; then
			dm="$i"
			break
		fi
	done
	if isempty dm; then
		# not fatal, so do not exit but report via slxlog
		slxlog "runvirt-wm" "Could not find any window manager to use!"
		notify_user "Konnte keinen Window Manager finden. (Das ist schlecht!)"
		return 1
	fi

	# If it's the same, do nothing
	if ! is_wm_running "${dm}"; then
		# start DM determined above
		declare -a OPTS=()
		case "${dm}" in
		openbox|kwin|metacity)
			OPTS+=( "--replace" )
			;;
		xfwm4)
			OPTS+=( "--replace" "--compositor=off" )
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
		writelog "Starting ${dm} ${OPTS[*]}."
		declare -g RUNVIRT_DM="${dm}"
		{
			trap '' HUP
			trap '[ -n "$wpid" ] && kill "$wpid"' EXIT
			trap '[ -n "$wpid" ] && kill "$wpid"; exit 5' INT
			trap '[ -n "$wpid" ] && kill "$wpid"; exit 6' TERM
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
				0|129|130|143)
					writelog "WM exited normally"
					exit 0 ;;
				esac
				if [ "$duration" -ge 3 ]; then
					dc=0
				elif [ "$(( dc++ ))" -gt 5 ]; then
					notify_user "$RUNVIRT_DM crasht immer wieder."
					writelog "$RUNVIRT_DM keeps crashing"
					exit 1
				fi
			done
		} &
		declare -g RUNVIRT_DM_PID="$!"
	fi

	# run very simple taskbar in case user minimizes VM somehow and doesn't know Alt+Tab
	if [ -z "$RUNVIRT_TASKBAR_PID" ] || ! kill -0 "$RUNVIRT_TASKBAR_PID"; then
		( sleep 1; exec fspanel ) &
		declare -g RUNVIRT_TASKBAR_PID="$!"
	fi

	if isempty HAVE_WM_CLEANUP_HOOK; then
		add_cleanup stop_wm
		declare -rg HAVE_WM_CLEANUP_HOOK=yes
	fi
	return 0
}

is_wm_running() {
	isempty RUNVIRT_DM_PID && return 1
	! isempty "$1" && [ "$RUNVIRT_DM" != "$1" ] && return 1
	kill -0 "$RUNVIRT_DM_PID"
}

stop_wm() {
	if ! isempty RUNVIRT_TASKBAR_PID; then
		kill "$RUNVIRT_TASKBAR_PID"
		declare -g RUNVIRT_TASKBAR_PID=
	fi
	if ! isempty RUNVIRT_DM_PID; then
		kill "$RUNVIRT_DM_PID"
		usleep 100000
		kill -9 "$RUNVIRT_DM_PID"
		declare -g RUNVIRT_DM_PID=
		declare -g RUNVIRT_DM=
	fi
}

## MAIN ##
call_post_source start_wm
