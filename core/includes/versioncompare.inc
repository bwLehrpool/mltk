
# From http://stackoverflow.com/a/4025065/2043481
# Returns 0 when equal, 1 when $1 > $2, 2 otherwise
version_compare () {
	if [[ "$1" == "$2" ]]; then
		return 0
	fi
	local IFS=".-_"
	local i ver1=($1) ver2=($2)
	# fill empty fields in ver1 with zeros
	for ((i=${#ver1[@]}; i<${#ver2[@]}; i++)); do
		ver1[i]=0
	done
	for ((i=0; i<${#ver1[@]}; i++)); do
		if [[ -z "${ver2[i]}" ]]; then
			# fill empty fields in ver2 with zeros
			ver2[i]=0
		fi
		if ((10#${ver1[i]} > 10#${ver2[i]})); then
			return 1
		fi
		if ((10#${ver1[i]} < 10#${ver2[i]})); then
			return 2
		fi
	done
	return 0
}

version_ge () {
	version_compare "$1" "$2"
	local RET=$?
	[ "$RET" == "2" ] && return 1
	return 0
}

version_le () {
	version_compare "$1" "$2"
	local RET=$?
	[ "$RET" == "1" ] && return 1
	return 0
}

version_gt () {
	version_compare "$1" "$2"
	local RET=$?
	[ "$RET" == "1" ] && return 0
	return 1
}

version_lt () {
	version_compare "$1" "$2"
	local RET=$?
	[ "$RET" == "2" ] && return 0
	return 1
}

version_eq () {
	version_compare "$1" "$2"
	return $?
}

