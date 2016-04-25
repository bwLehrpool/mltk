########################
# Include: Setup sound #
########################
# 8086:1e20: Sound card in bwPX4

writelog "Starting sound setup ..."

if lspci -n | grep -E -i '8086:1e20( |$)'; then
	VOL="100%" # bwPC 4: Speaker too quiet :-(
else
	VOL="85%"
fi

if true; then
	# detecting which card is to be used
	writelog "Detecting which sound card to use ..."
	PROC="/proc/asound/cards"
	if [ ! -r "$PROC" ]; then
		writelog "'${PROC}' not found or not readable."
		SOUND_CARD_INDEX=0
		SOUND_CARD_COUNT=1
	else
		# Try to filter HDMI cards first
		SOUND_CARD_INDEX=$(grep -v -i 'HDMI' "${PROC}" | grep -E -o '^[[:space:]]{0,2}[0-9]+[[:space:]]+' | head -n 1)
		# If empty, try again with all
		[ -z "${SOUND_CARD_INDEX}" ] && SOUND_CARD_INDEX=$(cat "${PROC}" | grep -E -o '^[[:space:]]{0,2}[0-9]+[[:space:]]+' | head -n 1)
		if [ -z "${SOUND_CARD_INDEX}" ]; then
			writelog "No sound card found."
			SOUND_CARD_INDEX=0
		fi
		SOUND_CARD_COUNT=$(grep -E '^[[:space:]]{0,2}[0-9]+[[:space:]]+' "${PROC}" | wc -l)
	fi
	
	SOUND_CARD_INDEX="$(grep -E -o '[0-9]+' <<<$SOUND_CARD_INDEX)"
	writelog "Detected sound card index is: $SOUND_CARD_INDEX"
	writelog "Sound card count: $SOUND_CARD_COUNT"
	
	# Adjust sound volume (playback)... Random mixer names we have encountered during testing
	writelog "Setting up volume..."
	amixer -q -c "$SOUND_CARD_INDEX" sset 'Master' "$VOL" unmute
	amixer -q -c "$SOUND_CARD_INDEX" sset 'PCM' "$VOL" unmute
	amixer -q -c "$SOUND_CARD_INDEX" sset 'CD' "$VOL" unmute
	amixer -q -c "$SOUND_CARD_INDEX" sset 'Headphone' "$VOL" unmute
	amixer -q -c "$SOUND_CARD_INDEX" sset 'Front' "$VOL" unmute
	amixer -q -c "$SOUND_CARD_INDEX" sset 'Speaker' "$VOL" unmute
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
	amixer -q -c "$SOUND_CARD_INDEX" sset 'Input Source' 'Front Mic' # Let's hope nobody uses rear mic...
	# fix random static noise when starting vmplayer when module snd_pcsp (not pcspkr) is loaded
	amixer -q -c pcsp sset Master "0%" mute
	writelog "Done setting up volume."
fi >> "${LOGFILE}" 2>&1
