##############################################
# Include: Setup virtual floppy for drive b: #
##############################################

# Copy guest configuration (with added information) config.xml to be accessed
# via virtual floppy
cp "$xmlfile" "$VMCHOOSER_DIR/fd-loop/config.xml"

# Add another file with resolution information
xrandr | grep -o -E 'connected\s*(primary)?\s*[0-9]+x[0-9]+\+0\+0' | grep -o -E '[0-9]+x[0-9]+' | head -n 1 > "$VMCHOOSER_DIR/fd-loop/hostres.txt"

# Add our magic openslx binary that sets the correct guest resolution
cp "$VMCHOOSER_DIR/data/openslx.exe" "$VMCHOOSER_DIR/fd-loop/"
cp "$VMCHOOSER_DIR/data/SHARELWS.BAT" "$VMCHOOSER_DIR/fd-loop/"
rm "$VMCHOOSER_DIR/fd-loop/SHARELW.CFG"

for SHARE in $(grep -o -E '^SLX_SHARE_.*_PATH' /opt/openslx/config); do
	eval "echo -n \$${SHARE} | /opt/openslx/bin/tr '/' '\' " >> "$VMCHOOSER_DIR/fd-loop/SHARELW.CFG"
	#eval echo -n " \$${SHARE%PATH}AUTH_PASS /user:\$${SHARE%PATH}AUTH_USER" >> "$VMCHOOSER_DIR/fd-loop/SHARELW.CFG"
	echo -e '\r\n' >> "$VMCHOOSER_DIR/fd-loop/SHARELW.CFG"
done
