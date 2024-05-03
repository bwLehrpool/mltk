#!/bin/ash
# ^ actually sourced

if [ "$PAM_SERVICE" != "su" ] && [ "$PAM_SERVICE" != "sudo" ]; then
	md5=$( printf "%s" "$PAM_USER" | md5sum )
	file="/run/openslx/lightdm/${md5:0:32}"
	if [ -s "$file" ]; then
		getent="$( getent passwd "$PAM_USER" )"
		USER_UID="$( printf "%s" "$getent" | awk -F: '{print $3; exit}' )"
		udir="/run/user/$USER_UID"
		if ! [ -d "$udir" ]; then
			USER_GID="$( printf "%s" "$getent" | awk -F: '{print $4; exit}' )"
			mkdir -p "$udir"
			chmod 0700 "$udir"
			chown "${USER_UID}:${USER_GID}" "$udir"
		fi
		mv -f "$file" "$udir/cow-token"
		chown "$USER_UID" "$udir/cow-token"
	fi
fi
