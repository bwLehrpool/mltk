
detect_distribution () {
	# Set up distribution and package management
	[ -z "$SYS_DISTRIBUTION" ] && perror "SYS_DISTRIBUTION not set (should be done by helper/distribution.inc)"
	# Then determine packet manager
	case "$SYS_DISTRIBUTION" in
		ubuntu)
			PACKET_MANAGER="apt"
			PACKET_HANDLER="dpkg"
			detect_ubuntu_lts
		;;
		debian)
			PACKET_MANAGER="apt"
			PACKET_HANDLER="dpkg"
		;;
		opensuse)
			PACKET_MANAGER="zypper"
			PACKET_HANDLER="rpm"
		;;
		centos|scientific|fedora)
			PACKET_MANAGER="yum"
			PACKET_HANDLER="rpm"
		;;
		*)
			perror "Unknown Distribution: $SYS_DISTRIBUTION - Please specify its packet manager in core/bin/setup_target"
		;;
	esac
	# Get version - we mangle this quite a bit. first make sure it has no spaces, then split version at period (.), underscore (_) and dash (-)
	if [ -n "$FORCE_SYS_VERSION" ]; then
		SYS_VERSION="$FORCE_SYS_VERSION"
	else
		SYS_VERSION=$(lsb_release -rs | tolower)
	fi
	SYS_CODENAME=$(lsb_release -c|cut -f 2)		# Codename: eg. Ubuntu raring, openSuse: Dartmouth etc.
	local VERSION=$(echo $SYS_VERSION | sed -r 's/\s//g;s/[\._]/ /g;s/-//g')
	local STRTMP=""
	PRINT_SYS_VERSIONS="*.conf.$SYS_DISTRIBUTION"
	SYS_VERSIONS="$SYS_DISTRIBUTION"
	for PART in $VERSION; do
		[ -z "$PART" ] && continue
		STRTMP+=".$PART"
		SYS_VERSIONS="${SYS_DISTRIBUTION}${STRTMP} $SYS_VERSIONS"
		PRINT_SYS_VERSIONS="*.conf.${SYS_DISTRIBUTION}${STRTMP} $PRINT_SYS_VERSIONS"
	done
	pinfo "Config source order: *.conf first, then the first one of these (if found)"
	pinfo "$PRINT_SYS_VERSIONS"
}

detect_ubuntu_lts () {
	local TMP=$(dpkg -S /usr/bin/Xorg)
	[[ "$TMP" == xserver-xorg* ]] || perror "Could not detect xserver package version (returned: $TMP)"
	TMP=${TMP%: *}
	TMP=${TMP#xserver-xorg-core}
	pinfo "Ubuntu LTS Xorg suffix: $TMP"
	UBUNTU_XORG_PKG_SUFFIX="$TMP"
}

