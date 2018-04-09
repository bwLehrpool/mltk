function runscript()
{
	logger "openslx sharemapper: runscript: looking for a script to run..."
	if [ -f "$DISKMOUNTDIR/openslx.ini" ]; then
		EXTENSION=$(grep scriptExt "$DISKMOUNTDIR/openslx.ini" | cut -f 2 -d "=")
		[ -z "$EXTENSION" -o "$EXTENSION" = "bat" ] && \
			logger "openslx sharemapper: runscript: Script extension empty or \'bat\', trying anyway."
		if [ -f "$DISKMOUNTDIR/runscript${EXTENSION}" ]; then
			su -c "$DISKMOUNTDIR/runscript${EXTENSION}" "$LOCALUSER"
			ERR=$?
			if [ $ERR -eq 0 ]; then
				logger "openslx sharemapper: runscript: runscript${EXTENSION} ok, errorlevel ${ERR}."
			else
				logger "openslx sharemapper: runscript: Script error ${ERR}."
			fi
		fi
	else
		logger "openslx sharemapper: runscript: Could not find $DISKMOUNTDIR/openslx.ini for script extension."
	fi
}
