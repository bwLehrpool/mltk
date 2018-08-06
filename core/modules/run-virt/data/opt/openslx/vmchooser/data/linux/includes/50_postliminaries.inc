postliminaries_native()
{
	# USERHOMEDIR=~/PERSISTENT, not ~! ################
	rmdir "$USERHOMEDIR"	&& logger "openslx unmounter: home directory entry (PERSISTENT) deleted." \
				|| logger "openslx unmounter: could not delete home directory (PERSISTENT) entry - not empty / unmounted!"
}

postliminaries_vmware()
{
	unlink "$USERHOMEDIR"
	ERR=$?
	if [ "$ERR" -ne 0 ]; then
		logger "openslx unmounter: vmware mode: could not unlink ${USERHOMEDIR}!"
		logger "openslx unmounter: vmware mode: this is a severe problem; I do not know what to do - exiting!"
		exit 1
	else
		logger "openslx unmounter: vmware mode: ${USERHOMEDIR} unlinked."
		# checking whether there was a directory moved out earlier:
		[ -d "${USERHOMEDIR}"_mov ] && mv "${USERHOMEDIR}"_mov "${USERHOMEDIR}"
	fi
}
