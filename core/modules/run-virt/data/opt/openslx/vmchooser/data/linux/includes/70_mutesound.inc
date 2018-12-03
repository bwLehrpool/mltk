function mutesound()
{

local PACTL=$(which pactl)
local AMIXER=$(which amixer)
local SYSTEM=alsa
local PACTLERR
local AMIXERERR

if [[ -z "$AMIXER" && -z "$PACTL" ]]; then
	logger "openslx sound muter: Neither pactl (pulseaudio) nor amixer (alsa) found. Exiting."
	return 1
fi


if [ -n "$PACTL" ]; then
	SYSTEM=pulse
	logger "openslx sound muter: pulseaudio seems to be installed..."
	x=2
	# while [ $(su -c "pulseaudio --check" "$LOCALUSER") ]; do
	while [ -z "$(ps aux|grep /pulseaudio|grep -v grep)" ]; do
		logger "openslx sound muter: Waiting for pulseaudio..."
		if [ "$x" -gt 30]; then
			logger "openslx sound muter: Waited $x sec for pulseaudio, giving up."
			[ -n "$AMIXER" ] && SYSTEM=alsa || SYSTEM=""	# unset -v SYSTEM
			break
		fi
		sleep $x; x=$((x + 2))
 	done
fi

	# MUTESOUND: Sound 1=aus, 0=an
	# This is a bit problematic as there is no way to actually know whether
	# a command works within different distributions, esp. alsamixer commands 
	# tend to successfully mute but not unmute. So we (at this stage) just 
	# hammer known commands through - one will work, hopefully.
	case $MUTESOUND in 0)	logger "openslx sound muter: Unmuting sound, using $SYSTEM."
				case $SYSTEM in	pulse)	XDG_RUNTIME_DIR=/run/user/$(id -u $LOCALUSER) su -c "pactl set-sink-mute 0 0" "$LOCALUSER"
							PACTLERR=$?
							XDG_RUNTIME_DIR=/run/user/$(id -u $LOCALUSER) su -c "$AMIXER" -q -D pulse sset Master unmute "$LOCALUSER"
							AMIXERERR=$?
							logger "openslx sound muter: Sound hopefully unmuted, pactl: $PACTLERR, amixer: $AMIXERERR."
							;;
						alsa)	"$AMIXER" set Master unmute
							AMIXERERR=$?
							logger "openslx sound muter: Sound hopefully unmuted, amixer: $AMIXERERR."
							;;
				esac
				;;
			1)	logger "openslx sound muter: Muting sound, using $SYSTEM"
				case $SYSTEM in pulse)	XDG_RUNTIME_DIR=/run/user/$(id -u $LOCALUSER) su -c "pactl set-sink-mute 0 1" "$LOCALUSER"
							PACTLERR=$?
							XDG_RUNTIME_DIR=/run/user/$(id -u $LOCALUSER) su -c "$AMIXER" -q -D pulse sset Master mute "$LOCALUSER"
							AMIXERERR=$?
							logger "openslx sound muter: Sound hopefully unmuted, pactl: $PACTLERR, amixer: $AMIXERERR."
							;;
						alsa)	"$AMIXER" set Master mute
							AMIXERERR=$?
							logger "openslx sound muter: Sound hopefully unmuted, amixer: $AMIXERERR."
							;;
				esac
				;;
			*)	logger "openslx sound muter: Unknown muteSound option $MUTESOUND"
				;;
	esac
	logger "openslx sound muter: end."
}
