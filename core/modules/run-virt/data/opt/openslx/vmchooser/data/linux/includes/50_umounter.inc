umount_home() {
	umount "${USERHOMEDIR}"			&& logger "openslx umounter: umounted home (${USERHOMEDIR})." \
							|| logger "openslx umounter: could not home(${USERHOMEDIR})!"
}

umount_shares() {
	get_shares	# fills array ROHSHARES; row 0 global infos from (shares-)CONFIGFILE,

	for (( CONFIGROW = 2; CONFIGROW < ${#ROHSHARES[@]}; CONFIGROW++ )); do		# row 1 always home, so forget...
		SHAREPATH=$(echo "${ROHSHARES[CONFIGROW]}" | cut -f 1 -d$'\t')          # Could be a webdav share...
		SHARENAME=$(echo "${ROHSHARES[CONFIGROW]}" | cut -f 3 -d$'\t' | sed 's/ /_/g')
		[ -z "${SHARENAME}" ] && SHARENAME=$(echo "${ROHSHARES[CONFIGROW]}" | cut -f 2 -d$'\t' | sed 's/://g') # yields SHARELETTER

		if [ "${SHAREPATH:0:4}" == 'http' ]; then
			fusermount -u "${USERHOME}/${SHARENAME}"	&& logger "openslx umounter: umounted ${SHARENAME}. (webdav)" \
									|| logger "openslx umounter: could not umount ${SHARENAME}! (webdav)"
		else
			umount "${USERHOME}/${SHARENAME}"		&& logger "openslx umounter: umounted ${SHARE}." \
									|| logger "openslx umounter: could not umount ${SHARE}!"
		fi
	done
}
