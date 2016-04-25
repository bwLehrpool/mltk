#
# copy list of files using tar
tarcopy () {
	if [ $# -gt 0 -a "x$1" == "x-i" ]; then
		shift
		local IGNORE_ERROR="--ignore-failed-read"
	else
		local IGNORE_ERROR=
	fi
	if [ "x$IGNORE_TAR_ERROR" != "x" ]; then
		unset IGNORE_TAR_ERROR
		IGNORE_ERROR="--ignore-failed-read"
	fi
	[ $# -ne 2 ] && perror "Sanity check failed: tarcopy needs exactly two params, but $# were given."
	local FROM=$(trim "$1")
	local TO=$(trim "$2")
	if [ -z "$FROM" ]; then
		pwarning "tarcopy called with empty input list (dest was '$TO')"
		return
	fi
	local SHORT=$FROM
	[ ${#SHORT} -gt 30 ] && SHORT=$(echo "$SHORT" | sed ':a;N;$!ba;s/\n/ /g' | cut -c-25)...$(echo "$SHORT" | cut -c$[${#SHORT} - 4]-)
	[ -z "$TO" ] && perror "tarcopy called with empty destination."
	[ ! -d "$TO" ] && { mkdir -p "$TO" || perror "could not create destination "$TO" for tar-copy."; } 
	# TODO count files copied? would remove the need to do it everywhere :)
	tar $IGNORE_ERROR -cpP $FROM | tar -xp -C "$TO" \
		--transform 's,^/lib/udev/rules.d,/usr/lib/udev/rules.d,' \
		2> /dev/null
	local PS=(${PIPESTATUS[*]})
	[ "x$IGNORE_ERROR" == "x" -a "x${PS[0]}" != "x0" ] && perror "packing-part of tar-copy from '$SHORT' to '$TO' failed. (${PS[0]})"
	[ "x${PS[1]}" != "x0" ] && perror "unpacking-part of tar-copy from '$SHORT' to '$TO' failed. (${PS[1]})"
}

# usage: CANONICALIZED_PATH=$(canonalize <path>)
#        usage with relative path requires you to be in the correct directory.
canonicalize() {
	cd -P -- "$(dirname -- "$1")" && printf '%s\n' "$(pwd -P)/$(basename -- "$1")"
}

