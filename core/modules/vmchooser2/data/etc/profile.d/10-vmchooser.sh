#!/bin/ash

TEMP_HOME_DIR="$HOME"
PERSISTENT_HOME_DIR="$HOME/PERSISTENT"

if [ -d "$PERSISTENT_HOME_DIR" ]; then

	rm -f -- "$TEMP_HOME_DIR/.config/openslx"
	mkdir -p "$PERSISTENT_HOME_DIR/.config/openslx" "$TEMP_HOME_DIR/.config"
	ln -nfs "$PERSISTENT_HOME_DIR/.config/openslx" "$TEMP_HOME_DIR/.config/openslx"

fi

