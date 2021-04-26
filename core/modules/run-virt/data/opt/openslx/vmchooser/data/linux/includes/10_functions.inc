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
	MOUNTPARAM="$1"
	MOUNTOPTS="$2"
	SHAREPATH="$3"
	MOUNTDIR="$4"

	# Ausgabe: konnte mounten: 0, konnte nicht mounten: 1, schon gemountet 2
	already_mounted "$MOUNTDIR"
	ERR=$?
	if [ "$ERR" -eq 0 ]; then
		logger "openslx sharemapper: $SHAREPATH already mounted."
		AUSGANG=2	
	else	
		case "$MOUNTER" in 
			nfs)	AUSGANG=0
				x=2
				while ! mount $MOUNTPARAM "${MOUNTOPTS}" "$SHAREPATH" "$MOUNTDIR" 2>/dev/null 1>&2; do
					logger "openslx sharemapper: could not mount ${SHAREPATH} to ${MOUNTDIR}, waited another $x seconds, retrying."
					sleep $x
					if [ "$x" -gt 6 ]; then
						AUSGANG=1
						logger "openslx sharemapper: timeout, could not mount ${SHAREPATH} to ${MOUNTDIR}. mount.nfs installed?"
						break
					fi
					let x=x+2
				done
				[ "$AUSGANG" -eq 0 ] 	&& logger "openslx sharemapper: ${SHAREPATH} mounted to ${MOUNTDIR} (nfs)."	# Todo: Schöner schreiben:)
				;;
			cifs)	AUSGANG=0
				x=2
				[ -n "$GLOBALDOMAIN" ] && MOUNTOPTS="${MOUNTOPTS},domain=$GLOBALDOMAIN"
				for VERSION in $CIFSVERSIONS; do
					AUSGANG=0
					while ! mount $MOUNTPARAM "${VERSION}","${MOUNTOPTS}" "$SHAREPATH" "$MOUNTDIR" 2>/dev/null 1>&2; do
						logger "openslx sharemapper: could not mount ${SHAREPATH} to ${MOUNTDIR}, waited another $x seconds, retrying."
						sleep $x
						if [ "$x" -gt 4 ]; then
							AUSGANG=1
							logger "openslx sharemapper: timeout, could not mount ${SHAREPATH} to ${MOUNTDIR} cifs v${VERSION}."
							break
						fi
						let x=x+2
					done
					[ "$AUSGANG" -eq 0 ] && break
				done
				[ "$AUSGANG" -eq 0 ]    && logger "openslx sharemapper: ${SHAREPATH} mounted to ${MOUNTDIR} (cifs $VERSION)."
				;;
			dav)    AUSGANG=0
				x=2
				while ! echo -e "${USER}\n${PASSWD}" | mount $MOUNTPARAM "$MOUNTOPTS" "$SHAREPATH" "$MOUNTDIR" 2>/dev/null 1>&2; do
					logger "openslx sharemapper: could not mount ${SHAREPATH} to ${MOUNTDIR}, waited another $x seconds, retrying."
					sleep x
					if [ "$x" -gt 4 ]; then
						AUSGANG=1
						logger "openslx sharemapper: timeout, could not mount ${SHAREPATH} to ${MOUNTDIR}. mount.davfs installed?"
						break
					fi
					let x=x+2
				done
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
			echo "Konnte Laufwerk ${SHAREPATH} nicht einhängen. mount.cifs / mount.davfs installiert?" >> "$USERLOGFILE"
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

function mount_home()
{
	logger "openslx sharemapper: home share \"$SHARENAME\" found (for PERSISTENT)."
	export USER="${GLOBALUSER}"
	export PASSWD="${PW}"
	ERRORMSG=""
	mounter "${MOUNTPARAM}" "${MOUNTOPTS}" "${SHAREPATH}" "${USERHOMEDIR}" 2>/dev/null 1>&2
	ERR=$?
	unset USER
	unset PASSWD
	
	case $ERR in	1)	logger "openslx sharemapper: could not mount home share."
				echo "Konnte Home-Laufwerk nicht einhängen!" >> "$USERLOGFILE"
				;;
			2)      logger "openslx sharemapper: home share seems to be already mounted."
				;;
			0)	logger "openslx sharemapper: Linking ${USERDESKTOP}/$(basename $USERHOMEDIR), if possible."
				# Changed: Link on Desktop no longer "Home-verz.", but same
				# name as last part of $USERHOMEDIR (-->PERSISTENT)
				# ln -s "$USERHOMEDIR" "${USERDESKTOP}"/"$SHARENAME"
				ln -s "$USERHOMEDIR" "${USERDESKTOP}"/$(basename "$USERHOMEDIR")
				chown "$LOCALUSER" "${USERDESKTOP}"/$(basename "$USERHOMEDIR")
				logger "openslx sharemapper: "${USERDESKTOP}"/$(basename "$USERHOMEDIR") linked."
				;;
			*)      logger "openslx sharemapper: Unknown error level mounting home share."
			;;
	esac
}

### Funktionen Mounter Ende #######################

set_userpart()
{
	case "$SCRIPTUSER" in
		0)      USERPART=""
			;;
		1)      USERPART=" su -c - $LOCALUSER "
			;;
		*)      logger "openslx utility: runscript: Unknown script user or script user not set. Running as user ${LOCALUSER}."
			# USERPART=""   # old default: Running as root
			USERPART=" su -c - $LOCALUSER "
			;;
	esac
}

### Funktionen Ende ###############################

