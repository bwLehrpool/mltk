umount_home() {
	umount /home/"${LOCALUSER}"/PERSISTENT	&& logger "openslx unmounter: umounted home (PERSISTENT)." \
						|| logger "openslx unmounter: could not home (PERSISTENT)!"
}

umount_shares() {
	index=0
	for SHARE in $(	mount | grep SHARE | tr -s ' ' | cut -f 3 -d " " ); do
		umount "${SHARE}"	&& logger "openslx unmounter: umounted ${SHARE}." \
					|| logger "openslx unmounter: could not umount ${SHARE}!"
	done
}
