get_xauthfile()
{
	# Das ist mitunter ein Problem, da die richtige xauth zu finden - Distroabhängig!
	for wartezeit in 1 2 3 5 10 10 0 ende; do
		[ "$wartezeit" = "ende" ] && { logger "openslx resolution utility: XAUTHFILE not found. Exiting."; exit 1; }
		# XAUTHORITY=$(ps aux | grep -o "root.*X.* \-auth *[^ ]*" | awk '{print $NF}')
		XAUTHORITY=$(ps aux | grep -o ".*/X.* \-auth *[^ ]*" | head -n 1 | awk '{print $NF}')
		[ -n "${XAUTHORITY}" ] && break
		sleep $wartezeit
		done
	logger "openslx resolution utility: XAUTHFILE found."
}


check_resolution() {
	ACTRES=$(DISPLAY="${DISPLAY}" XAUTHORITY="${XAUTHORITY}" xrandr|grep '*'|tr -s " "|cut -f 2 -d " ")
	if [ "$ACTRES" != "$RESOLUTION" ]; then
		logger "openslx resolution utility: resolution changed; re-changing."
		DISPLAY="${DISPLAY}" XAUTHORITY="${XAUTHORITY}" xrandr --output ${AUSGABE} --mode ${RESOLUTION}
	else
		logger "openslx resolution utility: resolution unchanged."
	fi
}

set_resolution()
{
	DISPLAY="${1}.0" 
	logger "openslx resolution utility started."
	
	# Da nun fremdgemountet wird, hier sicherheitshalber zur Kontrolle wg. Gross-/Kleinschreibung:
	SLXCONFIGFILE=$(find "${DISKMOUNTDIR}" -iwholename "${SLXCONFIGFILE}")
	
	if [ -f "$SLXCONFIGFILE" ]; then
		RESOLUTION=$(grep 'resolution=' "$SLXCONFIGFILE" | cut -d '=' -f 2)
		if [ -z $RESOLUTION ]; then
			logger "openslx resolution utility: resolution file $SLXCONFIGFILE seems to be empty!"
			exit 1
		fi
		logger "openslx resolution utility: resolution file $SLXCONFIGFILE yields ${RESOLUTION}."
	else
		logger "openslx resolution utility: resolution file $SLXCONFIGFILE not found!"
		exit 1
	fi
	
	# Das ist mitunter ein Problem, da die richtige zu finden - Distroabhängig!
	# Hier ein Beispiel für openSuse 13.2, sollte auch für Debian, Ubuntu, Redhat usw. laufen:.
	# XAUTHDATEI=$(ps aux | grep -o "root.*X.* \-auth *[^ ]*" | awk '{print $NF}')
	get_xauthfile

	# Zu verbessern: Der Name des Verbundenen ist nicht immer bekannt. Daher nehmen wir das
	# erste 'connected' in der Ausgabe xrandrs Und warten evtl. ein Weilchen)::
	for wartezeit in 1 1 1 2 2 2 4 4 8 8 0 ende; do
		[ "$wartezeit" = "ende" ] && { logger "openslx resolution utility: Could not detect output device. Exiting." ; exit 1; }
		AUSGABE=$(DISPLAY="${DISPLAY}" XAUTHORITY="${XAUTHORITY}" xrandr -q|grep -m 1 " connected "|awk '{print $1}')
		[ -n "$AUSGABE" ] && break
		sleep $wartezeit
	done
	
	# Pruefen, ob xrand eine passende modeline ausgibt:
	if [ "$(DISPLAY=${DISPLAY} XAUTHORITY=${XAUTHORITY} xrandr | grep -c ${RESOLUTION}[[:space:]])" -eq 0 ]; then
		logger "openslx resolution utility: xrandr yields no fitting modeline; adding one."
		MODELINE=$(cvt ${RESOLUTION//x/ } | grep -v "^#" | sed "s/Modeline //g" | sed 's/"//g')
		DISPLAY="${DISPLAY}" XAUTHORITY="${XAUTHORITY}" xrandr --newmode $(echo ${MODELINE})
		DISPLAY="${DISPLAY}" XAUTHORITY="${XAUTHORITY}" xrandr --addmode ${AUSGABE} $(echo ${MODELINE} | cut -d " " -f 1)
		# Dann einzusteuernde Auflösung natürlich auf die neue ändern:
		RESOLUTION=$(echo ${MODELINE} | cut -d " " -f 1)
		logger "openslx resolution utility: (xrandr) modeline ${MODELINE} added (${RESOLUTION})."
	fi
	
	# Auflösung per xrandr setzen:
	DISPLAY="${DISPLAY}" XAUTHORITY="${XAUTHORITY}" xrandr --output ${AUSGABE} --mode ${RESOLUTION}
	ERR=$?
	if [ $ERR -ne 0 ]; then
		logger "openslx resolution utility: xrandr error code ${ERR}."
	else
		logger "openslx resolution utility: xrandr ok. Mode $RESOLUTION set."
	fi
	
	# Das hier ist ein wenig problematisch, da nach X-Start die gewünschte Desktopumgebung
	# diese gern zuvor gespeicherte Auflösungen wieder einspielt. Daher warten wir einfach,
	# und prüfen in gewissen Zeitabständen, ob sich die Auflösung geändert hat, und spielen
	# im Änderungsfalle die Grundsystemauflösung wieder ein. Schön ist das nicht.
	
	# Now, let's wait some seconds to see if another process re-changes res.:
	sleep 5
	check_resolution
	
	# Now, let's wait some more seconds to see if another process re-changes res.:
	sleep 15
	check_resolution
	
	# Now, let's wait even more seconds to see if another process re-changes res.:
	sleep 20
	check_resolution
	
	# Now, let's wait even more seconds to see if another process re-changes res.:
	sleep 40
	check_resolution
	
	exit 0
}
