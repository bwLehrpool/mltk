#!/bin/bash
#
# get all files of required packages by a module
#
# Usage:
#	list_content_packages
#		- lists all files/directories in REQUIRED_CONTENT_PACKAGES
#	list_content_packages --files
#		- lists all files in REQUIRED_CONTENT_PACKAGES
#	list_content_packages --dirs
#		- lists all dirs in REQUIRED_CONTENT_PACKAGES
#
#	NOTE:	additional packages needed to be listed can be given
#		through the environment variable EXTRA_PACKAGES

list_content_packages() {
	if [ -z "$REQUIRED_CONTENT_PACKAGES" ]; then
		pinfo "No required packages for $MODULE"
		return 0
	fi
	[ $# -gt 2 ] && perror "'list_content_packages' accepts only 1 or no args. $# given."
	local PACKAGE=""
	for PACKAGE in $REQUIRED_CONTENT_PACKAGES $EXTRA_PACKAGES; do
		list_content_package "$1" "$PACKAGE" || return 1
	done
}
list_content_package() {
	#[ -z "$EXTRA_PACKAGES" ] || pinfo "Listing additional packages: $EXTRA_PACKAGES"
	[ $# -gt 2 ] && perror "'list_content_package' accepts max 2 args. $# given."
	local OP="-e"
	case "$1" in
		"--files")
			OP="-f"
			;;
		"--dirs")
			OP="-d"
			;;
		"")
			OP="-e"
			;;
		*)
			perror "'list_content_packages' invalid argument: $1"
			;;
	esac
	local PACKAGE="$2"
	local OPTIONAL="${PACKAGE:0:1}"
	[ "$OPTIONAL" = "@" ] && PACKAGE="${PACKAGE:1}"
	local FILES=""
	if [ "$PACKET_HANDLER" = "dpkg" ]; then
		PACKAGECOMMAND="dpkg -L"
	elif [ "$PACKET_HANDLER" = "rpm" ]; then
		PACKAGECOMMAND="rpm -ql"
	fi

	if [ -n "$REQUIRED_PACKET_FILES_BLACKLIST" ]; then
		FILES="$($PACKAGECOMMAND "$PACKAGE" | grep "^/" | \
			grep -v "$REQUIRED_PACKET_FILES_BLACKLIST" | \
			grep -v -E 'share/(man|doc)|/var/run|/var/log|/etc/init\.d'; \
			echo ":###:${PIPESTATUS[0]}")"
	else
		FILES="$($PACKAGECOMMAND "$PACKAGE" | grep "^/" | grep -v -E 'share/(man|doc)|/var/run|/var/log|/etc/init\.d'; echo ":###:${PIPESTATUS[0]}")"
	fi

	# ugly hack to get our return value
	local LPRET=$(echo "$FILES" | awk -F ':###:' '{printf $2}')
	FILES=$(echo "$FILES" | awk -F ':###:' '{print $1}')
	if [ "x$LPRET" != "x0" -a "x$OPTIONAL" != "x@" ]; then
		pdebug "FILES: '$FILES'"
		perror "dpkg/rpm exited with code '$LPRET' for required package ${PACKAGE}."
	fi
	[ "x$LPRET" != "x0" ] && pwarning "dpkg/rpm exited with code '$LPRET' for optional package ${PACKAGE}." && return 0
	[ -z "$FILES" ] && pwarning "list_packet_files empty for packet ${PACKAGE}." && return 0
	pdebug "Packet $PACKAGE has $(echo $FILES | wc -w) files..."
	for FILE in $FILES; do
		if [ "$OP" "$FILE" ] || [ -h "$FILE" ]; then
			echo "$FILE"
		fi
	done
	return 0
}
#
# Convenience function
#
list_packet_files() {
	list_content_packages --files
}

#
# Pass list of packages, returns filtered list, with packages removed
# that are already installed
#
filter_installed_packages() {
	local i
	if [ "$PACKET_HANDLER" == "dpkg" ]; then
		declare -A existing
		for i in $( dpkg -l "$@" 2> /dev/null | awk '$1 == "ii" {print $2}' ); do
			existing["$i"]=1
		done
		for i in "$@"; do
			[ -n "${existing["${i}"]}" ] || [ -n "${existing["${i}:${PLATFORM_DPKG}"]}" ] || echo "$i"
		done
	elif [ "$PACKET_HANDLER" == "rpm" ]; then
		for i in "$@"; do
			rpm --quiet -q "$i" &> /dev/null || echo "$i"
		done
	else
		pwarning "'Don't know how to determine installation status of packages for $PACKET_HANDLER"
		echo "$@"
	fi
}

#
# install individual package depending on package manager
#
install_packages() {
	if [ "$PACKET_MANAGER" == "apt" ]; then
		if ! apt-get install --no-install-recommends -y "$@"; then
			perror "install_packages: apt-get failed, see log"
		fi
	elif [ "$PACKET_MANAGER" == "zypper" ]; then
		if ! zypper --no-refresh --non-interactive install --force-resolution "$@"; then
			perror "install_packages: zypper failed, see log"
		fi
	elif [ "$PACKET_MANAGER" == "yum" ]; then
		if ! yum --assumeyes install "$@"; then
			perror "install_packages: yum failed, see log"
		fi
	else
		perror "No packet manager determined, this should not happen!"
	fi
}

