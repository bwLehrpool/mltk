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
