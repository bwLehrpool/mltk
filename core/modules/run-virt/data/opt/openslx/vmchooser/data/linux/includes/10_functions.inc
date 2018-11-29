### Funktionen ####################################

### Funktionen Mounter ############################

function xor()
{	local RES=($(echo "$1" | sed "s/../0x& /g"))
	local ONE=($(echo "$2" | sed "s/../0x& /g"))
	local COUNT1=${#RES[@]}
	local COUNT2=${#ONE[@]}
	for (( i = 0; i < $COUNT1; i++ )); do
		RES[$i]=$((${ONE[$(( i % COUNT2 ))]:-0} ^ ${RES[$i]:-0}))
	done
	printf "%02x" "${RES[@]}"
}

function already_mounted()
{
	# Ausgabe: gemountet = true = 0, nicht gemountet = false = 1
	local AUSGANG
	mount | grep -q " ${1} " && AUSGANG=0 || AUSGANG=1 
	return $AUSGANG
}

function mounter()
{
	# Ausgabe: konnte mounten: 0, konnte nicht mounten: 1, schon gemountet 2
	already_mounted "$4"
	ERR=$?
	if [ "$ERR" -eq 0 ]; then
		logger "openslx sharemapper: $3 already mounted."
		AUSGANG=2	
	else	
		case "$MOUNTER" in 
			nfs)	AUSGANG=0
				x=2
				while ! mount $1 $2 $3 $4 2>/dev/null 1>&2; do
					logger "openslx sharemapper: could not mount ${3} to ${4}, waited another $x seconds, retrying."
					sleep $x
					if [ "$x" -gt 6 ]; then
						AUSGANG=1
						logger "openslx sharemapper: timeout, could not mount ${3} to ${4}. mount.nfs installed?"
						break
					fi
					let x=x+2
				done
				[ "$AUSGANG" -eq 0 ] 	&& logger "openslx sharemapper: ${3} mounted to ${4} (nfs)."	# Todo: Schöner schreiben:)
				;;
			cifs)	AUSGANG=0
				x=2
				for VERSION in $CIFSVERSIONS; do
					AUSGANG=0
					while ! mount $1 vers="$VERSION",${2} $3 $4 2>/dev/null 1>&2; do
						logger "openslx sharemapper: could not mount ${3} to ${4}, waited another $x seconds, retrying."
						sleep $x
						if [ "$x" -gt 4 ]; then
							AUSGANG=1
							logger "openslx sharemapper: timeout, could not mount ${3} to ${4} cifs v${VERSION}."
							break
						fi
						let x=x+2
					done
					[ "$AUSGANG" -eq 0 ] && break
				done
				[ "$AUSGANG" -eq 0 ]    && logger "openslx sharemapper: ${3} mounted to ${4} (cifs $VERSION)."
				;;
			*)	logger "openslx sharemapper: unknown mounter ${MOUNTER}!"
				;;
		esac
	fi
	return $AUSGANG
}

function mount_share()
{
	# USERSHAREDIR="${USERHOME}"/SHARE_"${CONFIGROW}" # ROHSHARES: Zeilennummer; Shares >=1
	USERSHAREDIR="${USERHOME}"/"${SHARENAME}"
	if [ ! -d "$USERSHAREDIR" ]; then
		mkdir -p "$USERSHAREDIR" 2>/dev/null
		chown "$LOCALUSER":$(id --group "$LOCALUSER") "$USERSHAREDIR"
		chmod 700 "$USERSHAREDIR"
		logger "openslx sharemapper: share mount dir $USERSHAREDIR created."
	fi
	# Wenn kein Homeverzeichnis, dann share zuerst mit den Credentials aus der share-Konfig
	# versuchen zu mounten; wenn nicht, dann mit den Hauptcredentials nachversuchen.
	logger "openslx sharemapper: non-home share \"$SHARENAME\" (${CONFIGROW}) found."
	export USER="${SHAREUSER}"
	export PASSWD="${SHAREPASS}"
	mounter "${MOUNTPARAM}" "${MOUNTOPTS}" "${SHAREPATH}" "${USERSHAREDIR}" 2>/dev/null 1>&2
	ERR=$?
	unset USER
	unset PASSWD
	if [ "$ERR" -eq 1 ]; then
		export USER="${GLOBALUSER}"
		export PASSWD="${PW}"
		logger "openslx sharemapper: Could not mount ${USERSHAREDIR}, now trying using user credentials."
		mounter "${MOUNTPARAM}" "${MOUNTOPTS}" "${SHAREPATH}" "${USERSHAREDIR}" 2>/dev/null 1>&2
		ERR=$?                          # ERR merken wg. Links aus USERSHAREDIR
		if [ "$ERR" -eq 1 ]; then
			logger "openslx sharemapper: Could not mount ${USERSHAREDIR} even using user credentials; giving up."
			echo "Konnte Laufwerk ${SHAREPATH} nicht einhängen." >> "$USERLOGFILE"
		fi
		unset USER
		unset PASSWD
	fi
	if [ "$ERR" -eq 0 ]; then
		logger "openslx sharemapper: Linking ${SHARENAME}, if possible."

		if [ -n "${SHARENAME}" ]; then
			ln -s "$USERSHAREDIR" "${USERDESKTOP}"/"$SHARENAME"
			chown "$LOCALUSER" "${USERDESKTOP}"/"$SHARENAME"
			logger "openslx sharemapper: ${SHARENAME} linked."
		elif [ -n "${SHARELETTER}" ]; then
			ln -s "$USERSHAREDIR" "${USERDESKTOP}"/"$SHARELETTER"
			chown "$LOCALUSER" "${USERDESKTOP}"/"$SHARELETTER"
			logger "openslx sharemapper: No sharename found, share letter ${SHARELETTER} linked."
		fi

	else
		logger "openslx sharemapper: deleting previously creates share mount dir $USERSHAREDIR."
		rmdir "$USERSHAREDIR" 2>/dev/null
	fi
}

### Funktionen Mounter Ende #######################

### Funktionen Ende ###############################

