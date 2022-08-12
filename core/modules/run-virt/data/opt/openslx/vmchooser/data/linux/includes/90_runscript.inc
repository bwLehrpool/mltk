function wait_for_wm()
{
	# This just waits until there's a window manager.
	logger "openslx utility: wait_for_wm: Waiting for window manager."
	local expr="no such atom|not found"
	local max=10
	while [[ "$(DISPLAY=:0.0 XAUTHORITY=${XAUTHORITY} xprop -root _NET_SUPPORTING_WM_CHECK)" =~ $expr ]]; do
		sleep 2
		[ $(( --max )) -le 0 ] && break
	done
	logger "openslx utility: wait_for_wm: Window manager detected."
}

function runscript_visibility()
{
	local SCRIPT="$1"
	local USERPART="$2"
	local VISIBILITY="$3"

	case "$VISIBILITY" in
	0)	# 0=hidden
		logger "openslx utility: runscript_visibility: Starting script $SCRIPT hidden(${VISIBILITY}): $USERPART"
		DISPLAY=:0.0 XAUTHORITY="${XAUTHORITY}" $USERPART "$SCRIPT"
		;;
	1)	# 1=normal
		get_xauthfile
		wait_for_wm
		if [ -n "$USERPART" ]; then
			logger "openslx utility: runscript_visibility: Starting script $SCRIPT maximised(${VISIBILITY}) with: $USERPART."
			DISPLAY=:0.0 XAUTHORITY="${XAUTHORITY}" $USERPART "xterm $SCRIPT"
		else
			logger "openslx utility: runscript_visibility: Starting script $SCRIPT maximised(${VISIBILITY}) without any USERPART."
			DISPLAY=:0.0 XAUTHORITY="${XAUTHORITY}" $USERPART xterm "$SCRIPT"
		fi
		;;
	2)	# 2=minimized (xterm -iconic)
		logger "openslx utility: runscript_visibility: starting script $SCRIPT minimised(${VISIBILITY}). $USERPART"
		get_xauthfile
		wait_for_wm

		# Truly not the best way to wait until the desktop environment is ready, but there's no common way to
		# check for DE readiness. Until then, we'll sleep.
		# sleep 20
		if [ -n "$USERPART" ]; then
			logger "openslx utility: runscript_visibility: Starting script $SCRIPT minimised(${VISIBILITY}) with: $USERPART."
			DISPLAY=:0.0 XAUTHORITY="${XAUTHORITY}" $USERPART "xterm -iconic $SCRIPT"
		else
			logger "openslx utility: runscript_visibility: Starting script $SCRIPT minimised(${VISIBILITY}) without any USERPART."
			DISPLAY=:0.0 XAUTHORITY="${XAUTHORITY}" $USERPART xterm -iconic "$SCRIPT"
		fi
		;;
	*)	# *=?
		logger "openslx utility: runscript_visibility: script $SCRIPT unkown scriptVisibility $SCRIPTVISIBILITY."
		;;
	esac
}

function runscript()
{
	logger "openslx: runscript: looking for a script to run..."
	ERR=0
	if [ -f "$SLXCONFIGFILE" ]; then
		[ -z "$SCRIPTEXT" -o "$SCRIPTEXT" = "bat" ] && \
			logger "openslx utility: runscript: Script extension empty or \'bat\', trying anyway."
		if [ -f "$DISKMOUNTDIR/runscript${SCRIPTEXT}" ]; then
			logger "openslx utility: runscript: found script $DISKMOUNTDIR/runscript${SCRIPTEXT}."
			logger "openslx utility: runscript: script options: scriptuser: ${SCRIPTUSER}, scriptvisibility: ${SCRIPTVISIBILITY}."
			set_userpart
			runscript_visibility "$DISKMOUNTDIR/runscript${SCRIPTEXT}" "$USERPART" "$SCRIPTVISIBILITY" &
			SCRIPTPID=$!
			wait $SCRIPTPID
			ERR=$?
			if [ "$ERR" -eq 0 ]; then
				logger "openslx utility: runscript: runscript${SCRIPTEXT} ok, errorlevel ${ERR}."
			else
				logger "openslx utility: runscript: Script error ${ERR}."
			fi
		else
			logger "openslx utility: runscript: script $DISKMOUNTDIR/runscript${SCRIPTEXT} not found."
		fi
	else
		logger "openslx utility: runscript: Could not find $SLXCONFIGFILE."
		return 1
	fi
}

