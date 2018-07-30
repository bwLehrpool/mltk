function wait_for_wm()
{
	# This just waits until there's a window manager.
	while [[ "$(DISPLAY=:0.0 XAUTHORITY=${XAUTHORITY} xprop -root _NET_SUPPORTING_WM_CHECK)" =~ "no such atom" ]]; do
		sleep 2
	done
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

			case "$SCRIPTUSER" in
				0)	USERPART=""
					;;
				1)	USERPART=" su -c - $LOCALUSER "
					;;
				*)	logger "openslx utility: runscript: Unknown script user or script user not set. Running as root."
					USERPART=""
					;;
			esac

			case "$SCRIPTVISIBILITY" in
				0) 	# 0=hidden
					logger "openslx utility: runscript: Starting script hidden."
					echo "$USERPART $DISKMOUNTDIR/runscript${SCRIPTEXT}"
					$USERPART "$DISKMOUNTDIR/runscript${SCRIPTEXT}" &
					;;
				1) 	# 1=normal
					logger "openslx utility: runscript: Starting script maximised."
					get_xauthfile
					wait_for_wm
					DISPLAY=:0.0 XAUTHORITY="${XAUTHORITY}" $USERPART xterm "$DISKMOUNTDIR/runscript${SCRIPTEXT}" &
					;;
				2) 	# 2=minimized (xterm -iconic)
					logger "openslx utility: runscript: starting script minimised."
					get_xauthfile
					wait_for_wm

					# Truly not the best way to wait until the desktop environment is ready, but there's no common way to
					# check for DE readiness. Until then, we'll sleep.
					sleep 20

					DISPLAY=:0.0 XAUTHORITY="${XAUTHORITY}" $USERPART xterm -iconic "$DISKMOUNTDIR/runscript${SCRIPTEXT}" &
					;;
				*)	# *=?
					logger "openslx utility: runscript: unkown scriptVisibility $SCRIPTVISIBILITY."
					;;
			esac

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
		exit 1
	fi
}

