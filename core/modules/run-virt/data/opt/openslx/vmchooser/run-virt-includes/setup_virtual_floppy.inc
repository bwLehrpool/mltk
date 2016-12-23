#!/bin/bash
##############################################
# Include: Setup virtual floppy for drive b: #
##############################################

declare -rg FLOPPYIMG="${TMPDIR}/floppy.img"
declare -rg TMPHOME="${HOME}"
declare -rg RUNSCRIPT="${TMPDIR}/runscript.tmp"

wget -T 6 -O "${RUNSCRIPT}" "${SLX_VMCHOOSER_BASE_URL}/lecture/${IMGUUID}/runscript" > /dev/null &
WGET=$!

dd "if=/dev/zero" "of=${FLOPPYIMG}" count=1440 bs=1024
chmod 0600 "${FLOPPYIMG}"
mkfs.fat "${FLOPPYIMG}" || mkfs.vfat "${FLOPPYIMG}" || mkdosfs "${FLOPPYIMG}"

# Create file with resolution information etc.
. "/opt/openslx/inc/shares"
if [ -z "$SHARE_REMAP_MODE" ]; then
	SHARE_REMAP_MODE_INI="0"
	SHARE_REMAP_MODE="3"
else
	SHARE_REMAP_MODE_INI="$SHARE_REMAP_MODE"
fi
[ -z "$SHARE_CREATE_MISSING_REMAP" ] && SHARE_CREATE_MISSING_REMAP="1"
declare -rg RESOLUTION=$(xrandr | grep -o -E 'connected\s*(primary)?\s*[0-9]+x[0-9]+\+0\+0' \
	| grep -o -E -m1 '[0-9]+x[0-9]+')

# Legacy: HOSTRES.TXT
cat > "${TMPDIR}/HOSTRES.TXT" <<-HIER
${RESOLUTION}
HIER

# Create file for network shares to mount
declare -rg SHARES="${TMPDIR}/shares.dat"
touch "${SHARES}"
chmod 0600 "${SHARES}"
if ! pwdaemon --query "${TMPHOME}/.pwsocket" > "${SHARES}"; then
	slxlog "virt-pwdaemon" "Could not start pwdaemon"
else
	sed -i 's/^/192.168.101.1\t/' "${SHARES}" # TODO: Depending on nettype (in case we have != nat some day)
	if [ "${SHARE_REMAP_MODE}" = 1 -o "${SHARE_REMAP_MODE}" = 2 ] && [ -e "${TMPHOME}/.home" ]; then
		NETHOME=$(cat "${TMPHOME}/.home")
		[ -z "$SHARE_HOME_DRIVE" ] && SHARE_HOME_DRIVE="H:"
		# Tab between items, so spaces can be used!
		echo "${NETHOME}	${SHARE_HOME_DRIVE}	Home-Verzeichnis" >> "${SHARES}"
	fi
	for VAR in ${!SHARE_LINE_*}; do
		echo "${!VAR}" >> "${SHARES}"
	done
fi

wait "$WGET"

# Check downloaded runscript, handle extension marker
EXT=
if [ -s "$RUNSCRIPT" ]; then
	EXT=$(head -n 1 "$RUNSCRIPT" | grep -o -i '^EXT=.*$' | cut -d '=' -f 2-)
	[ -n "$EXT" ] && [ "x${EXT:0:1}" != "x." ] && EXT=".$EXT"
fi

# Write info file
UNAME=
[ -s "${HOME}/.account" ] && UNAME=$(cat "${HOME}/.account")
[ -z "${UNAME}" ] && UNAME=$(whoami)
cat > "${TMPDIR}/openslx.ini" <<-HIER
[openslx]
username=${UNAME}
resolution=${RESOLUTION}
createMissingRemap=${SHARE_CREATE_MISSING_REMAP}
remapMode=${SHARE_REMAP_MODE_INI}
homeDrive=${SHARE_HOME_DRIVE}
scriptExt=${EXT}

[remap]
documents=${SHARE_DOCUMENTS}
downloads=${SHARE_DOWNLOADS}
desktop=${SHARE_DESKTOP}
media=${SHARE_MEDIA}
other=${SHARE_OTHER}
HIER

# Copy all them there filez into floppy image
mcopy -i "${FLOPPYIMG}" "${TMPDIR}/openslx.ini" "${TMPDIR}/HOSTRES.TXT" "${SHARES}" "::/"
mcopy -i "${FLOPPYIMG}" "$VMCHOOSER_DIR/data/openslx.exe" "::/"
# Copy guest configuration (with added information) config.xml to be accessed
# via virtual floppy
mcopy -i "${FLOPPYIMG}" "$xmlfile" "::/config.xml"

# Copying linux directory:
mcopy -s -i "${FLOPPYIMG}" "$VMCHOOSER_DIR/data/linux" "::/"

# User supplied runscript
if [ -n "$EXT" ]; then
	sed -i '1d' "${RUNSCRIPT}"
	mcopy -i "${FLOPPYIMG}" "${RUNSCRIPT}" "::/runscript${EXT}"
fi

rm -f -- "${SHARES}" "${TMPDIR}/openslx.ini" "${TMPDIR}/HOSTRES.TXT"
unset SHARES VAR NETHOME UNAME

