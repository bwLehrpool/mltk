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
#			Common functions for chrooting
#
# -----------------------------------------------------------------------------

declare -rg CHROOT_TEMPDIR="${ROOT_DIR}/tmp/chroot"
declare -rg CHROOT_MOUNTDIR="${CHROOT_TEMPDIR}/rootmount"
declare -rg CHROOT_BINDDIR="${CHROOT_TEMPDIR}/rootbind"
declare -rg CHROOT_TMPFS="${CHROOT_TEMPDIR}/upper"
declare -rg CHROOT_LOWERDIR="/"
declare -rg CHROOT_BINDMOUNTS="/dev /proc /sys /run"

# Helper for the helper to detect overlay filesystem name
chroot_detect_overlayfs() {
	declare -g OVERLAYFS_FSTYPES="$(grep -oE '\boverlay(fs)?$' /proc/filesystems)"
	[ -z "$OVERLAYFS_FSTYPES" ] && return 1
	# at least one overlayfs in /proc/filesystems
	# either one should work, will try them all later
	pdebug "Found overlayfs types: $OVERLAYFS_FSTYPES"
	readonly OVERLAYFS_FSTYPES
	return 0
}
# Helper to make sure we can use overlayfs
chroot_init_overlayfs() {
	[ -n "$OVERLAYFS_FSTYPES" ] && return 0
	# check if we already support it
	chroot_detect_overlayfs && return 0

	# nothing found, try to load kernel module
	pwarning "No overlayfs found in /proc/filesystems, trying to load module..."
	for NAME in overlay overlayfs; do
		if modprobe "${NAME}"; then
			chroot_detect_overlayfs && break
		fi
	done
	if [ -z "$OVERLAYFS_FSTYPES" ]; then
		perror "Could not initialize overlayfs!"
	fi
}

# Helper function to setup the directory structure
chroot_prepare_dirs() {
	# first check if CHROOT_TEMPDIR exists
	local i
	if [ -d "${CHROOT_TEMPDIR}" ]; then
		for i in "${CHROOT_MOUNTDIR}" "${CHROOT_BINDDIR}" "${CHROOT_TMPFS}"; do
			# try to umount and rmdir
			pdebug "Unmounting ${i}"
			umount "${i}"
			if [ -d "${i}" ]; then
				rmdir "${i}" || perror "Could not remove CHROOT_MOUNTDIR '${i}', meaning it has stuff in it. Aborting..."
			fi
		done
	fi

	mkdir -p "${CHROOT_TEMPDIR}" || perror "Could not create base directory for mount directories $CHROOT_TEMPDIR."
	for DIR in "${CHROOT_BINDDIR}" "${CHROOT_MOUNTDIR}"; do
		mkdir -p "${DIR}" || perror "Could not create directory for mount directory $DIR."
	done
}

# Helper to mount the overlay structure:
# - bind mount system / to CHROOT_BINDDIR and make it read-only
# - make an overlay from CHROOT_LOWERDIR CHROOT_UPPERDIR
# - bind mount additional pseudo-fs (as given in CHROOT_BINDMOUNTS)
chroot_prepare_mounts() {
	# Newer kernels (5.x) cannot have upperdir as subdirectory of lowerdir - tmpfs
	mkdir -p "${CHROOT_TMPFS}"
	mount -t tmpfs -o size=4G chrootupper "${CHROOT_TMPFS}" || perror "Could not mount tmpfs as upperdir"
	mkdir -p "${CHROOT_UPPER_TMPFS}" || perror "Could not create ${CHROOT_UPPER_TMPFS}"
	rsync -axHAX "${CHROOT_UPPERDIR}/" "${CHROOT_UPPER_TMPFS}/" || perror "Could not put upperdir into upupdir"

	# lastly, mount / on CHROOT_BINDDIR and remount read-only
	# do NOT mount anything else after this, as you'd get duplicate entries in /proc/mounts
	mount -o bind "${CHROOT_LOWERDIR}" "${CHROOT_BINDDIR}" \
			|| perror "Could not bind-mount CHROOT_LOWERDIR '$CHROOT_LOWERDIR' to CHROOT_BINDDIR '$CHROOT_BINDDIR'."
	mount -o remount,ro,bind "${CHROOT_BINDDIR}"		|| perror "Could not remount CHROOT_BINDDIR '$CHROOT_BINDDIR' read-only."

	# check that it really is read-only
	< /proc/mounts  gawk -v want="${CHROOT_BINDDIR}" '{if ( $2 == want && $4 ~ /(^|,)ro($|,)/ ) ok=1} END {if (ok == 1) exit 0; exit 1}' \
			|| perror "CHROOT_BINDDIR '${CHROOT_BINDDIR}' is not read-only! Aborting..."

	# Note: The overlay fs mount syntax seems to be changed between Ubuntu 14.04.2 and 14.04.3 (Kernel 3.13 and 3.19). Instead of
	# checking overlay-modinfo (which may fail if overlayfs is not incorporated as module) or kernel versions, we simply try to
	# mount 'old school' first and then, if that fails, the new way to mount with workdir. See differences in mount syntax below.
	pinfo "Now mounting overlayfs ..."
	for OVERLAYFS_NAME in ${OVERLAYFS_FSTYPES}; do
		# We have to use a overlayfs workdir which _must_ be in the same filesystem as CHROOT_UPPERDIR. So
		# we traverse to the directory below CHROOT_UPPERDIR and mkdir/mktemp a workdir there. In the possible
		# case that CHROOT_UPPERDIR is the root dir of a filesystem there's nothing we can do but exit.
		mkdir -p "$CHROOT_WORKDIR"
		if [ -z "$CHROOT_WORKDIR" ] || ! [ -d "$CHROOT_WORKDIR" ]; then
			perror "Could not mkdir overlayfs workdir $CHROOT_WORKDIR for new overlayfs mount syntax."
		fi
		# Now we try to mount the overlayfs in the new fashion:
		mount -v -t "${OVERLAYFS_NAME}" "${OVERLAYFS_NAME}" \
			-o lowerdir="${CHROOT_BINDDIR}",upperdir="${CHROOT_UPPER_TMPFS}",workdir="${CHROOT_WORKDIR}" \
					"${CHROOT_MOUNTDIR}" \
				|| perror "Could not mount (overlayfs) $CHROOT_BINDDIR, $CHROOT_UPPER_TMPFS, ${CHROOT_WORKDIR} to $CHROOT_MOUNTDIR."
		pinfo "New overlayfs mount syntax has worked, commencing."
	done

	#  mount pseudo-filesystems
	for DIR in $CHROOT_BINDMOUNTS; do
		if ! [ -d "$DIR" ]; then
			pwarning "Skipping bind mount of inexistant directory: $DIR"
			continue
		fi
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
		#                                                     #
		#                     Warning!                        #
		#                                                     #
		#  This file is only meant to be executed within      #
		#  the specially chrooted mltk building environment.  #
		#                                                     #
		#  Do NOT execute it if you are not sure what you do, #
		#  it may be very harmful if being run in a normal    #
		#  system environment!                                #
		#                                                     #
		#######################################################
		export LD_PRELOAD=/bla.so
		echo "chroot started successfully."
	EOF

	# Preload bullshit for close_range
	gcc -xc - -o "${CHROOT_MOUNTDIR}/bla.so" -shared <<-PUFF
		#include <unistd.h>
		int close_range(unsigned int a, unsigned int b, int c)
		{
			if (b - a > 1000) b = a + 1000;
			for (unsigned int i = a; i < b; ++i) {
				close(i);
			}
			return 0;
		}
	PUFF
	[ -s "${CHROOT_MOUNTDIR}/bla.so" ] || exit 12

	# dump the piped input to it
	cat >> "${CHROOT_MOUNTDIR}/autoexec.bat"

	# make it executable
	chmod +x "${CHROOT_MOUNTDIR}/autoexec.bat" || perror "Failed to make '${CHROOT_MOUNTDIR}/autoexec.bat' exeutable."
}

chroot_handle_whiteouts() {
	local WHITEOUT_LIST="${CHROOT_UPPERDIR}/overlay.whiteout.list"
	rm -f -- "$WHITEOUT_LIST"
	mkdir -p "$(dirname "$WHITEOUT_LIST")" || perror "Could not create $(dirname "$WHITEOUT_LIST")"
	pdebug "Searching for overlayfs-whiteouts ..."
	for WHITEOUT in $(find "$CHROOT_UPPERDIR" \( -type c -perm 0000 \) -o -lname "(overlay-whiteout)"); do
		pdebug "Whiteout found: $WHITEOUT"
		echo "/./${WHITEOUT#$CHROOT_UPPERDIR}" >> "$WHITEOUT_LIST"
		rm -f -- "$WHITEOUT" || perror "Could not delete whiteout $WHITEOUT!"
	done
	[ -s "$WHITEOUT_LIST" ] && pinfo "Whiteout list dumped to '${CHROOT_UPPERDIR}/overlay.whiteout.list'"
	return 0
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

	local CHROOT_WORKDIR="${CHROOT_TMPFS}/workdir-$MODULE"
	local CHROOT_UPPER_TMPFS="${CHROOT_TMPFS}/upperdir-$MODULE"
	local debugdir="${ROOT_DIR}/tmp/debug-chroot-$MODULE"
	umount "$debugdir" 2> /dev/null
	rmdir "$debugdir" 2> /dev/null

	# init overlayfs
	chroot_init_overlayfs || perror "Failed to initialize overlayfs with $?."
	# Clean up previous mess
	chroot_cleanup_mounts
	# first prepare the dir structure
	chroot_prepare_dirs || perror "'chroot_prepare_dirs' failed with $?."
	chroot_prepare_mounts || perror "'chroot_prepare_mounts' failed with $?."

	# generate the code to be executed when chroot'ing
	chroot_gen_autoexec || perror "'chroot_gen_autoexec' failed with $?."

	# do the chroot
	exec 0>&8		# This redirection is used for debugging within a chroot
	chroot --userspec root:root "${CHROOT_MOUNTDIR}" /autoexec.bat
	local RET=$?
	#local i
	#for i in $CHROOT_BINDMOUNTS; do
	#	umount -lf "${CHROOT_MOUNTDIR}/$i"
	#done
	#umount -lf "${CHROOT_MOUNTDIR}"
	if [ "$RET" -eq 0 ]; then
		pinfo "chroot executed '${CHROOT_MOUNTDIR}/autoexec.bat' succeeded."
		if rsync -axHAX "${CHROOT_UPPER_TMPFS}/" "${CHROOT_UPPERDIR}/"; then
			umount -l -f "${CHROOT_TMPFS}" || pwarning "Cannot umount $CHROOT_TMPFS"
		else
			pwarning "Could not properly rsync UPPER_TMPFS to UPPERDIR. tmpfs at '${CHROOT_UPPER_TMPFS}' will not be unmounted for further inspection."
		fi
	else
		perror "Failed to run '/autoexec.bat' inside the chroot '$CHROOT_MOUNTDIR' with error code: $RET. You can examine the upperdir at ${CHROOT_UPPER_TMPFS}. It's a tmpfs that hasn't been unmounted."
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
	if [ "x$(mount | grep "$(readlink -f "$MOUNT")")" != "x" ]; then
		# still mounted
		pdebug "'$MOUNT' is mounted!"
		return 1
	else
		pdebug "'$MOUNT' is not mounted."
		return 0
	fi
}

# Helper to cleanup the temporary mounts
chroot_cleanup_mounts() {
	local exe FILE pid tries
	local DOKILL=
	for tries in 1 2 0; do
		for exe in /proc/*/exe; do
			pid=${exe#/proc/}
			pid=${pid%/exe}
			FILE=$(readlink -f "$exe")
			if [ "${FILE#$CHROOT_TEMPDIR}" != "$FILE" ]; then
				pwarning "Killing $FILE ($pid)"
				kill $DOKILL "$pid"
				DOKILL="-9"
				kill "$pid"
			fi
		done
		[ -z "$DOSLEEP" ] && break
		sleep "$tries"
	done
	local i mnt ok
	for i in "" "" "" "-l" "-lf"; do
		ok=true
		for mnt in $(awk -v "dir=^${CHROOT_TEMPDIR}" '{if ($2 ~ dir) print $2}' /proc/mounts); do
			[ "${mnt%/}" = "${CHROOT_TMPFS%/}" ] && continue
			pdebug "Unmounting $mnt"
			umount $i "$mnt" || ok=false
		done
		$ok && break
	done
	$ok || perror "Could not unmount all chroot related dirs."
}
