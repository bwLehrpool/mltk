#!/bin/ash

wt="PVS2 Manager"
readonly wt

moveVm() {
	# make sure vmplayer is really on workspace 0
	local window windows
	windows=$(xdotool search --onlyvisible --class vmplayer)
	for window in $windows; do
		wmctrl -i -r "$window" -t 0
	done
}

moveMgr() {
	# make sure mgr is on workspace 1
	wmctrl -r "$wt" -t 1
}

getCurrent() {
	wmctrl -d | awk '{if ($2 == "*") print $1}'
}

case "$EVENT" in
init)
	echo "CHECKED=false"
	moveVm
	moveMgr
	wmctrl -s 0
	;;
connected)
	echo "VISIBLE=$ISLOCAL"
	if [ "$ISLOCAL" = "true" ]; then
		moveVm
		moveMgr
		current=$(getCurrent)
		if [ "$current" = "1" ]; then
			echo "CHECKED=true"
		else
			echo "CHECKED=false"
		fi
	fi
	;;
disconnected)
	echo "VISIBLE=false"
	moveMgr
	moveVm
	wmctrl -s 0
	;;
clicked)
	moveMgr
	moveVm
	# Ignore $CHECKED here as reported by PVS, since the user might have switched desktops manually
	current=$(getCurrent)
	if [ "$current" = "1" ]; then
		wmctrl -s 0
		echo "CHECKED=false"
	else
		wmctrl -a "$wt"
		wmctrl -s 1
		echo "CHECKED=true"
	fi
	;;
esac

