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

function homedir_warning()
{
	if [ -d "${USERHOMEDIR}" ]; then 
		cat <<-HEREDOC > "${USERHOMEDIR}/WARNUNG_LIES_MICH.txt"
			Achtung:
			Wenn Sie diese Datei sehen bedeutet das, dass Ihr Netzwerkhome nicht 
			eingehängt werden konnte. Hier abgelegte Dateien bleiben daher NICHT 
			erhalten! Speichern Sie Ihre Daten in einem anderen Netzlaufwerk oder 
			einem externen Datenträger a la USB-Stick.
		HEREDOC
		chown "${LOCALUSER}": "${USERHOMEDIR}/WARNUNG_LIES_MICH.txt"
	
		cat <<-THEREDOC > "${USERHOMEDIR}/WARNING_READ_ME.txt"
			Warning:
			If you see this file, it means that your home network drive could 
			not be mounted. Files stored here WILL BE LOST after poweroff! 
			Please save your data to a different network drive or an external 
			storage device such as a USB stick.
		THEREDOC
		chown "${LOCALUSER}": "${USERHOMEDIR}/WARNING_READ_ME.txt"
		
		cat <<-SOMEWHEREDOC > "${USERHOMEDIR}/ATTENTION_LISEZ_MOI.txt"
			Attention!
			Si vous voyez ce fichier, cela signifie que votre dossier personnel 
			n'a pas pu être rattaché. Les fichiers déposés ici ne sont PAS 
			conservés! Enregistrez vos données sur un autre lecteur réseau ou 
			sur un support externe (clé USB ou autre).
		SOMEWHEREDOC
		chown "${LOCALUSER}": "${USERHOMEDIR}/ATTENTION_LISEZ_MOI.txt"
	fi
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
					logger "openslx sharemapper: nfs: could not mount ${SHAREPATH} to ${MOUNTDIR}, waited $x seconds, retrying."
					let x=x+2
					if [ "$x" -gt 4 ]; then
						AUSGANG=1
						logger "openslx sharemapper: nfs: timeout, could not mount ${SHAREPATH} to ${MOUNTDIR} using ${MOUNTOPTS}. mount.nfs installed?"
						break
					fi
					sleep $x
				done
				[ "$AUSGANG" -eq 0 ] 	&& logger "openslx sharemapper: nfs: ${SHAREPATH} mounted to ${MOUNTDIR} (nfs)."
				;;
			cifs)	[ -n "$GLOBALDOMAIN" ] && MOUNTOPTS="${MOUNTOPTS},domain=$GLOBALDOMAIN"
				for VERSION in $CIFSVERSIONS; do
					x=0
					AUSGANG=0
					while ! mount $MOUNTPARAM "${VERSION}","${MOUNTOPTS}" "$SHAREPATH" "$MOUNTDIR" 2>/dev/null 1>&2; do
						logger "openslx sharemapper: cifs: could not mount ${SHAREPATH} to ${MOUNTDIR}, waited $x seconds, retrying."
						let x=x+2
						if [ "$x" -gt 4 ]; then
							AUSGANG=1
							logger "openslx sharemapper: cifs: timeout, could not mount ${SHAREPATH} to ${MOUNTDIR} using ${MOUNTOPTS}, cifs v${VERSION}."
							break
						fi
						sleep $x
					done
					[ "$AUSGANG" -eq 0 ] && break
				done
				
		[ "$AUSGANG" -eq 0 ]    && logger "openslx sharemapper: ${SHAREPATH} mounted to ${MOUNTDIR} using ${MOUNTOPTS}, cifs $VERSION."
				;;
			dav)    while ! echo -e "${USER}\n${PASSWD}" | mount $MOUNTPARAM "$MOUNTOPTS" "$SHAREPATH" "$MOUNTDIR" 2>/dev/null 1>&2; do
					x=2
					AUSGANG=0
					logger "openslx sharemapper: dav: could not mount ${SHAREPATH} to ${MOUNTDIR}, waited another $x seconds, retrying."
					if [ "$x" -gt 4 ]; then
						AUSGANG=1
						logger "openslx sharemapper: dav: timeout, could not mount ${SHAREPATH} to ${MOUNTDIR} using ${MOUNTOPTS}. mount.davfs installed?"
						break
					fi
					sleep $x
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
		if [ -z "$PERSISTENT_MODE" ]; then
			logger "openslx sharemapper: Linking ${SHARENAME}, if possible."

			if [ -n "${SHARENAME}" ]; then
				ln -sn "$USERSHAREDIR" "${USERDESKTOP}"/"$SHARENAME"
				chown "$LOCALUSER" "${USERDESKTOP}"/"$SHARENAME"
				logger "openslx sharemapper: ${SHARENAME} linked."
			elif [ -n "${SHARELETTER}" ]; then
				ln -sn "$USERSHAREDIR" "${USERDESKTOP}"/"$SHARELETTER"
				chown "$LOCALUSER" "${USERDESKTOP}"/"$SHARELETTER"
				logger "openslx sharemapper: No sharename found, share letter ${SHARELETTER} linked."
			fi
		fi
	else
		logger "openslx sharemapper: deleting previously creates share mount dir $USERSHAREDIR."
		rmdir "$USERSHAREDIR" 2>/dev/null
	fi
}

function mount_home()
{
	logger "openslx sharemapper: home share \"$SHARENAME\" found (for PERSISTENT)."
	homedir_warning		# Just to leave a WARNING!!! in homedir if not mounted (over)
	export USER="${GLOBALUSER}"
	export PASSWD="${PW}"
	ERRORMSG=""
	mounter "${MOUNTPARAM}" "${MOUNTOPTS}" "${SHAREPATH}" "${USERHOMEDIR}" 2>/dev/null 1>&2
	ERR=$?
	unset USER
	unset PASSWD
	
	case $ERR in
			1)
				logger "openslx sharemapper: could not mount home share."
				echo "Konnte Home-Laufwerk nicht einhängen!" >> "$USERLOGFILE"
				;;
			2)
				logger "openslx sharemapper: home share seems to be already mounted."
				;;
			0)
				if [ -z "$PERSISTENT_MODE" ]; then
					logger "openslx sharemapper: Linking ${USERDESKTOP}/$(basename $USERHOMEDIR), if possible."
					# Changed: Link on Desktop no longer "Home-verz.", but same
					# name as last part of $USERHOMEDIR (-->PERSISTENT)
					# ln -s "$USERHOMEDIR" "${USERDESKTOP}"/"$SHARENAME"
					ln -sn "$USERHOMEDIR" "${USERDESKTOP}"/$(basename "$USERHOMEDIR")
					chown "$LOCALUSER" "${USERDESKTOP}"/$(basename "$USERHOMEDIR")
					logger "openslx sharemapper: "${USERDESKTOP}"/$(basename "$USERHOMEDIR") linked."
				fi
				;;
			*)
				logger "openslx sharemapper: Unknown error level mounting home share."
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

