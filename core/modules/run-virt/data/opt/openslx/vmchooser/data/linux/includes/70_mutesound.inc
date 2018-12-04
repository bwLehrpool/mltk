function mutesound()
{

local PACTL=$(which pactl)
local AMIXER=$(which amixer)
local SYSTEM=alsa
local PACTLERR
local AMIXERERR
local time

if [[ -z "${AMIXER}" && -z "${PACTL}" ]]; then
	logger "openslx sound muter: Neither pactl (pulseaudio) nor amixer (alsa) found. Exiting."
	return 1
fi


if [ -n "${PACTL}" ]; then
	SYSTEM=pulse
	logger "openslx sound muter: pulseaudio seems to be installed..."
	time=2
	# while [ $(su -c "pulseaudio --check" "${LOCALUSER}") ]; do
	while [ -z "$(ps aux|grep /pulseaudio|grep -v grep)" ]; do
		if [ "$x" -gt 30 ]; then
			logger "openslx sound muter: Waited ${time} sec for pulseaudio, giving up."
			break
		fi
		sleep ${time}; time=$((time + 2))
		logger "openslx sound muter: Waited $x seconds for pulseaudio..."
 	done
else
	SYSTEM=alsa
	logger "openslx sound muter: alsa seems to be installed..."
fi

# MUTESOUND: Sound 1=aus, 0=an
# This is a bit problematic as there is no way to actually know whether
# a command works within different distributions, esp. alsamixer commands 
# tend to successfully mute but not unmute. So we (at this stage) just 
# hammer known commands through - one will work, hopefully.
case "${MUTESOUND}" in 0)
		logger "openslx sound muter: Unmuting sound, using ${SYSTEM}."
		case $SYSTEM in	pulse)
			XDG_RUNTIME_DIR=/run/user/$(id -u $LOCALUSER) su -c "${PACTL} set-sink-mute 0 0" "${LOCALUSER}"
			PACTLMUTEERR=$?
			XDG_RUNTIME_DIR=/run/user/$(id -u $LOCALUSER) su -c "${AMIXER} -q -D pulse sset Master unmute" "${LOCALUSER}"
			AMIXERMUTEERR=$?
			XDG_RUNTIME_DIR=/run/user/$(id -u $LOCALUSER) su -c "${PACTL} set-sink-volume 0 ${SOUNDVOL}" "${LOCALUSER}"
			PACTLVOLERR=$?
			XDG_RUNTIME_DIR=/run/user/$(id -u $LOCALUSER) su -c "${AMIXER} -D pulse sset Master ${SOUNDVOL}" "${LOCALUSER}"
			PACTLVOLERR=$?
			logger "openslx sound muter: Sound hopefully unmuted, pactl: ${PACTLMUTEERR}, amixer: ${AMIXERMUTEERR}."
			logger "openslx sound muter: Volume set, pactl: ${PACTLVOLERR}, amixer: $AMIXERVOLERR}."
			;;
		  alsa)	"${AMIXER} set Master unmute"
			AMIXERMUTEERR=$?
			"${AMIXER} set Master ${SOUNDVOL}"
			AMIXERVOLERR=$?
			logger "openslx sound muter: Sound hopefully unmuted, amixer: ${AMIXERMUTEERR}."
			logger "openslx sound muter: Volume set, amixer: ${AMIXERVOLERR}."
			;;
		esac
		;;
	1)
	logger "openslx sound muter: Muting sound, using ${SYSTEM}"
		case "${SYSTEM}" in pulse)
			XDG_RUNTIME_DIR=/run/user/$(id -u ${LOCALUSER}) su -c "${PACTL} set-sink-mute 0 1" "${LOCALUSER}"
			PACTLMUTEERR=$?
			XDG_RUNTIME_DIR=/run/user/$(id -u ${LOCALUSER}) su -c "${AMIXER} -q -D pulse sset Master mute" "${LOCALUSER}"
			AMIXERMUTEERR=$?
			logger "openslx sound muter: Sound hopefully unmuted, pactl: ${PACTLMUTEERR}, amixer: ${AMIXERVOLERR}."
			;;
		   alsa) "${AMIXER}" set Master mute
			AMIXERMUTEERR=$?
			logger "openslx sound muter: Sound hopefully unmuted, amixer: ${AMIXERMUTEERR}."
			;;
		esac		;;
		*)	logger "openslx sound muter: Unknown muteSound option ${MUTESOUND}."
;;
esac
logger "openslx sound muter: end."
}
