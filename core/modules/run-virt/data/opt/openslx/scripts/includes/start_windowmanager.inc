####################################################
# Include: Start windowmanager for easier handling #
####################################################

# Some problems may arise with windows opening in background when
# using eg. vmware without a window manager.

for dm in openbox kwin xfwm4 metacity blackbox twm fvwm2 ; do
	if which $dm >/dev/null 2>&1 ; then
		if [ "$dm" = "fvwm2" ] ; then
			echo "EdgeScroll 0 0" > ${redodir}/fvwm
			writelog "Starting fvwm2."
			fvwm2 -f ${redodir}/fvwm >/dev/null 2>&1 &
		else
			writelog "Starting ${dm}."
			$dm >/dev/null 2>&1 &
		fi
		break
	fi
done
