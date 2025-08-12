function mutesound()
{

local wpctlbin=$(which wpctl)
local pactlbin=$(which pactl)
local amixerbin=$(which amixer)
local pipewire=false; 	local pipepulse=false;	local pulseaudio=false
local pactlerr="x";	local wpctlerr="x";	local amixererr="x"
local wpctlopt; 	local pactlopt;		local amixeropt
local wpctlmuteopt;	local wpctlmute="";	local wpctlunmute=""
local pactlmuteopt;	local pactlmute="";	local pactlunmute=""
local amixermuteopt;	local amixermute="mute"; local amixerunmute="unmute"

logger "openslx sound muter: Started."

if [[ -z "${amixerbin}" && -z "${pactlbin}" && -z "${wpctlbin}" ]]; then
	logger "openslx sound muter: Neither pactl, amixer or wpctl found. Exiting."
	return 1
fi

if ps aux | grep -q "pipewire$";		then	pipewire=true; wpctlopt="set-mute @DEFAULT_AUDIO_SINK@"; \
							wpctlmute="1"; wpctlunmute="0"; amixeropt="-c 0 set Master"
							logger "openslx sound muter: pipewire daemon running."; fi
if ps aux | grep -q "pipewire-pulse$";		then	pipepulse=true;  pactlopt="set-sink-mute @DEFAULT_SINK@"; \
							pactlmute="true"; pactlunmute="false"; amixeropt="-c 0 set Master"
							logger "openslx sound muter: pipewire-pulseaudio  daemon running."; fi
if [ $(ps aux | grep -c "pulseaudio") -gt 1 ];	then 	pulseaudio=true; pactlopt="set-sink-mute 0"; pactlmute="0"; pactlunmute="1"; \
							amixeropt="-q -D pulse set Master"
							logger "openslx sound muter: pulseaudio daemon running."; fi

if $pipewire && ! $pipepulse && ! $pulseaudio && [ -z "${wpctlbin}" ]; then
	logger "openslx sound muter: Pipewire detected, but no pipewire-pulse, (native) pulseauadio and wpctl found. Exiting."
	return 1
fi

# MUTESOUND: Sound 1=aus, 0=an
case "${MUTESOUND}" in 
	0)	logger "openslx sound muter: Unmuting sound."
		wpctlmuteopt="$wpctlunmute"; pactlmuteopt="$pactlunmute"; amixermuteopt="$amixerunmute" 
		;;
	1)	logger "openslx sound muter: Muting sound."
		wpctlmuteopt="$wpctlmute"; pactlmuteopt="$pactlmute"; amixermuteopt="$amixermute" 
		;;
	*)	logger "openslx sound muter: Unknown muteSound option ${MUTESOUND}. Exiting."
		return 1 ;;
esac

if [ -n "$wpctlbin" ]; then
	XDG_RUNTIME_DIR=/run/user/$(id -u $LOCALUSER) su -c "$wpctlbin $wpctlopt $wpctlmuteopt" ${LOCALUSER}
	wpctlerr=$?
	logger "openslx sound muter: $wpctlbin $wpctlopt $wpctlmuteopt $LOCALUSER"
fi
if [ -n "$pactlbin" ]; then
	XDG_RUNTIME_DIR=/run/user/$(id -u ${LOCALUSER}) su -c "$pactlbin $pactlopt $pactlmuteopt" ${LOCALUSER}
	pactlerr=$?
	logger "openslx sound muter: $pactlbin ${pactlopt} $pactlmuteopt $LOCALUSER"
fi
if [ -n "$amixerbin" ]; then
	XDG_RUNTIME_DIR=/run/user/$(id -u $LOCALUSER) su -c "$amixerbin ${amixeropt} $amixermuteopt" ${LOCALUSER}
	amixererr=$?
	logger "openslx sound muter: $amixerbin ${amixeropt} $amixermuteopt" $LOCALUSER
fi

logger "openslx sound muter: wpctl $wpctlerr, pactl $pactlerr, amixer $amixererr"
logger "openslx sound muter: end."
}

