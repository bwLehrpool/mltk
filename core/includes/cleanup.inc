#!/bin/bash
# -----------------------------------------------------------------------------
#
# Copyright (c) 2014..2018 bwLehrpool-Projektteam
#
# This program/file is free software distributed under the GPL version 2.
# See https://www.gnu.org/licenses/old-licenses/gpl-2.0-standalone.html
#
# If you have any feedback please consult https://bwlehrpool.de and
# send your feedback to support@bwlehrpool.de.
#
# General information about bwLehrpool can be found at https://bwlehrpool.de
#
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

