##############################################
# Include: Setup virtual floppy for drive b: #
##############################################
## Functions ##
# download the given resource for this lecture
# and makes it accessible globally through a
# variable of the same name. Given name will be
# put to lower case before wget'ing so its safe
# to use upper case variables...
load_dmsd_resource() {
	[ $# -eq 0 ] && writelog "Missing argument for resource." && return
	while [ $# -gt 0 ]; do
		local RESOURCE="$1"
		declare -g ${RESOURCE}="${TMPDIR}/${RESOURCE}"
		local RESOURCE_URL="${SLX_VMCHOOSER_BASE_URL}/lecture/${IMGUUID}/$(echo $1 | awk '{print tolower($0)}')"
		shift
		echo "DOWNLOAD to ${!RESOURCE}"
		if ! wget -T 6 -O "${!RESOURCE}" "${RESOURCE_URL}" 2> /dev/null >&2; then
			writelog "Downloading runscript from '${RESOURCE_URL}' failed."
			continue
		fi
		if [ ! -s "${!RESOURCE}" ]; then
			# zero bytes, log and ignore
			writelog "Downloaded resource from '${RESOURCE_URL}' has zero bytes."
			unset "${RESOURCE}"
		else
			# there is stuff in it, make it readonly
			readonly "${RESOURCE}"
		fi
	done
	return 0
}
setup_virtual_floppy() {
	declare -rg SLX_FLOPPY_IMG="${TMPDIR}/floppy.img"
	declare -rg TMPHOME="${HOME}"

	# create floppy disk file + filesystem
	dd "if=/dev/zero" "of=${SLX_FLOPPY_IMG}" count=1440 bs=1024
	chmod 0600 "${SLX_FLOPPY_IMG}"
	mkfs.fat "${SLX_FLOPPY_IMG}" || mkfs.vfat "${SLX_FLOPPY_IMG}" || mkdosfs "${SLX_FLOPPY_IMG}"
	
	# Create file with resolution information etc.
	$(safesource "/opt/openslx/inc/shares")

	if notempty SHARE_REMAP_MODE; then
		local SHARE_REMAP_MODE_INI="$SHARE_REMAP_MODE"
	else
		local SHARE_REMAP_MODE_INI="0"
		local SHARE_REMAP_MODE="3"
	fi
	notempty SHARE_CREATE_MISSING_REMAP || local SHARE_CREATE_MISSING_REMAP="1"
	
	# Legacy: openslx.exe expects HOSTRES.TXT
	local RESOLUTION=$(xrandr | grep -o -E 'connected\s*(primary)?\s*[0-9]+x[0-9]+\+0\+0' \
		| grep -o -E -m1 '[0-9]+x[0-9]+')
	# TODO fallback if this fails? this is currently never checked...
	cat > "${TMPDIR}/HOSTRES.TXT" <<-EOF
	${RESOLUTION}
	EOF
	
	# Create file for network shares to mount
	declare -g SHARES="${TMPDIR}/shares.dat"
	touch "${SHARES}"
	chmod 0600 "${SHARES}"
	if ! pwdaemon --query "${TMPHOME}/.pwsocket" > "${SHARES}"; then
		slxlog "virt-pwdaemon" "Could not start pwdaemon"
	else
		sed -i 's/^/192.168.101.1\t/' "${SHARES}" # TODO: Depending on nettype (in case we have != nat some day)
		if [ "${SHARE_REMAP_MODE}" = 1 -o "${SHARE_REMAP_MODE}" = 2 ] && [ -e "${TMPHOME}/.home" ]; then
			NETHOME=$(cat "${TMPHOME}/.home")
			notempty SHARE_HOME_DRIVE || local SHARE_HOME_DRIVE="H:"
			# Tab between items, so spaces can be used!
			echo "${NETHOME}	${SHARE_HOME_DRIVE}	Home-Verzeichnis" >> "${SHARES}"
		fi
		for VAR in ${!SHARE_LINE_*}; do
			echo "${!VAR}" >> "${SHARES}"
		done
	fi

	## Network shares
	# openslx.exe expects network shares line in the following format:
	#		<path> <letter> <shortcut> <username> <password>
	# which is coincidentally the one we received from dmsd :)
	if load_dmsd_resource NETSHARES; then
		if [ -s "${NETSHARES}" ]; then
			# try to detect if we are running windows and replace '/' with '\'
			# in an attempt to fix an improperly given samba path
			cmd=(cat)
			if [[ "$( tr '[A-Z]' '[a-z]' <<< ${VM_OS_TYPE})" = *windows* ]]; then
				cmd=(tr / '\\')
			fi
			< $NETSHARES "${cmd[@]}" >> "${SHARES}"
			# replace the tag for the username, if present
			sed -i 's/%loginuser%/'"${USER}"'/g' "${SHARES}"
		fi
	fi

	## Runscript
	# Check downloaded runscript, handle extension marker
	if load_dmsd_resource RUNSCRIPT; then
		local EXT=
		if [ -s "$RUNSCRIPT" ]; then
			EXT=$(head -n 1 "$RUNSCRIPT" | grep -o -i '^EXT=.*$' | cut -d '=' -f 2-)
			[ -n "$EXT" ] && [ "x${EXT:0:1}" != "x." ] && EXT=".$EXT"
		fi
	fi
	
	# Write info file
	local UNAME=
	[ -s "${HOME}/.account" ] && UNAME=$(cat "${HOME}/.account")
	notempty UNAME || UNAME=$(whoami)
	cat > "${TMPDIR}/openslx.ini" <<-EOF
	[openslx]
	username=${UNAME}
	resolution=${RESOLUTION}
	createMissingRemap=${SHARE_CREATE_MISSING_REMAP}
	remapMode=${SHARE_REMAP_MODE_INI}
	homeDrive=${SHARE_HOME_DRIVE}
	megaDrive=SEGA
	scriptExt=${EXT}
	passCreds=${SHARE_CREDENTIAL_PASSTHROUGH}
	bootIp=${SLX_PXE_CLIENT_IP}
	
	[remap]
	documents=${SHARE_DOCUMENTS}
	downloads=${SHARE_DOWNLOADS}
	desktop=${SHARE_DESKTOP}
	media=${SHARE_MEDIA}
	other=${SHARE_OTHER}
	EOF
	
	# Copy all them there filez into floppy image
	mcopy -i "${SLX_FLOPPY_IMG}" "${TMPDIR}/openslx.ini" "${TMPDIR}/HOSTRES.TXT" "${SHARES}" "::/"
	mcopy -i "${SLX_FLOPPY_IMG}" "$VMCHOOSER_DIR/data/openslx.exe" "::/"
	# Copy guest configuration (with added information) config.xml to be accessed
	# via virtual floppy
	mcopy -i "${SLX_FLOPPY_IMG}" "$XML_FILE" "::/config.xml"
	
	# Copying linux directory:
	mcopy -s -i "${SLX_FLOPPY_IMG}" "$VMCHOOSER_DIR/data/linux" "::/"
	
	# User supplied runscript
	if [ -n "$EXT" ]; then
		sed -i '1d' "${RUNSCRIPT}"
		mcopy -i "${SLX_FLOPPY_IMG}" "${RUNSCRIPT}" "::/runscript${EXT}"
	fi
	
	# Admin supplied runscript(s)
	if [ -d "${VMCHOOSER_DIR}/adminrun" ]; then
		mcopy -s -i "${SLX_FLOPPY_IMG}" "${VMCHOOSER_DIR}/adminrun" "::/"
	fi
	
	rm -f -- "${SHARES}" "${TMPDIR}/openslx.ini" "${TMPDIR}/HOSTRES.TXT"
}

## MAIN ##
# Sanity checks #
# Check if we have any filesystem tool
if ! check_dep --oneof mkfs.fat mkfs.vfat mkdosfs; then
	# no tools found in minilinux, this means no floppy/openslx.exe will be supported
	slxlog "virt-fstool-missing" "Could not find any of 'mkfs.fat', 'mkfs.vfat', 'mkdosfs'!"
	writelog "Could not find any filesystem-related tools to create the floppy's filesystem with!"
	error_user "Konnte kein virtuelles Floppy anlegen! Setzen der Auflösung oder Einhängen von Netzwerkfreigaben werden nicht funktionieren. Bitte kontaktieren Sie den Support."
	# still continue as this is not really critical
fi

# separate check for other deps
if ! check_dep dd chmod pwdaemon sed head grep cut whoami mcopy; then
	writelog "Missing core tools to setup the virtual floppy!"
else
	reg_feature_handler "slxfloppy" "setup_virtual_floppy"
fi
