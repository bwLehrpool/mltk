#!/bin/ash

[ "$EVENT" = "clicked" ] || exit 0

exec beamergui -w
