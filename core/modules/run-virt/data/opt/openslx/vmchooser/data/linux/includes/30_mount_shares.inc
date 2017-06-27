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
			MOUNTOPTS="$MOUNTOPTSCIFS"
		else
			MOUNTOPTS="$MOUNTOPTSNFS"
		fi

		# User-Homeverzeichnis?
		if [ "${SHARENAME:0:5}" == "Home-" ]; then
			logger "openslx sharemapper: home share \"$SHARENAME\" found (for PERSISTENT)."
			export USER="${GLOBALUSER}"
			export PASSWD="${PW}"
			mounter "${MOUNTOPTS}" "${SHAREPATH}" "${USERHOMEDIR}" 2>/dev/null 1>&2
			unset USER
			unset PASSWD
		else
			if [ -z "${SHARELETTER}" ]; then
				logger "openslx sharemapper: $SHARELETTER not found. Do not know where to mount."
				continue
			else
				USERSHAREDIR="${USERHOME}"/SHARE_"${CONFIGROW}"	# ROHSHARES: Zeilennummer; Shares >=1
				if [ ! -d "$USERSHAREDIR" ]; then
					mkdir -p "$USERSHAREDIR" 2>/dev/null
					chown "$LOCALUSER":$(id --group "$LOCALUSER") "$USERSHAREDIR"
					chmod 700 "$USERSHAREDIR"
					logger "openslx sharemapper: share mount dir $USERSHAREDIR created."
				fi
			fi
			# Wenn kein Homeverzeichnis, dann share zuerst mit den Credentials aus der share-Konfig
			# versuchen zu mounten; wenn nicht, dann mit den Hauptcredentials nachversuchen.
			logger "openslx sharemapper: non-home share \"$SHARENAME\" (${CONFIGROW}) found."
			export USER="${SHAREUSER}"
			export PASSWD="${SHAREPASS}"
			mounter "${MOUNTOPTS}" ${SHAREPATH} ${USERSHAREDIR} 2>/dev/null 1>&2
			ERR=$?
			unset USER
			unset PASSWD
			if [ "$ERR" -eq 1 ]; then
				export USER="${GLOBALUSER}"
				export PASSWD="${PW}"
				logger "openslx sharemapper: Could not mount ${USERSHAREDIR}, now trying using user credentials."
				mounter "${MOUNTOPTS}" ${SHAREPATH} ${USERSHAREDIR} 2>/dev/null 1>&2 
				ERR=$?				# ERR merken wg. Links aus USERSHAREDIR
				if [ "$ERR" -eq 1 ]; then
					logger "openslx sharemapper: Could not mount ${USERSHAREDIR} even using user credentials; giving up."
				fi
				unset USER
				unset PASSWD
			fi
			if [ "$ERR" -eq 0 ]; then
				logger "openslx sharemapper: Linking ${SHARELETTER} and $SHARENAME, if possible."
				[ -n "${SHARELETTER}" ] && ln -s "$USERSHAREDIR" "${USERHOME}"/"$SHARELETTER"
				[ -n "${SHARELETTER}" ] && ln -s "$USERSHAREDIR" "${USERHOME}"/"$SHARENAME"
			fi
		fi
	done
}
