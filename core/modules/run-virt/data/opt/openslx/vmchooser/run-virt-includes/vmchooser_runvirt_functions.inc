#!/bin/bash
#######################################################
# Include: Set functions needed by vmchooser-run_virt #
#######################################################
## Assigns an ID to the currently starting VM to support
# multiple instances running simultaneously.
# Note that VM_ID will always have two digits.
get_vm_id() {
	local script=${BASH_SOURCE[-1]}
	[ -z "$script" ] && script="$0"
	if [ -n "$script" ]; then
		script=$(readlink -f "$script")
		if [ -n "$script" ] && [ -s "$script" ]; then
			#bingo
			declare -g VM_ID=$(ps ax | grep -F "$script" | grep -v 'grep' | grep -o -- "${script}.*\$" | sort -u | wc -l)
			if [ "$VM_ID" -gt 0 ]; then
				[ "${#VM_ID}" -eq 1 ] && VM_ID="0${VM_ID}"
				[ "${#VM_ID}" -gt 2 ] && VM_ID="${VM_ID:0:2}"
				[ "${#VM_ID}" -eq 2 ] && readonly VM_ID && return
			fi
		fi
	fi
	# fallback: take last two digits of current pid...
	VM_ID=$(expr substr $$ $(expr ${#$} - 1) 2)
	[ "${#VM_ID}" -eq 1 ] && VM_ID="0${VM_ID}"
	readonly VM_ID
}

################# LOGGING FUNCTIONS ##################
# Helper function to write to stdout and logfile
writelog() {
	local DATE=$(date +%Y-%m-%d-%H-%M-%S)
	# write to stdout?
	if [ "x$1" = "x--quiet" ]; then
		shift
	else
		echo -e "$DATE: $@"
	fi
	# log into file
	echo -e "$DATE: ${SLX_DEBUG:+(${FUNCNAME[1]}) }$@" >> "${LOGFILE}"
}

# Helper function to notify the user.
# This directly returns and do not wait for a user confirmation.
notify_user() {
	local TOPIC="$1"
	shift
	notify-send -u normal "$TOPIC" "$*"
	writelog "Notify: **${TOPIC}**: $*"
}

# Helper to display an error message box to the user.
# Only returns when the user dismisses the message.
error_user() {
	local TITLE="$(translate "$1")"
	shift
	local BODY="$*"
	local MSG
	if [ -n "$BODY" ]; then
		BODY="$(translate "$BODY")"
		MSG="   $TITLE

$BODY"
	else
		MSG="$TITLE"
		BODY="$TITLE"
		TITLE="ERROR"
	fi
	# Zenity should yield the nicest result
	# TODO the title is only set as the window name,
	# which cannot be seen without a window manager
	zenity --error --title "$TITLE" --text "$BODY"
	local RET=$?
	[ $RET -le 1 ] && return
	# no zenity...
	# QnD abuse printergui for error message as it's blocking
	/opt/openslx/cups/printergui --error "$MSG" && return
	# no printergui...
	# xmessage is ugly but gets the job done
	xmessage "$MSG" && return
	# unfortunately, I can only think of notify+sleep right now
	notify-send -u critical "$TITLE" "$BODY"
	sleep 10
}

################## CLEANUP FUNCTIONS ##################
# Registers functions to be called when cleanexit is called.
# Only accepts functions that were not previously registered.
# This kinda detects when a cleanup function was overriden,
# or at least that something is fishy.
declare -ag CLEANUP_TASKS
add_cleanup() {
	[ $# -lt 1 ] && writelog "'${FUNCNAME[0]}' needs at least one argument! $# given." && return
	# check if the given function name is already used
	while [ $# -ne 0 ]; do
		if array_contains CLEANUP_TASKS "$1"; then
			writelog "Cleanup function '$1' already registered! Are there multiple definitions of this function?"
			writelog "This might suggest that a previously defined cleanup function was overriden!"
			return 1
		fi
		CLEANUP_TASKS+=("$1")
		shift
	done
	return 0
}
# This function will be called at the end of vmchooser-run_virt
# or upon critical errors during the runtime.
# It will call the cleanup functions registered through 'add_cleanup'
# and clean TMPDIR if appropriate. Further, it will evaluate and
# process the EXIT_{TYPE,REASON} variables that hold information
# on why we are about to exit.
#
# EXIT_TYPE should be either:
#	- 'internal' for critical internal errors, this will
#     automatically send the logfile via slxlog.
#	- 'user' for errors related to the user's choice
# Others could be thought of like 'external' for failures
# with remote services (e.g. slx apis, external image repo, ...)
#
# EXIT_REASON should contain a user-friendly message to print to the user.
# it can be prefixed with err.\S+, which will serve as a translation identifier
cleanexit() {
	writelog "Cleanexit '$1' triggered by '${BASH_SOURCE[1]}:${FUNCNAME[1]}'"
	sleep 1
	while isset CLEANUP_TASKS; do
		local TASK=${CLEANUP_TASKS[-1]}
		unset -v CLEANUP_TASKS[-1]
		if ! is_function $TASK; then
			writelog "Registered cleanup function '$TASK' is not a function. This should not be..."
			continue
		fi
		if ! ${TASK}; then
			writelog "Failed to run cleanup function '$TASK'! Exit code: $RET"
		fi
	done

	# If we're not in debug mode, remove all temporary files
	if notempty SLX_DEBUG && isset TMPDIR; then
		rm -rf -- "${TMPDIR}"
	fi

	# Now see if we need to do the catch all error stuff
	# if 0 given, exit 0
	[ "x$1" = "x0" ] && exit 0
	# if no code was given, exit 129
	[ $# -eq 0 ] && writelog "Cleanexit called without arguments! Dev error?"

	# given exit code is set and not 0, handle the error now
	# now evaluate the EXIT_{TYPE,REASON} variables and generate error title/text
	local ERR_TITLE ERR_TEXT ERR_FOOTER
	# default error footer
	ERR_FOOTER="Versuchen Sie den Computer neuzustarten und falls das Problem bestehen bleibt, kontaktieren Sie den Support."
	if notempty EXIT_TYPE; then
		case "${EXIT_TYPE}" in
			user)
				ERR_TITLE="Auswahlfehler"
				ERR_FOOTER="Beim Start Ihrer Veranstaltung sind Fehler aufgetreten. Versuchen Sie es mit einer anderen Veranstaltung."
				;;
			internal)
				ERR_TITLE="Interner Fehler"
				;;
			*)
				ERR_TITLE="Unbekannter Fehler"
				writelog "Unknown EXIT_TYPE: '${EXIT_TYPE}'."
				;;
		esac
	fi
	if notempty EXIT_REASON; then
		# Try i18n
		ERR_TEXT="$(translate "$EXIT_REASON")"
	else
		# this should never happen if EXIT_REASON is properly
		# used when calling cleanexit !
		ERR_TEXT="Unbekannter Fehler"
	fi

	# first send the logfile (in case the user does not close the error before using magic keys e.g.)
	# for any other error types besides 'user'. Do no slxlog if we run wrapped (from /opt/openslx/scripts/vmchooser-run_virt)
	[ "x${EXIT_TYPE}" != "xuser" ] && ! isset WRAPPED && \
		slxlog "runvirt-exit-${EXIT_TYPE}" "Critical error happened in '${BASH_SOURCE[1]}:${FUNCNAME[1]}', see logs." "${LOGFILE}"

	# finally display the error
	error_user "${ERR_TITLE}" "
${ERR_TEXT}

${ERR_FOOTER}
"
	writelog "All done. Exiting."
	# if no exit code was given as $1, exit 129
	exit "${1:-129}"
}

##
# Translate the given string using a lookup file for messages/strings
# This expects one parameter in the form of "msg.foo.bar_baz${IFS}Fallback msg"
# where the first word is really just of the format msg.*
# If no translation is found, the rest of the string will be returned. If the
# argument does not start with msg.*, the whole string will be returned as is
translate() {
	local ERR_TEXT_CODE
	read -r ERR_TEXT_CODE _ <<<$1 # No quotes to strip leading spaces automatically
	if [[ "$ERR_TEXT_CODE" = msg.* ]]; then
		# Message starts with message identifier
		if ! is_array TRANSLATION_STRING; then
			unset TRANSLATION_STRING
			if [ -s "${RUN_VIRT_INCLUDE_DIR}/strings.inc" ]; then
				declare -A TRANSLATION_STRING
				$(safesource "${RUN_VIRT_INCLUDE_DIR}/strings.inc")
			fi
		fi
		if is_array TRANSLATION_STRING && [ -n "${TRANSLATION_STRING[$ERR_TEXT_CODE]}" ]; then
			echo "${TRANSLATION_STRING[$ERR_TEXT_CODE]}"
			return 0
		fi
		# Identifier not found in translation file, fallback
		echo "${1#*$ERR_TEXT_CODE}"
	else
		# No identifier, output everything
		echo "$1"
	fi
}

################# SOURCING FUNCTIONS #################
# Wrapped 'source' that first checks for existence and
# syntax before actually sourcing the given file.
# The option '--exit' triggers cleanexit upon syntax errors.
# Without it, it returns 0 when tests passed, 1 otherwise.
# Usage:
#	safesource [--exit] <files>
safesource() {
	declare -i EXIT_ON_FAILURE=0
	[ "x$1" = "x--exit" ] && EXIT_ON_FAILURE=1 && shift
	while [ $# -gt 0 ]; do
		# sanitze filename just to be sure as it is part of the eval coming later
		# alphanumeric and - _ . should be enough for common file naming scheme
		if [[ ! "$1" =~ ^[a-zA-Z0-9./_-]+$ ]]; then
			writelog "'$1' is a weird filename to source! Ignoring."
			return 1
		fi
		local FILE="$1"
		shift
		bash -n "${FILE}"
		local -i RET=$?
		if [ $RET -ne 0 ]; then
			case $RET in
				1) writelog --quiet "Bad file to source: ${FILE}" ;;
				2) writelog --quiet "Bad syntax: ${FILE}" ;;
				126) writelog --quiet "Could not access: ${FILE}" ;;
				127) writelog --quiet "File not found: ${FILE}" ;;
				*) writelog --quiet "Syntax check (bash -n) returned unknown error code '${RET}' for: ${FILE}" ;;
			esac
			if [ $EXIT_ON_FAILURE -eq 1 ]; then
				echo "eval EXIT_REASON=\"Could not safesource '${FILE}'\" cleanexit 1 ;"
			else
				echo "eval writelog \"Could not safesource '${FILE}'.\" ;"
			fi
			return 1
		fi
		echo "eval source ${FILE} ;"
		echo "run_post_source ${FILE} ;"

	done
	return 0
}

# Registers functions to be called after sourcing an include.
# Includes should only define functions and register them
# to be called after successfully sourcing.
declare -Ag RUN_POST_SOURCE
call_post_source() {
	while [ $# -gt 0 ]; do
		if ! is_function "$1"; then
			writelog "Tried to register a non-function: '$1'"
			continue
		fi
		if notempty BASH_SOURCE[1]; then
			RUN_POST_SOURCE[${BASH_SOURCE[1]}]+="$1 "
			shift
		else
			writelog "Could not determine the sourced file calling ${FUNCNAME[0]}"
		fi
	done
}
# Helper called after sourcing the file via safesource. It just calls the
# functions in the same order they were registered.
run_post_source() {
	[ $# -ne 1 ] && writelog "'${FUNCNAME[0]}' expects one argument only! $# given." && return 1
	for TASK in ${RUN_POST_SOURCE["${1}"]}; do
		# sanity checks
		if ! is_function $TASK; then
			writelog "\tRegistered function '$TASK' is not a function!"
			return 1 # TODO maybe even cleanexit here as this seems very bad...
		fi
		# remove from stack before running it
		RUN_POST_SOURCE["${1}"]="${RUN_POST_SOURCE[${1//${TASK}\ /\ }]}"
		${TASK}
		local -i RET=$?
		if [ $RET -ne 0 ]; then
			writelog "\tFailed to run post source '${TASK}' (Exit code: $RET)"
			return $RET
		fi
	done
	return 0
}

################# FEATURE FUNCTIONS ##################
# Helper to register feature handlers, read run-virt.d/README
declare -Ag FEATURE_HANDLERS
reg_feature_handler() {
	if [ $# -ne 2 ]; then
		writelog "'${FUNCNAME[0]}' expects 2 arguments! $# given."
		return 1
	fi
	if notempty FEATURE_HANDLERS["$1"]; then
		writelog "'${BASH_SOURCE[1]}' tried to overwrite feat handler '$1'! Ignoring."
		# maybe allow overwritting?
		return 1
	fi
	if ! is_function "$2"; then
		writelog "'${BASH_SOURCE[1]}' tried to register a non-function as feat handler!"
		writelog "\t'$2' is a '$(type -t $2 2>&1)'."
		return 1
	fi
	# all good, save it
	FEATURE_HANDLERS["$1"]="$2"
	return 0
}


################### XML FUNCTIONS ####################
# Extract given xpath from given xml file
# e.g.: xmlextract '//node/nestednode/@attribute' "$file"
# @param
# @return Plain text, UTF-8
xmlextract() {
	xmlstarlet sel -T -E utf-8 -t -v "$1" "$2"
}

# Wrapper for convenience
get_xml () {
	xmlextract "//settings/eintrag/${1}/@param" "${XML_FILE}"
}


################## HELPER FUNCTIONS ##################
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

# Convenience function
isempty() {
	! notempty $@
}

# Helper to test if given arguments are declared as functions
is_function() {
	while [ $# -gt 0 ]; do
		local TYPE="$(type -t "$1" 2>/dev/null)"
		if [ "x${TYPE}" != "xfunction" ]; then
			writelog "'$1' not a function but a '${TYPE}'."
			return 1
		fi
		shift
	done
	return 0
}

# Helper to test if given arguments are declared as arrays
is_array() {
	# -ne 1 ] && writelog "is_array: Expects 1 argument! $# given." && return 1
	while [ $# -gt 0 ]; do
		local ARRAY_DEF="$(declare -p ${1} 2>/dev/null)"
		if [[ ! "${ARRAY_DEF}" =~ "declare -a" ]] && [[ ! "${ARRAY_DEF}" =~ "declare -A" ]]; then
			return 1
		fi
		shift
	done
	return 0
}

# Helper to test is the given array contains given value
# Usage:
#	array_contains ARRAY_TO_TEST <values...>
array_contains() {
	if [ $# -lt 2 ]; then
		#writelog "${FUNCNAME[0]}: Expects at least 2 arguments, $# given."
		return 1
	fi
	# is $1 even defined?
	local ARRAY_DEF="$(declare -p $1 2>/dev/null)"
	if isempty ARRAY_DEF; then
		#writelog "${FUNCNAME[0]}: '$1' not defined!"
		return 1
	fi
	local ARRAY_NAME="$1"
	shift

	# sanity check on $ARRAY_DEF being either indexed or associative array
	if ! is_array "${ARRAY_NAME}"; then
		#writelog "${FUNCNAME[0]}: '${ARRAY_NAME}' not an array! Declared as:\t${ARRAY_DEF}"
		return 1
	fi

	# now test if array contains the given values in $2+
	while [ $# -gt 0 ]; do
		# check if ARRAY_DEF contains '"<value>"'
		if [[ ! "${ARRAY_DEF}" =~ '="'${1}'"'[^\]]+ ]]; then 
			#writelog "${FUNCNAME[0]}: '${1}' not in '${ARRAY_NAME}'"
			return 1
		fi
		shift
	done
	return 0
}

# Helper to check if the given arguments are valid command names.
# This uses 'type -t' thus supports notably binaries, functions
# and aliases (might need these one day).
# By default, only 0 is returned if all arguments are found.
# Use '--oneof' to return 0 if any one of the arguments are found.
check_dep() {
	[ $# -lt 1 ] && return 1
	unset ONEOF
	if [ "x$1" = "x--oneof" ]; then
		local ONEOF="1"
		shift
	fi
	while [ $# -gt 0 ]; do
		if ! type -t "$1" >/dev/null 2>&1 ; then
			writelog "Dependency check failed! Could not find '$1'."
			isset ONEOF || return 1
		else
			isset ONEOF && return 0
		fi
		shift
	done
	isset ONEOF && return 1 || return 0
}

# TODO: This is only used once in the whole script:
# to cleanup the os string stored in the xml
# Since the rework of this script, the os strings come from
# the satellite server which already gives us a sanitized string
# thus this function might not be needed anymore, as calling it on
# new gen os strings effectively does nothing.
# Removes any non-alphanumerical and non-hyphen chars
# from the given parameters.
clean_string() {
	if [ "$#" -ge 1 ]; then
		echo "$@" | tr '[A-Z]' '[a-z]' | tr -d -c '[a-z0-9\-]'
	else
		tr '[A-Z]' '[a-z]' | tr -d -c '[a-z0-9\-]'
	fi
}

# Helper to check whether given directory resides in RAM, either
# by being mounted as tmpfs or not mounted at all in which case
# we assume the same. Returns 0 if so, 1 otherwise.
dir_on_tmpfs() {
	local current_dir="$1"
	while [ -n "$current_dir" ]; do
		local mount_line="$(awk -v dir="$current_dir" '$2 == dir' /proc/mounts)"
		if [ -z "$mount_line" ]; then
			# check its parent directory
			current_dir="${current_dir%/*}"
			continue
		fi
		[ "x$(cut -d' ' -f3 <<< ${mount_line})" == "xtmpfs" ]
		return $?
	done
	# NG: check if / is fully backed by hard drive
	local dmsetup_conf="/run/openslx/dmsetup.conf"
	if [ -s "$dmsetup_conf" ]; then
		return $( ! grep -qE '^/dev/mapper/root\s+/\s+type=1' "$dmsetup_conf" )
	fi
	# else assume old-gen were / was in RAM
	return 0
}

# Helper to detect given cpu flags.
# If more than one flag is given, assume that matching
# any of them is sufficient.
# Returns 0 if detected, 1 otherwise.
detect_cpu_flag() {
	if [ "$#" -eq 0 ]; then
		writelog "${FUNCNAME[0]} requires at least one argument, 0 given."
		return 1
	fi
	local flags=$1
	while [ "$#" -ne 0 ]; do
		flags="$flags|$1"
		shift
	done
	grep -m1 -qE '^flags\s*:.*\b('"${flags}"')\b' /proc/cpuinfo
}

# downloads the given URL to given file
download_file() {
  [ $# -ne 2 ] && writelog "Usage: $0 <url> <path>." && return 1
  local _url="$1"
  local _path="$2"
  echo "Downloading '$_url' to '$_path'..."
  if ! wget -T 6 -O "$_path" "$_url" 2> /dev/null >&2; then
    writelog "Downloading '$_url' failed."
    return 1
  fi
  if [ ! -s "$_path" ]; then
    # zero bytes, log and ignore
    writelog "Downloaded resource from '$_url' has zero bytes."
    return 1
  fi
  return 0
}

