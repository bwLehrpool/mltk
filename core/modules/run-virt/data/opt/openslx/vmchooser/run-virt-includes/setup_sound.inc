#!/bin/bash
# ^-- Add shebang even though it's sourced so vim highlights bash specific features properly
########################
# Include: Setup sound #
########################

writelog "Starting sound setup ..."

unset VOL
if [ -r "/run/hwinfo" ] && source "/run/hwinfo"; then
	# On startup, the volume of Master, PCM, Speaker, etc. will be set to 100%
	# Some hardware with builtin speakers might be a bit too loud then, so you can
	# define an exception table here. Array key is "Manufacturer//Model"
	declare -A VOLUME_EXCEPTIONS
	VOLUME_EXCEPTIONS["Hewlett-Packard//HP Compaq 8200 Elite CMT PC"]="85%" # This is bwPC3
	# Read
	VOL=${VOLUME_EXCEPTIONS["${HW_MANUF}//${HW_MODEL}"]}
fi

# Default to maximum volume
if [ -z "$VOL" ]; then
	VOL="100%"
fi

if true; then
	# detecting which card is to be used
	writelog --quiet "Detecting which sound card to use ..."
	PROC="/proc/asound/cards"
	if [ ! -r "$PROC" ]; then
		writelog --quiet "'${PROC}' not found or not readable."
		SOUND_CARD_INDEX=0
		SOUND_CARD_COUNT=1
	else
		# Try to filter HDMI cards first
		SOUND_CARD_INDEX=$(grep -v -i 'HDMI' "${PROC}" | grep -E -o '^[[:space:]]{0,2}[0-9]+[[:space:]]+' | head -n 1)
		# If empty, try again with all
		[ -z "${SOUND_CARD_INDEX}" ] && SOUND_CARD_INDEX=$(cat "${PROC}" | grep -E -o '^[[:space:]]{0,2}[0-9]+[[:space:]]+' | head -n 1)
		if [ -z "${SOUND_CARD_INDEX}" ]; then
			writelog --quiet "No sound card found."
			SOUND_CARD_INDEX=0
		fi
		SOUND_CARD_COUNT=$(grep -E '^[[:space:]]{0,2}[0-9]+[[:space:]]+' "${PROC}" | wc -l)
	fi
	
	SOUND_CARD_INDEX="$(grep -E -o '[0-9]+' <<<$SOUND_CARD_INDEX)"
	writelog --quiet "Detected sound card index is: $SOUND_CARD_INDEX"
	writelog --quiet "Sound card count: $SOUND_CARD_COUNT"
	
	# Adjust sound volume (playback)... Random mixer names we have encountered during testing
	writelog --quiet "Setting up volume..."
	(
		amixer -q -c "$SOUND_CARD_INDEX" sset 'Master' "$VOL" unmute
		amixer -q -c "$SOUND_CARD_INDEX" sset 'PCM' "100%" unmute
		amixer -q -c "$SOUND_CARD_INDEX" sset 'CD' "100%" unmute
		amixer -q -c "$SOUND_CARD_INDEX" sset 'Headphone' "100%" unmute
		amixer -q -c "$SOUND_CARD_INDEX" sset 'Front' "100%" unmute
		amixer -q -c "$SOUND_CARD_INDEX" sset 'Speaker' "100%" unmute
		# Recording. It seems that (most) devices need the volume set to 0, so you
		# don't hear your own mic input, but should be unmuted. Also on some cards,
		# you need to set the cap option on the mic you want to use, while other cards
		# will just ignore that option.
		# Plus, most cards have a Capture mixer, which needs to be set to cap too, and
		# have its volume turned up. (There'll probably be some cards that need yet
		# another setup, but this works for now on 4 tested cards)
		amixer -q -c "$SOUND_CARD_INDEX" sset 'Rear Mic Boost' "50%" cap unmute
		amixer -q -c "$SOUND_CARD_INDEX" sset 'Rear Mic' "0%" cap unmute
		amixer -q -c "$SOUND_CARD_INDEX" sset 'Front Mic Boost' "50%" cap unmute
		amixer -q -c "$SOUND_CARD_INDEX" sset 'Front Mic' "0%" cap unmute
		amixer -q -c "$SOUND_CARD_INDEX" sset 'Mic Boost' "50%" cap unmute
		amixer -q -c "$SOUND_CARD_INDEX" sset 'Mic' "0%" cap unmute
		amixer -q -c "$SOUND_CARD_INDEX" sset 'Capture' "100%" cap unmute
		amixer -q -c "$SOUND_CARD_INDEX" sset 'Input Source' 'Mic'
		amixer -q -c "$SOUND_CARD_INDEX" sset 'Input Source' 'Front Mic' # Let's hope nobody uses rear mic...
		# fix random static noise when starting vmplayer when module snd_pcsp (not pcspkr) is loaded
		amixer -q -c pcsp sset Master "0%" mute 2>/dev/null >&2
	) 2>&1 | grep -v 'amixer: Unable to find'
	writelog --quiet "Done setting up volume."
fi >> "${LOGFILE}" 2>&1 # Don't pipe here since it would spawn a subshell so all variable modifications would be lost

