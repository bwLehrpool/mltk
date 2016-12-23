### Funktionen ####################################

function xor()
{	local RES=($(echo "$1" | sed "s/../0x& /g"))
	shift 1
	while [[ "$1" ]]; do
		local ONE=($(echo "$1" | sed "s/../0x& /g"))
		local COUNT1=${#RES[@]}
		if [ $COUNT1 -lt ${#ONE[@]} ]; then
			COUNT1=${#ONE[@]}
		fi
		for (( i = 0; i < $COUNT1; i++ )); do
			RES[$i]=$((${ONE[$i]:-0} ^ ${RES[$i]:-0}))
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
		mount $1 $2 $3 2>/dev/null 1>&2
		ERR=$?
		if [ "$ERR" -gt 0 ]; then
			logger "openslx sharemapper: could not mount ${2} to ${3}."
			AUSGANG=1
		else
			logger "openslx sharemapper: ${2} mounted to ${3}."
			AUSGANG=0	
		fi
	fi
	return $AUSGANG
}

### Funktionen Ende ###############################

