#!/bin/bash
activate_qt() {
	[ $# -eq 1 ] || perror "activate_qt() requires the version as only argument, $# given."
	local QT_VERSION="$1"
	# Handle environment
	export QT_SELECT="qt${QT_VERSION}"
	QTDIR="/usr/share/qt${QT_VERSION}"
	[ -d "$QTDIR" ] && export QTDIR
	# Handle qt chooser
	[ -d "/usr/share/qtchooser" ] || return 0 # No qtchooser found, hope for the best...
	local QT_DEFAULT="$(readlink -f /usr/share/qtchooser/default.conf)"
	if [ -e "$QT_DEFAULT" ] && [[ "$(basename "$QT_DEFAULT")" =~ ^qt${QT_VERSION}.*\.conf$ ]]; then
		: # Already default
	else
		# no default active, try to set it
		local QT_CANDIDATES="$(find "/usr/share/qtchooser" -name "qt$QT_VERSION*" | grep -E "$ARCHREGEX")"
		[ -z "$QT_CANDIDATES" ] && QT_CANDIDATES="$(find "/usr/share/qtchooser" -name "qt$QT_VERSION*")"
		local QTS="$(echo "$QT_CANDIDATES" | wc -w)"
		if [ "$QTS" -eq 1 ]; then
			# we found it, lets activate it and hope for the best
			ln -sf "$QT_CANDIDATES" "/usr/share/qtchooser/default.conf" \
					|| perror "Could not link 'default.conf' to '/usr/share/qtchooser/$QT_CANDIDATES'"
		elif [ "$QTS" -eq 0 ]; then
			perror "No qt ${QT_VERSION} config found in /usr/share/qtchooser"
		else
			# here we have the case with different qt4 files or none at all
			# in both cases, there is a bigger problem we shouldn't try to guess which one is correct
			perror "More than one qt ${QT_VERSION} version found in '/usr/share/qtchooser'. The configuration is messy. Fix it first."
		fi
	fi
}
