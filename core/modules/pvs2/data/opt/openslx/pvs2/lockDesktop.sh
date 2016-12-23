#!/bin/ash

# Problem: While any application (e.g. VMware) is holding the mouse and
# keyboard grab, xscreensaver couldn't grab them, so it will ignore the
# locking request. Without the keyboard grab, all input would still go
# to the vmware window below the black screen, which is, you know, bad,
# since you cannot enter your password to unlock the workstation again.

# So we minimize vmware, lock the screen, and then restore vmware.
# TODO: Add other virtualizers (vbox, kvm) later if needed.
WINDOWS=$(xdotool search --class vmplayer)
for window in $WINDOWS; do
	xdotool windowminimize $window
done
# move mouse pointer to the center of the screen to avoid some problems with ghost clicks
xdotool mousemove --polar 0 0 --sync

# now actually lock
xscreensaver-command --lock

# above lock call is blocking, so now xscreensaver should be active - let's restore vmware
for window in $WINDOWS; do
	xdotool windowmap $window
done

