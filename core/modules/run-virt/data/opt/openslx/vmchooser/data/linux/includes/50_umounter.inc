umount_all() {
	local HOME
	local WEBDAV="(webdav) "
	local UMOUNT
	get_shares	# fills array ROHSHARES; row 1+ share infos from (shares-)CONFIGFILE

	for (( CONFIGROW = 1; CONFIGROW < ${#ROHSHARES[@]}; CONFIGROW++ )); do		# row 1 is homedir, 2+ shares
		SHAREPATH=$(echo "${ROHSHARES[CONFIGROW]}" | cut -f 1 -d$'\t')          # Could be a webdav share...
		SHARENAME=$(echo "${ROHSHARES[CONFIGROW]}" | cut -f 3 -d$'\t' | sed 's/ /_/g')
		if [ "${SHARENAME:0:5}" == "Home-" ]; then
			HOME="home "
			UMOUNT="${USERHOMEDIR}"
		else
			HOME=""
			# if there's no sharename let's at least try with SHARELETTER:
			[ -z "${SHARENAME}" ] && SHARENAME=$(echo "${ROHSHARES[CONFIGROW]}" | cut -f 2 -d$'\t' | sed 's/://g')
			UMOUNT="${USERHOME}/${SHARENAME}"
		fi

		if [ "${SHAREPATH:0:4}" == 'http' ]; then
			fusermount -u "$UMOUNT"	&& logger "openslx umounter: umounted ${HOME}${WEBDAV} ${UMOUNT}." \
						|| logger "openslx umounter: could not umount ${HOME}${WEBDAV} ${UMOUNT}!"
		else
			umount "${UMOUNT}"	&& logger "openslx umounter: umounted $HOME ${UMOUNT}." \
						|| logger "openslx umounter: could not umount ${HOME}${UMOUNT}!"
		fi
	done
}

