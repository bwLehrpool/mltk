# vim:ft=sh

spawn_process()
{
	if [ "$USERCREDS" -eq 1 ]; then
		get_creds			
		USERNAME="$LOGINUSER" PASSWORD="$PW" runscript_visibility "$SCRIPT" "$USERPART" "$VISIBILITY" &
		unset PASSWORD
		unset PW
	else
		runscript_visibility "$SCRIPT" "$USERPART" "$VISIBILITY" &
	fi
	
	local WAITPID="$!"
	wait $WAITPID
	local ERR=$?
	logger "openslx utility: adminrun: script ${SCRIPT} (PID ${WAITPID}) finished, errorlevel ${ERR}."
}

function adminrun()
{
	logger "openslx utility: adminrun: started."
	
	if [ ! -d "$ADMINRUNDIR" ]; then
		logger "openslx utility: adminrun: no adminrun directory found."
		return
	fi

	logger "openslx utility: adminrun: adminrun directory found, starting scripts."
	set_userpart
	for SCRIPT in "$ADMINRUNDIR"/*; do 
		USERPART=" su -c - $LOCALUSER "	# adminrun: not as user
		# USERPART=" "
		VISIBILITY=$(echo ${SCRIPT%%.*}|cut -f 2 -d "-")	# visibility: 0 versteckt, 1 normal, 2 minimiert
		USERCREDS=$(echo ${SCRIPT%%.*}|cut -f 3 -d "-")		# user/pass: 0 nein, 1 ja
		logger "openslx utility: adminrun: starting script ${SCRIPT} using visibility ${VISIBILITY} and usercreds ${USERCREDS}."
		spawn_process &
	done



}
