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
#			Common functions for chrooting
#
# -----------------------------------------------------------------------------

declare -rg CHROOT_TEMPDIR="${ROOT_DIR}/tmp/chroot"
declare -rg CHROOT_MOUNTDIR="${CHROOT_TEMPDIR}/rootmount"
declare -rg CHROOT_BINDDIR="${CHROOT_TEMPDIR}/rootbind"
declare -rg CHROOT_LOWERDIR="/"
declare -rg CHROOT_BINDMOUNTS="/dev /proc /sys /run"


# Helper function to setup the directory structure
chroot_prepare_dirs() {
	# first check if CHROOT_TEMPDIR exists
	if [ -d "${CHROOT_TEMPDIR}" ]; then
		# try to umount and rmdir CHROOT_MOUNTDIR
		umount "${CHROOT_MOUNTDIR}" 2>/dev/null
		if [ -d "${CHROOT_MOUNTDIR}" ]; then
			rmdir "${CHROOT_MOUNTDIR}" || perror "Could not remove CHROOT_MOUNTDIR '${CHROOT_MOUNTDIR}', meaning it has stuff in it. Aborting..."
		fi

		# try to umount and rmdir CHROOT_BINDDIR
		umount "${CHROOT_BINDDIR}" 2>/dev/null
		if [ -d "${CHROOT_BINDDIR}" ]; then
		 	rmdir "${CHROOT_BINDDIR}" || perror "Could not remove CHROOT_BINDDIR '${CHROOT_BINDDIR}', meaning it has stuff in it. Aborting..."
		fi

		# try to rmdir CHROOT_TEMPDIR
		if [ -d "${CHROOT_TEMPDIR}" ]; then
			rmdir "${CHROOT_TEMPDIR}" || perror "Could not remove CHROOT_TEMPDIR '${CHROOT_TEMPDIR}', meaning it has stuff in it. Aborting..."
		fi
	fi

	mkdir -p "${CHROOT_TEMPDIR}"	|| perror "Could not create base directory for mount directories $CHROOT_TEMPDIR."
	for DIR in "${CHROOT_BINDDIR}" "${CHROOT_MOUNTDIR}"; do
		mkdir -p "${DIR}"			|| perror "Could not create directory for mount directory $DIR."
	done
}

# Helper to mount the overlay structure:
#	- bind mount system / to CHROOT_BINDDIR and make it read-only
#	- make an overlay from CHROOT_LOWERDIR CHROOT_UPPERDIR
#	- bind mount additional pseudo-fs (as given in CHROOT_BINDMOUNTS)
chroot_prepare_mounts() {

	# first mount / on CHROOT_BINDDIR and remount read-only
	mount -o bind "${CHROOT_LOWERDIR}" "${CHROOT_BINDDIR}" \
			|| perror "Could not bind-mount CHROOT_LOWERDIR '$CHROOT_LOWERDIR' to CHROOT_BINDDIR '$CHROOT_BINDDIR'."
	mount -o remount,ro,bind "${CHROOT_BINDDIR}"		|| perror "Could not remount CHROOT_BINDDIR '$CHROOT_BINDDIR' read-only."

	# check that it really is read-only
	[ "x$(mount | grep -E "^/ on ${CHROOT_BINDDIR}" | grep -v '\(.*ro.*\)')" != "x" ] \
			&& perror "CHROOT_BINDDIR '${CHROOT_BINDDIR}' is not read-only! Aborting..."

	# Note: The overlay fs mount syntax seems to be changed between Ubuntu 14.04.2 and 14.04.3 (Kernel 3.13 and 3.19). Instead of
	# checking overlay-modinfo (which may fail if overlayfs is not incorporated as module) or kernel versions, we simply try to
	# mount 'old school' first and then, if that fails, the new way to mount with workdir. See differences in mount syntax below.
	pinfo "Now mounting overlayfs. Trying old mount syntax (up to Kernel 3.13) ..."
	mount -t overlayfs overlayfs -o lowerdir="${CHROOT_BINDDIR}",upperdir="${CHROOT_UPPERDIR}" "${CHROOT_MOUNTDIR}" 2>/dev/null
	if [ $? -ne 0 ]; then
		pinfo "Old mount syntax failed. Trying new mount syntax (Kernel 3.19+) ..."
		# We have to use a overlayfs workdir which _must_ be in the same filesystem as CHROOT_UPPERDIR. So
		# we traverse to the directory below CHROOT_UPPERDIR and mkdir/mktemp a workdir there. In the possible
		# case that CHROOT_UPPERDIR is the root dir of a filesystem there's nothing we can do but exit.
		CHROOT_WORKDIR="$(mktemp -d $(dirname ${CHROOT_UPPERDIR})/workdirXXX)" \
			|| perror "Could not mkdir overlayfs workdir $CHROOT_WORKDIR for new overlayfs mount syntax."
		# Now we try to mount the overlayfs in the new fashion:
		mount -t overlayfs overlayfs -o lowerdir="$CHROOT_LOWERDIR",upperdir="${CHROOT_UPPERDIR}",workdir="${CHROOT_WORKDIR}" "${CHROOT_MOUNTDIR}" \
			|| perror "Could not mount (overlayfs) $CHROOT_LOWERDIR, $CHROOT_UPPERDIR to $CHROOT_BINDDIR."
		pinfo "New overlayfs mount syntax has worked, commencing."
	else
		pinfo "Old overlayfs mount syntax has worked, commencing."
	fi

	#  mount pseudo-filesystems
	for DIR in $CHROOT_BINDMOUNTS; do
		mount -o bind "${DIR}" "${CHROOT_MOUNTDIR}/${DIR}" \
			|| perror "Could not bind mount '$DIR' into CHROOT_MOUNTDIR/DIR '$CHROOT_MOUNTDIR/$DIR'."
	done
}

# Helper to generate the mighty autoexec.bat
chroot_gen_autoexec() {
	# create the script to be automatically executed.
	cat >"${CHROOT_MOUNTDIR}/autoexec.bat"<<-EOF
		#!/bin/bash
		#######################################################
		#						      #
		#		    Warning!			      #
		#						      #
		#  This	file is only meant to be executed within      #
		#  the specially chrooted mltk building environment.  # 
		#						      #
		#  Do NOT execute it if you are not sure what you do, #
		#  it may be very harmful if being run in a normal    #
		#  system environment!				      #
		#						      #
		#######################################################
		echo "chroot started successfully."
	EOF

	# dump the piped input to it
	cat >> "${CHROOT_MOUNTDIR}/autoexec.bat"

	# make it executable
	chmod +x "${CHROOT_MOUNTDIR}/autoexec.bat" || perror "Failed to make '${CHROOT_MOUNTDIR}/autoexec.bat' exeutable."
}

chroot_handle_whiteouts() {
	local WHITEOUT_LIST="${CHROOT_UPPERDIR}/overlay.whiteout.list"
	rm -f -- "$WHITEOUT_LIST="
	#mkdir -p "$(dirname "$WHITEOUT_LIST")" || perror "Could not create $(dirname "$WHITEOUT_LIST")"
	pdebug "Searching for overlayfs-whiteouts ..."
	for WHITEOUT in $(find "$CHROOT_UPPERDIR" -lname "(overlay-whiteout)"); do
		pdebug "Whiteout found: $WHITEOUT"
		echo "/./${WHITEOUT#$CHROOT_UPPERDIR}" >> "$WHITEOUT_LIST"
		rm -f -- "$WHITEOUT" || perror "Could not delete whiteout $WHITEOUT!"
	done
	pinfo "Whiteout list dumped to '${CHROOT_UPPERDIR}/overlay.whiteout.list'"
}

###############################################################################
#
#				MAIN FUNCTION
#
# Main function to be called from the outside
# Usage:
#	chroot_run <build_dir> < <code_to_exec_in_chroot>
#
# Example:
#	chroot_run /tmp/chroot_build <<-EOF
#		echo "This will be executed inside the chroot"
#	EOF
#
# It will run:
#	- chroot_prepare
#	- chroot $CHROOT_TEMPDIR/rootmount
#	- executes $CHROOT_TEMPDIR/rootmount/autoexec.bat
#	- chroot_cleanup
chroot_run() {
	# check args
	[ $# -eq 1 ] || perror "'chroot_run' requires exactly 1 parameter. Given $#.  Use 'chroot_run <build_dir>'"

	local CHROOT_UPPERDIR="$1"
	mkdir -p "$1" 

	# first prepare the dir structure
	chroot_prepare_dirs || perror "'chroot_prepare_dirs' failed with $?."
	chroot_prepare_mounts || perror "'chroot_prepare_mounts' failed with $?."

	# generate the code to be executed when chroot'ing
	chroot_gen_autoexec || perror "'chroot_gen_autoexec' failed with $?."

	# do the chroot
	exec 0>&8		# This redirection is used for debugging within a chroot
	chroot --userspec root:root "${CHROOT_MOUNTDIR}" /autoexec.bat
	local RET=$?
	if [ "$RET" -eq 0 ]; then
		pinfo "chroot executed '${CHROOT_MOUNTDIR}/autoexec.bat' succeeded."
	else
		perror "Failed to run '$CHROOT_MOUNTDIR/autoexec.bat' inside the chroot to '$CHROOT_MOUNTDIR' with error code: $RET"
	fi

	# handle whiteouts
	chroot_handle_whiteouts || perror "'chroot_handle_whiteouts' failed with error code: $?"

	# finally cleanup all the mounting stuff we did previously
	chroot_cleanup_mounts			|| perror "'chroot_cleanup' failed with $?."
}

###############################################################################
#
#				CLEANUP FUNCTIONS
#
# Helper to check if the given path is mounted
chroot_check_mount_point() {
	[ "$#" -eq 1 ] || perror "'chroot_check_mount_point' called with $# arguements, only 1 accepted."
	local MOUNT="$1"	
	if [ "x$(mount | grep "$(readlink -f $MOUNT)")" != "x" ]; then
		# still mounted
		pdebug "'$MOUNT' is mounted!"
		return 1
	else
		pdebug "'$MOUNT' is not mounted."
		return 0
	fi
}

# Helper to umount the given path
chroot_umount() {
	[ "$#" -eq 1 ] || perror "'chroot_umount' called with $# arguments, only 1 accepted."
	local MOUNT="$1"

	# check if MOUNT is mounted
	if ! chroot_check_mount_point "${MOUNT}"; then
		# still mounted
		if umount -l "${MOUNT}"; then
			pdebug "Successfully umounted '${MOUNT}'."
		else
			pwarning "Could not umount '${MOUNT}'! Trying again..."
			# now it gets ugly
			for i in `seq 1 5`; do
				umount -l "${MOUNT}" && return 0
			done
			perror "Could not umount '${MOUNT}' after 5 tries! This shouldn't happen. Check your scripts."
		fi
	else
		pdebug "'${MOUNT}' is not mounted."
	fi

	# better be safe than sorry
	chroot_check_mount_point "$MOUNT" || perror "'$MOUNT' is still mounted, exiting before something bad happens..."
}

# Helper to cleanup the temporary mounts
chroot_cleanup_mounts() {
	if [[ "$(mount | grep -c $CHROOT_TEMPDIR)" -gt 0 ]]; then	# No point in unmounting then...
		for DIR in $CHROOT_BINDMOUNTS; do
			chroot_umount "${CHROOT_MOUNTDIR}/${DIR}"
		done
		chroot_umount "${CHROOT_MOUNTDIR}"
		chroot_umount "${CHROOT_BINDDIR}"
	else
		pinfo "Nothing chroot-related is mounted - exiting."
	fi

	# In case of 'new' overlayfs mount - should perhaps be handled via flag...
	if [ -d "${CHROOT_WORKDIR}" ]; then
		# Too much of a coward to rm -rf somewhere. Both directories should be empty so we use rmdir.
		rmdir "${CHROOT_WORKDIR}/work" && pinfo "rmdir-ed CHROOT_WORKDIR/work ${CHROOT_WORKDIR}/work needed for new overlayfs mount syntax." \
				|| pinfo "Could not rmdir CHROOT_WORKDIR/work ${CHROOT_WORKDIR}/work - clean it by hand."
		rmdir "${CHROOT_WORKDIR}" && pinfo "rmdir-ed CHROOT_WORKDIR ${CHROOT_WORKDIR} needed for new overlayfs mount syntax." \
				  || pinfo "Could not rmdir CHROOT_WORKDIR ${CHROOT_WORKDIR} needed for new overlayfs mount syntax - clean by hand."
	fi
}
