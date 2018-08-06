preliminaries_native()
{
	# USERHOMEDIR=~/PERSISTENT, not ~! ################
	[ -h "$USERHOMEDIR" ]		&& unlink "$USERHOMEDIR"
	[ ! -d "$USERHOMEDIR" ]         && mkdir -p "$USERHOMEDIR"
}

preliminaries_vmware()
{
	# USERHOMEDIR=~/PERSISTENT, not ~! ################
	if [ -d "$USERHOMEDIR" ]; then
		logger "openslx sharemapper: vmware mode: USERHOMEDIR ${USERHOMEDIR} is a directory, moving to ${USERHOMEDIR}_mov."
		mv "$USERHOMEDIR" "${USERHOMEDIR}"_mov
	fi

	[ -h "$USERHOMEDIR" ]		&& unlink "$USERHOMEDIR"
	logger "openslx sharemapper: vmware mode: linking $USERHOMEDIR to /mnt/hgfs/home."
	ln -s /mnt/hgfs/home "$USERHOMEDIR"	|| \
		logger "openslx sharemapper: vmware mode: Could not link to vmware hgfs mount dir."
}
