#!/bin/sh

xmessage -buttons "Abbrechen:0,Sitzung beenden:7" "Diese Sitzung sofort beenden?
Nicht gespeicherte Daten gehen verloren."
[ $? = 7 ] || exit 1

loginctl terminate-session $XDG_SESSION_ID
sleep 2
loginctl kill-session $XDG_SESSION_ID
sleep 2
loginctl terminate-seat $XDG_SEAT
exit 0
