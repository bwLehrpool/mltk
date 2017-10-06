### Funktionen ####################################

function xor()
{	local RES=($(echo "$1" | sed "s/../0x& /g"))
	shift 1
	while [[ "$1" ]]; do
		local ONE=($(echo "$1" | sed "s/../0x& /g"))
		local COUNT1=${#RES[@]}
		local COUNT2=${#ONE[@]}
		for (( i = 0; i < $COUNT1; i++ )); do
			RES[$i]=$((${ONE[$(( i % COUNT2 ))]:-0} ^ ${RES[$i]:-0}))
		done
		shift 1
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
	already_mounted "$3"
	ERR=$?
	if [ "$ERR" -eq 0 ]; then
		logger "openslx sharemapper: $3 already mounted."
		AUSGANG=2	
	else	
		AUSGANG=0
		x=2
		while ! mount $1 $2 $3 2>/dev/null 1>&2; do
			logger "openslx sharemapper: could not mount ${2} to ${3}, waited another $x seconds, retrying."
			sleep $x
			if [ "$x" -gt 6 ]; then
				AUSGANG=1
				logger "openslx sharemapper: timeout, could not mount ${2} to ${3}."
				break
			fi
			let x=x+2
		done
		[ "$AUSGANG" -eq 0 ] 	&& logger "openslx sharemapper: ${2} mounted to ${3}."	# Todo: Schöner schreiben:)
	fi
	return $AUSGANG
}

### Funktionen Ende ###############################

