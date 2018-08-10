function mount_shares()
{
	for (( CONFIGROW = 1; CONFIGROW < ${#ROHSHARES[@]}; CONFIGROW++ )); do
		SHAREPATH=$(echo "${ROHSHARES[CONFIGROW]}" | cut -f 1 -d$'\t')          # cifs- oder nfs-Share
		SHARELETTER=$(echo "${ROHSHARES[CONFIGROW]}" | cut -f 2 -d$'\t' | \
					sed 's/://g')					# Laufwerksbuchstabe ohne :
		SHARENAME=$(echo "${ROHSHARES[CONFIGROW]}" | cut -f 3 -d$'\t' | sed 's/ /_/g')	# Leerzeichen raus.
		SHAREUSER=$(echo "${ROHSHARES[CONFIGROW]}" | cut -f 4 -d$'\t')		# Username, bei Userhome nicht vorhanden
		SHAREPASS=$(echo "${ROHSHARES[CONFIGROW]}" | cut -f 5 -d$'\t')		# User-PW, bei Userhome nicht vorhanden

		# Sharetyp bestimmen:
		if [ "${SHAREPATH:0:2}" == '\\' ]  || [ "${SHAREPATH:0:2}" == '//' ]; then
			USER=$(echo "$SHAREUSER"|cut -d '\' -f 2)
			SHAREPATH="${SHAREPATH//\\//}"					# '\' --> `/` :-)
			MOUNTER=cifs
			MOUNTPARAM="$MOUNTCIFSPRAE"
			MOUNTOPTS="$MOUNTCIFSOPTS"
		else
			MOUNTER=nfs
			MOUNTPARAM="$MOUNTNFSPRAE"
			MOUNTOPTS="$MOUNTNFSOPTS"
		fi

		# User-Homeverzeichnis?
		if [ "${SHARENAME:0:5}" == "Home-" ]; then
			logger "openslx sharemapper: home share \"$SHARENAME\" found (for PERSISTENT)."
			export USER="${GLOBALUSER}"
			export PASSWD="${PW}"
			mounter "${MOUNTPARAM}" "${MOUNTOPTS}" "${SHAREPATH}" "${USERHOMEDIR}" 2>/dev/null 1>&2 &
			unset USER
			unset PASSWD

			# linken: userhomedir <-sharename, userhomedir <- shareletter ln -s "$USERSHAREDIR" "${USERDESKTOP}"/"$SHARENAME"
			#								ln -s "$USERSHAREDIR" "${USERDESKTOP}"/"$SHARELETTER"
			logger "openslx sharemapper: Linking ${SHARELETTER} or ${SHARENAME}, if possible."
			if [ -n "${SHARENAME}" ]; then
				ln -s "$USERHOMEDIR" "${USERDESKTOP}"/"$SHARENAME"
				logger "openslx sharemapper: ${SHARENAME} linked."
			elif [ -n "${SHARELETTER}" ]; then
				ln -s "$USERHOMEDIR" "${USERDESKTOP}"/"$SHARELETTER"
				logger "openslx sharemapper: No sharename found, share letter ${SHARELETTER} linked."
			fi
		else
			if [ -z "${SHARELETTER}" ]; then
				logger "openslx sharemapper: $SHARELETTER not found. Do not know where to mount."
				continue
			else
				mount_share &
			fi
		fi
	done
}

mnt_shares() {
	logger "openslx sharemapper: Started."
	
	# Wait for network connection to base system
	x=1
	while ! ping -w 1 -c 1 "${NATADDR}" 2>/dev/null 1>&2; do
		[ "$x" -gt 20 ] && { logger "openslx sharemapper: could not reach base system. Giving up."; exit 1; }
		let x=x+1
		sleep 2
	done
	
	logger "openslx sharemapper: base system reaching; commencing."
	
	# REMAPMODE (remapMode):                0: None, 1 Native, 2 Native Fallback, 3 vmware
	# CREATEMISSING (createMissingRemap):   0: Nichts tun, 1 xdg-Verzeichnisse
	
	case "$REMAPMODE" in
		0)      logger "openslx sharemapper: remapMode 0 (do nothing) detected."
		        ;;
		1)      logger "openslx sharemapper: remapMode 1 (native mode) detected."
			preliminaries_native
			get_creds               # fills global var PW with (currently) decrypted password
			get_shares              # fills array ROHSHARES; row 0 global infos from (shares-)CONFIGFILE,
						# following rows:       column 1 share path, col 2 drive letter, col 3 share name,
						#                       column 4 username, col 5 password.
			mount_shares            # mounts shares given in array ROHSHARES.
			;;
		2)      logger "openslx sharemapper: remapMode 2 (fallback mode) detected."
			preliminaries_native
			get_creds               # fills global var PW with (currently) decrypted password
			get_shares              # fills array ROHSHARES; row 0 global infos from (shares-)CONFIGFILE,
			# following rows:       column 1 share path, col 2 drive letter, col 3 share name,
			#                       column 4 username, col 5 password.
			mount_shares            # mounts shares given in array ROHSHARES.
			check_fallback          # checks for a mounted home and fallbacks to vmware share, if none found.
			;;
		3)      logger "openslx sharemapper: remapMode 3 (vmware mode) detected."
			preliminaries_vmware
			mount_shares            # mounts shares given in array ROHSHARES - runvirt checks whether there's
                        			# a home share given or not..
			;;
		*)      logger "openslx sharemapper: unknown remapMode in $SLXCONFIGFILE; doing nothing."
			;;
	esac
	
	# By the way: We also have to start a bwLehrpool-Suite given script (runscript). For convenience we will start it here and 
	# after mounting shares, as perhaps a given script needs mounted shares.
	exit 0
}
