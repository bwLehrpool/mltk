#######################################################
# Include: Starting PVS (Pool Video switch) if wanted #
#######################################################

# if [ -e /etc/xdg/autostart/pvsgui.desktop ]; then
# 	/usr/local/bin/pvsgui -p 2 -b >/dev/null 2>&1 &
# fi

if [[ "$DO_PROFILE" -ge "1" ]]; then
	wget "http://132.230.8.113/profile/do.php?action=start" &
fi
