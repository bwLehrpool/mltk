#######################################################
# Include: Set functions needed by vmchooser-run_virt #
#######################################################

# function to write to stdout and logfile
writelog() {
	local DATE=$(date +%Y-%m-%d-%H-%M-%S)
	# write to stdout?
	if [ "x$1" = "x--quiet" ]; then
		shift
	else
		echo -e "$DATE: $@"
	fi
	# log into file
	echo -e "$DATE: $@" >> "${LOGFILE}"
}

notify_user() {
	local TOPIC="$1"
	shift
	notify-send -u normal "$TOPIC" "$@"
	writelog "Notify: **${TOPIC}**: $*"
}

error_user() {
	local TOPIC="$1"
	shift
	local MSG TITLE BODY
	if [ $# -gt 0 ]; then
		MSG="   $TOPIC
$*"
		TITLE="$TOPIC"
		BODY="$*"
	else
		MSG="$TOPIC"
		TITLE="ERROR"
		BODY="$TOPIC"
	fi
	# Zenity should yield the nicest result
	zenity --error --title "$TITLE" --text "$BODY" && return
	# QnD abuse printergui for error message as it's blocking
	/opt/openslx/cups/printergui --error "$MSG" && return
	# printergui might not exist, try fallback here
	# unfortunately, i can only think of notify+sleep right now
	notify-send -u critical "$TITLE" "$BODY"
	sleep 10
}

# Clean exit will be called at the end of vmchooser-run_virt
cleanexit() {
	sleep 1
	# Ummount dnbd3-fuse
	if [ -n "$dnbd3_fuse_mount_point" ] && [ -e "$dnbd3_fuse_mount_point/img" ]; then
		for timeout in 1 1 1 FAIL; do
			fusermount -u "$dnbd3_fuse_mount_point" && break
			writelog "dnbd3 still busy...."
			[ "$timeout" = "FAIL" ] && break
			sleep "$timeout"
		done
	fi
	# Kill LPD
	[ -n "${PID_LPD}" ] && kill "${PID_LPD}"

	# If we're not in debug mode, remove all temporary files
	if [ -n "${TMPDIR}" -a -z "$SLX_DEBUG" ]; then
		rm -rf -- "${TMPDIR}"
	fi

	[ $# -gt 0 ] && exit "$1"
	exit 129 # No exit code was given :/
}

rv_clean_string() {
	if [ "$#" -ge 1 ]; then
		echo "$@" | tr '[A-Z]' '[a-z]' | tr -d -c '[a-z0-9\-]'
	else
		tr '[A-Z]' '[a-z]' | tr -d -c '[a-z0-9\-]'
	fi
}

# Check if the given variables are set (empty or not)
isset() {
	while [ $# -gt 0 ]; do
		[ -z "${!1+x}" ] && return 1
		shift
	done
	return 0
}

# Check if the given variables are not empty
notempty() {
	while [ $# -gt 0 ]; do
		[ -z "${!1}" ] && return 1
		shift
	done
	return 0
}

##
# Extract given xpath from given xml file
# e.g.: xmlextract '//node/nestednode/@attribute' "$file"
# @param
# @return Plain text, UTF-8
xmlextract() {
	xmlstarlet sel -T -E utf-8 -t -v "$1" "$2"
}

