# -----------------------------------------------------------------------------
#
# Copyright (c) 2014 - OpenSLX GmbH
#
# This program is free software distributed under the GPL version 2.
# See http://openslx.org/COPYING
#
# If you have any feedback please consult http://openslx.org/feedback and
# send your suggestions, praise, or complaints to feedback@openslx.org
#
# General information about OpenSLX can be found at http://openslx.org/
# -----------------------------------------------------------------------------
#
#						Trapped cleanup functions
#
# -----------------------------------------------------------------------------

__init () {
	# run 'cleanexit' when CTRL-c is pressed, an abrupt program termination or exit happens
	trap cleanexit SIGINT SIGTERM
}

# main cleaner function
cleanexit() {
	trap '' SIGINT SIGTERM	# from now on, ignore INT and TERM
	unset_quiet		# needed to get trap functioning correctly
	pwarning "SIGINT/SIGTERM triggered - cleaning up ..."
	# unmount and remove the temporary chroot stuff
	pinfo "Calling chroot_cleanup_mounts ..."
	chroot_cleanup_mounts
	# TODO vmware etc/vmware/config stuff here, if it is still needed
	exit 1			# perhaps a better exit code?
}

