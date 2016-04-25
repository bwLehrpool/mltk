#
# This include tries to determine system paths needed by mltk-modules
# The idea here is to have a central place to determine paths required
# by different modules instead of each module determining them on its own.
#

__init () {
	# Location of PAM-modules
	for CANDIDATE in $(strings "$(ldd "$(which login)" | grep libpam.so | head -n 1 | awk '{print $3}')" | grep /lib); do
		[ -f "$CANDIDATE/pam_unix.so" ] && declare -rg SYS_PAM_MODULES_PATH="$CANDIDATE" && break
	done

	[ -z "$SYS_PAM_MODULES_PATH" ] && perror "Failed to find pam_unix.so on this system."

	#
	# Figure out if we have split usr
	if [ -L "/bin" -a -L "/lib" ]; then
		declare -rg USR_SPLIT="no"
	else
		declare -rg USR_SPLIT="yes"
	fi
	pdebug "/bin and /lib are split from /usr/bin and /usr/lib (they are not symlinks): $USR_SPLIT"

	SYS_LIB_PATHS=""
	for DIR in /lib /lib32 /lib64 /usr/lib /usr/lib32 /usr/lib64; do
		[ -d "$DIR" -a ! -L "$DIR" ] && SYS_LIB_PATHS+=" $DIR"
	done
	declare -rg SYS_LIB_PATHS=$SYS_LIB_PATHS
	pdebug "System lib paths: $SYS_LIB_PATHS"
}

