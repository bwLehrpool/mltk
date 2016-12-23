function get_shares()
{
	i=0
	while read -r LINIE; do
		ROHSHARES[i]="$LINIE"
		((i++))
	done < "$CONFIGFILE"
	
#	echo "${#ROHSHARES[@]} eingelesen:"
#	for (( i = 0; i < ${#ROHSHARES[@]}; i++ )); do
#		echo -n "$i "
#		echo "${ROHSHARES[i]}"
#	done
	
	let SHAREZAHL=${#ROHSHARES[@]}-1
	if [ "$SHAREZAHL" -lt 1 ]; then
		logger "openslx sharemapper: No shares to map/mount found in config file $CONFIGFILE."
		exit 0
	else
		logger "openslx sharemapper: $SHAREZAHL share(s) found."
	fi
}
