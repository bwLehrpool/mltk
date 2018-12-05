#!/bin/ash

[ "$EVENT" = "clicked" ] || exit 0

if ps x | grep -v grep | grep -q 'beamergui -b'; then
	exec beamergui -w
fi

(
	beamergui -b -g &
) &

