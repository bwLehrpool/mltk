[ -n "$MLTK_QUIET" ] && return 0

# This module is an exception, it doesn't use __init but runs commands directly...
# Otherwise we cannot ensure that other module's init functions won't run first
# and mess up the fds

set_quiet () {
	if [ "x$MLTK_QUIET" != "x1" ]; then
		exec 6>&1  > "$LOG_DIR/stdout.log"
		exec 7>&2 2> "$LOG_DIR/stderr.log"
		MLTK_QUIET=1
	fi
}

unset_quiet () {
	if [ "x$MLTK_QUIET" == "x1" ]; then
		exec 1>&6 6>&-
		exec 2>&7 7>&-
		exec 6>&2
		MLTK_QUIET=0
	fi
}


pinfo  () {
	echo -e "\033[38;5;10m[info]\033[0m    $TOOL_STR $@" >&6
}

perror () {
	echo -e "\033[38;5;9m[error]\033[0m   $TOOL_STR $@" >&6
	qnd_exit
}

pwarning  () {
	echo -e "\033[38;5;11m[warning]\033[0m $TOOL_STR $@" >&6
}

pdebug  () {
	if [ "x$MLTK_QUIET" = "x1" ]; then
		echo -e "[DEBUG] $TOOL_STR $@" >&2
	else
		echo -e "\033[38;5;6m[debug]\033[0m   $TOOL_STR $@" >&6
	fi
}

exec 6>&2
exec 8>&0	# This redirection is used for chroot debugging

MLTK_QUIET=0
declare -rg LOG_DIR=${ROOT_DIR}/var/log
mkdir -p "$LOG_DIR" || perror "Could not create '$LOG_DIR'"
true

