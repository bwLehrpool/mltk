# Helper file for managing key-value containing files
# There are some specialized conveinience functions here first
# that mostly just pass a predefined filename to the genric function
# at the end

# Add the given environment variable to /etc/environment
add_env () {
	[ $# -ne 2 ] && perror "Usage: $0 'key' 'value'"
	[ -z "$1" ] && perror "$0: Empty key!"
	add_key_value "/etc/environment" "$1" "$2"
}

#
# Adds the given key-value-pair to a given file
# The file will be relative to the current target build dir,
# even if it starts with a slash.
# Will perror if the key already exists with a different value
add_key_value () {
	[ $# -ne 3 ] && perror "Usage: $0 'file' 'key' 'value'"
	[ -z "$TARGET_BUILD_DIR" ] && perror "No TARGET_BUILD_DIR set. Aborting for safety."
	local FILE="$TARGET_BUILD_DIR/$1"
	local KEY="$2"
	local VALUE="$(echo "$3" | sed "s/'/\\\\'/g")" # \\\\\\\\\\\\\\\\\\\\\\ßß
	if [ -s "$FILE" ]; then
		local CURRENT="$(grep -E "^\s*$KEY=.*$" "$FILE" | awk -F '=' '{$1=""; printf $0}' | itrim)"
		[ -n "$CURRENT" -a "'$VALUE'" != "$CURRENT" ] && perror "Cannot set $KEY to '$VALUE' as it is already set to $CURRENT"
		[ -n "$CURRENT" ] && return 0
	fi
	mkdir -p "$(dirname "$FILE")"
	echo "$KEY='$VALUE'" >> "$FILE"
}

