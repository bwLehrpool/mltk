#!/bin/bash
# ^-- Add shebang even though it's sourced so vim highlights bash specific features properly
########################
# Include: Setup sound #
########################
## Functions ##
setup_sound() {
	unset VOL
	if $(safesource "/run/hwinfo") ; then
		# On startup, the volume of Master, PCM, Speaker, etc. will be set to 100%
		# Some hardware with builtin speakers might be a bit too loud then, so you can
		# define an exception table here. Array key is "Manufacturer//Model"
		declare -A VOLUME_EXCEPTIONS
		VOLUME_EXCEPTIONS["Hewlett-Packard//HP Compaq 8200 Elite CMT PC"]="85%" # This is bwPC3
		# Read
		local VOL=${VOLUME_EXCEPTIONS["${HW_MANUF}//${HW_MODEL}"]}
	fi

	# Default to maximum volume
	notempty VOL || VOL="100%"

	if ! check_dep pactl || [ "$(amixer | grep -c '^Simple mixer' )" -gt 3 ]; then
		check_dep amixer && set_sound_alsa
	fi
	if check_dep pactl; then
		set_sound_pulse
	fi
	writelog --quiet "Done setting up volume."
}

set_sound_pulse() {
	writelog --quiet "Setting up volume to '$VOL' via pactl..."
	pactl set-sink-mute @DEFAULT_SINK@ 0
	pactl set-sink-volume @DEFAULT_SINK@ "$VOL"
}

set_sound_alsa() {
	# detecting which card is to be used
	writelog --quiet "Detecting which sound card to use ..."
	local PROC="/proc/asound/cards"
	if [ ! -r "$PROC" ]; then
		writelog --quiet "'${PROC}' not found or not readable."
		local SOUND_CARD_INDEX=0
		local SOUND_CARD_COUNT=1
	else
		# Try to filter HDMI cards first
		SOUND_CARD_INDEX=$(grep -v -i 'HDMI' "${PROC}" | grep -E -m1 -o '^[[:space:]]{0,2}[0-9]+[[:space:]]+')
		# If empty, try again with all
		notempty SOUND_CARD_INDEX || SOUND_CARD_INDEX=$(grep -E -m1 -o '^[[:space:]]{0,2}[0-9]+[[:space:]]+' "${PROC}")
		if isempty SOUND_CARD_INDEX; then
			writelog --quiet "\tNo sound card found."
			SOUND_CARD_INDEX=0
		fi
		SOUND_CARD_COUNT="$( grep -E -c '^[[:space:]]{0,2}[0-9]+[[:space:]]+' "${PROC}" )"
	fi
	
	SOUND_CARD_INDEX="$(grep -E -o '[0-9]+' <<<$SOUND_CARD_INDEX)"
	writelog --quiet "\tDetected sound card index is: $SOUND_CARD_INDEX"
	writelog --quiet "\tSound card count: $SOUND_CARD_COUNT"
	
	# Adjust sound volume (playback)... Random mixer names we have encountered during testing
	# TODO instead of trying to set volume blindly, why not use 'amixer scontrols' to find
	# which mixer control exists and set these only?
	
	writelog --quiet "Setting up volume to '$VOL' via amixer..."
	amixer -q -c "$SOUND_CARD_INDEX" -s <<-EOF
		sset 'Master' "$VOL" unmute
		sset 'PCM' "100%" unmute
		sset 'CD' "100%" unmute
		sset 'Headphone' "100%" unmute
		sset 'Front' "100%" unmute
		sset 'Speaker' "100%" unmute
		# Recording. It seems that (most) devices need the volume set to 0, so you
		# don't hear your own mic input, but should be unmuted. Also on some cards,
		# you need to set the cap option on the mic you want to use, while other cards
		# will just ignore that option.
		# Plus, most cards have a Capture mixer, which needs to be set to cap too, and
		# have its volume turned up. (There'll probably be some cards that need yet
		# another setup, but this works for now on 4 tested cards)
		sset 'Master' "$VOL" unmute
		sset 'PCM' "100%" unmute
		sset 'CD' "100%" unmute
		sset 'Headphone' "100%" unmute
		sset 'Front' "100%" unmute
		sset 'Speaker' "100%" unmute
		sset 'Rear Mic Boost' "50%" cap unmute
		sset 'Rear Mic' "0%" cap unmute
		sset 'Front Mic Boost' "50%" cap unmute
		sset 'Front Mic' "0%" cap unmute
		sset 'Mic Boost' "50%" cap unmute
		sset 'Mic' "0%" cap unmute
		sset 'Capture' "100%" cap unmute
		sset 'Input Source' 'Mic'
		sset 'Input Source' 'Front Mic' # Let's hope nobody uses rear mic...
	EOF
	# fix random static noise when starting vmplayer when module snd_pcsp (not pcspkr) is loaded
	lsmod | grep -q snd_pcsp && amixer -q -c pcsp sset Master "0%" mute
}

## MAIN ##
reg_feature_handler "sound" "setup_sound"
:
