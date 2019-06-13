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
	[ -z "$REQUIRED_CONTENT_PACKAGES" ] && pinfo "No required packages for $TOOL"  && return 1
	[ $# -gt 2 ] && perror "'list_content_packages' accepts only 1 or no args. $# given."
	local PACKAGE=""
	for PACKAGE in $REQUIRED_CONTENT_PACKAGES $EXTRA_PACKAGES; do
		list_content_package $1 $PACKAGE
	done

}
list_content_package(){
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
	local OPTIONAL="$(echo "$PACKAGE" | cut -c 1)"
	[ "x$OPTIONAL" = "x@" ] && PACKAGE="$(echo "$PACKAGE" | cut -c 2-)"
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
	[ "x$LPRET" != "x0" ] && pwarning "dpkg/rpm exited with code '$LPRET' for optional package ${PACKAGE}." && return
	[ -z "$FILES" ] && pwarning "list_packet_files empty for packet ${PACKAGE}." && return
	pdebug "Packet $PACKAGE has $(echo $FILES | wc -w) files..."
	for FILE in $FILES; do
		if [ "$OP" "$FILE" ] || [ -h "$FILE" ]; then
			echo "$FILE"
		fi
	done
}
#
# Convenience function
#
list_packet_files() {
	list_content_packages --files
}

#
# Convenience function
#
# install all dependencies of a module
# goes through all package as given in the variable REQUIRED_INSTALLED_PACKAGES
install_dependencies() {
	[ -z "${REQUIRED_INSTALLED_PACKAGES}${REQUIRED_CONTENT_PACKAGES}" ] && return
	# add all REQUIRED_CONTENT_PACKAGES that are not tagged as optional
	local TO_INSTALL="$REQUIRED_INSTALLED_PACKAGES"
	for pkg in $REQUIRED_CONTENT_PACKAGES ; do
		if [ "${pkg:0:1}" != "@" ]; then
			TO_INSTALL="$TO_INSTALL $pkg"
		fi
	done
	install_packages "$TO_INSTALL"
}

#
# install given packet through system's packet manager
# uses PACKET_HANDLER as determined in helper/system.inc
#
install_packages() {
	[ $# -eq 0 ] && perror "Sanity check failed: no argument given to install_package"
	local PACKAGE_LIST="$@"
	local INSTALLED_PACKAGES=""
	local LRET=""

	for PKG in ${PACKAGE_LIST}; do
		# check if installed:
		# this is done mainly for performance reasons, even though
		# reinstalling all packages is a valid possibility too...
		if [ "x$PACKET_HANDLER" == "xdpkg" ]; then
			dpkg -l ${PKG} > /dev/null 2>&1
			LRET=$?
		elif [ "x$PACKET_HANDLER" == "xrpm" ]; then
			rpm -ql "${PKG}" > /dev/null 2>&1
			LRET=$?
		else
			perror "No packet manager / handler determined, this should not happen!"
		fi

		if [ "x$LRET" == "x0" ]; then
			# check if it is completly installed, not just leftover configuration files
			if [ "x$PACKET_HANDLER" == "xdpkg" ]; then
				local INSTALL_STATUS=$(dpkg -l "${PKG}" | grep "${PKG}" | cut -c1-2)
				if [[ $INSTALL_STATUS != "ii" ]]; then
					pinfo "$PKG is either missing or a meta-package! Installing it to be sure..."
					install_package "${PKG}"
				else
					# package installed
					pdebug "${PKG} installed!"
				fi
			elif [ "x$PACKET_HANDLER" == "xrpm" ]; then
				# TODO: blindly install again for now
				rpm -q "${PKG}" >/dev/null 2>&1 || install_package "${PKG}"
			fi
		else
			# package not installed
			pdebug "$PKG not installed!"
			install_package $PKG
		fi
	done
	[ ! -z "$INSTALLED_PACKAGES" ] && pinfo "New packages installed: ${INSTALLED_PACKAGES}"
}

#
# install individual package depending on package manager
#
install_package() {
	if [ "$#" -ne 1 ]; then
		perror "Only call install_package with one argument!"
	fi

	if [ "x$PACKET_MANAGER" == "xapt" ]; then
		apt-get install -y ${PKG}
		local IRET=$?
		if [ "x$IRET" == "x0" ]; then
			# $PGK was installed successfully
			INSTALLED_PACKAGES+="$PKG "
		else
			# PKG was not installed
			# TODO error handling
			perror "install_packages: apt-get failed with '$IRET' for package '$PKG'"
		fi
	elif [ "x$PACKET_MANAGER" == "xzypper" ]; then
		zypper --no-refresh --non-interactive install --force-resolution ${PKG}
		local IRET=$?
		if [ "x$IRET" == "x0" ]; then
		# $PGK was installed successfully
			INSTALLED_PACKAGES+="$PKG "
		else
			# PKG was not installed
			# TODO error handling
			perror "install_packages: zypper failed with '$IRET' for package '$PKG'"
		fi
	elif [ "x$PACKET_MANAGER" == "xyum" ]; then
		yum --assumeyes install ${PKG}
		local IRET=$?
		if [ "x$IRET" == "x0" ]; then
			# $PGK was installed successfully
			INSTALLED_PACKAGES+="$PKG "
		else
			# PKG was not installed
			# TODO error handling
			perror "install_packages: yum failed with '$IRET' for package '$PKG'"
		fi
	else
		perror "No packet manager determined, this should not happen!"
	fi
}

