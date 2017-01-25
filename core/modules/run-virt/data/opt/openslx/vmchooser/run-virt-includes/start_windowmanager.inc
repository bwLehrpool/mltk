####################################################
# Include: Start windowmanager for easier handling #
####################################################
## Functions ##
# Some problems may arise with windows opening in background when
# using eg. vmware without a window manager.
start_wm() {
	# Sanity checks
	unset RUNVIRT_DM
	for DM in openbox kwin xfwm4 metacity blackbox twm fvwm2 ; do
		if type "${DM}" >/dev/null 2>&1 ; then
			declare -rg RUNVIRT_DM="${DM}"
			break
		fi
	done
	if ! isset RUNVIRT_DM; then
		# not fatal, so do not exit but report via slxlog
		slxlog "runvirt-wm" "Could not find any window manager to use!"
		notify_user "Konnte keinen Window Manager finden. (Das ist schlecht!)"
		return 1
	fi

	# start RUNVIRT_DM determined above
	writelog "Starting ${RUNVIRT_DM}."
	if [ "${RUNVIRT_DM}" = "fvwm2" ]; then
		# fvwm2 is not even in the minilinux, so this might be obsolete...
		echo "EdgeScroll 0 0" > ${TMPDIR}/fvwm
		fvwm2 -f ${TMPDIR}/fvwm >/dev/null 2>&1 &
	else
		${RUNVIRT_DM} >/dev/null 2>&1 &
	fi
	local RUNVIRT_DM_PID="$$"
	if ! kill -0 ${RUNVIRT_DM_PID} ; then
		# not fatal, just warn user
		notify_user "$RUNVIRT_VM stoppte unerwartet."
		return 1
	fi
	return 0
}

## MAIN ##
call_post_source start_wm
