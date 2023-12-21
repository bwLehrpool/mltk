#!/bin/sh

xml="/etc/xdg/xfce4/xfconf/xfce-perchannel-xml/xfce4-power-manager.xml"

[ -s "$xml" ] || exit 0

file="/sys/class/backlight/intel_backlight/max_brightness"

if ! [ -e "$file" ]; then
	for file in /sys/class/backlight/*/max_brightness; do
		break
	done
fi

[ -e "$file" ] || exit 0

min=$( cat "$file" )
min=$(( min / 100 ))
[ "$min" -gt 10 ] || min=10

xmlstarlet ed -u '/channel/property/property[@name="brightness-slider-min-level"]/@value' \
	-v "$min" "$xml" > "$xml.tmp"

mv -f "$xml.tmp" "$xml"
