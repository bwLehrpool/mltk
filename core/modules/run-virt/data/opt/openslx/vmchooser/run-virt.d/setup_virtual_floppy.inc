##############################################
# Include: Setup virtual floppy for drive b: #
##############################################
## Functions ##
setup_virtual_floppy() {
	declare -rg SLX_FLOPPY_IMG="${TMPDIR}/floppy.img"
	declare -rg TMPHOME="${HOME}"

	# create floppy disk file + filesystem
	dd "if=/dev/zero" "of=${SLX_FLOPPY_IMG}" count=1440 bs=1024
	chmod 0600 "${SLX_FLOPPY_IMG}"
	mkfs.fat "${SLX_FLOPPY_IMG}" || mkfs.vfat "${SLX_FLOPPY_IMG}" || mkdosfs "${SLX_FLOPPY_IMG}"
	
	if [ -s "${TMPHOME}/.openslx/shares" ]; then
		$(safesource "${TMPHOME}/.openslx/shares")
	else
		$(safesource "/opt/openslx/inc/shares")
	fi

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
		echo "- - - - - - -" > "${SHARES}"
	else
		sed -i 's/^/192.168.101.1\t/' "${SHARES}" # TODO: Depending on nettype (in case we have != nat some day)
		if [ "${SHARE_REMAP_MODE}" = 1 -o "${SHARE_REMAP_MODE}" = 2 ] && [ -e "${TMPHOME}/.openslx/home" ]; then
			NETHOME=$(cat "${TMPHOME}/.openslx/home")
			notempty SHARE_HOME_DRIVE || local SHARE_HOME_DRIVE="H:"
			# Tab between items, so spaces can be used!
			echo "${NETHOME}	${SHARE_HOME_DRIVE}	Home-Verzeichnis" >> "${SHARES}"
		fi
		for VAR in ${!SHARE_LINE_*}; do
			echo "${!VAR}" >> "${SHARES}"
		done
	fi

	local NETSHARES="$CONFDIR/netshares"
	local RUNSCRIPT="$CONFDIR/runscript"

	## Network shares
	# openslx.exe expects network shares line in the following format:
	#		<path> <letter> <shortcut> <username> <password>
	# which is coincidentally the one we received from dmsd :)
	if [ -f "$NETSHARES" ] || download_file "$url_lecture_netshares" "$NETSHARES"; then
		if [ -s "${NETSHARES}" ]; then
			# try to detect if we are running windows and replace '/' with '\'
			# in an attempt to fix an improperly given samba path
			cmd=(cat)
			if [[ "$( tr '[A-Z]' '[a-z]' <<< ${VM_OS_TYPE})" = *windows* ]]; then
				cmd=(tr / '\\')
			fi
			# replace the tag for the username, if present; the variable expansion escapes all ':' in $USER
			sed -i "s:%loginuser%:${USER//:/\\:}:g" "${NETSHARES}"
			< "${NETSHARES}"  "${cmd[@]}" >> "${SHARES}"
		fi
	fi

	## Runscript
	declare -A OPTIONS
	# Check downloaded runscript and handle options like
	# extension, visibility flag, ...
	# options are 'key=value' pairs separated with ';'
	if [ -f "$RUNSCRIPT" ] || download_file "$url_lecture_runscript" "$RUNSCRIPT"; then
		if [ -s "$RUNSCRIPT" ]; then
			IFS=$'\n'
			for OPT in $(head -n 1 "$RUNSCRIPT" | tr ";" "\n"); do
				KEY=$(cut -s -d '=' -f 1 <<< "${OPT^^}")
				[ -n "$KEY" ] && OPTIONS["$KEY"]=$(cut -s -d '=' -f 2- <<< "$OPT")
			done
			unset IFS

			[ ${OPTIONS[EXT]} ] && [ "x${OPTIONS[EXT]:0:1}" != "x." ] && OPTIONS[EXT]=".${OPTIONS[EXT]}"
		fi
	fi

	# Default to muted
	local MUTED=1
	# Satellite/location setting can override
	[ -n "$SLX_VM_MUTED" ] && [ "$SLX_VM_MUTED" -ge 0 ] && MUTED="$SLX_VM_MUTED"
	# finally, lecture can override
	if [ -n "${OPTIONS[SOUNDMUTED]}" ] && [ "${OPTIONS[SOUNDMUTED]}" != -1 ]; then
		MUTED="${OPTIONS[SOUNDMUTED]}"
	fi

	# Write info file
	local UNAME=
	[ -s "${HOME}/.openslx/account" ] && UNAME=$(cat "${HOME}/.openslx/account")
	notempty UNAME || UNAME=$(whoami)
	cat > "${TMPDIR}/openslx.ini" <<-EOF
	[openslx]
	username=${UNAME}
	resolution=${RESOLUTION}
	createMissingRemap=${SHARE_CREATE_MISSING_REMAP}
	remapMode=${SHARE_REMAP_MODE_INI}
	homeDrive=${SHARE_HOME_DRIVE}
	scriptExt=${OPTIONS[EXT]}
	scriptVisibility=${OPTIONS[VISIBILITY]}
	passCreds=${SHARE_CREDENTIAL_PASSTHROUGH}
	bootIp=${SLX_PXE_CLIENT_IP}
	muteSound=${MUTED}
	
	[remap]
	documents=${SHARE_DOCUMENTS}
	downloads=${SHARE_DOWNLOADS}
	desktop=${SHARE_DESKTOP}
	media=${SHARE_MEDIA}
	other=${SHARE_OTHER}
	EOF
	
	# Copy all them there filez into floppy image
	mcopy -i "${SLX_FLOPPY_IMG}" "${TMPDIR}/openslx.ini" "${TMPDIR}/HOSTRES.TXT" "${SHARES}" "${VMCHOOSER_DIR}/data/openslx.exe" "::/"
	# If we have a kerberos ticket, copy that one too (TODO: Copy keytab too?)
	if [ -n "$KRB5CCNAME" ]; then
		kf=${KRB5CCNAME#FILE:}
		kf=${kf#file:}
		if [ -n "$kf" ] && [ -f "$kf" ]; then
			mcopy -i "${SLX_FLOPPY_IMG}" "$kf" "::/krb5user.key"
		fi
	fi
	# Copy guest configuration (with added information) config.xml to be accessed
	# via virtual floppy
	mcopy -i "${SLX_FLOPPY_IMG}" "$XML_FILE" "::/config.xml"
	
	# Copying linux directory:
	mcopy -s -i "${SLX_FLOPPY_IMG}" "$VMCHOOSER_DIR/data/linux" "::/"
	
	# User supplied runscript
	if [ -n "${OPTIONS[EXT]}" ]; then
		sed -i '1d' "${RUNSCRIPT}"
		mcopy -i "${SLX_FLOPPY_IMG}" "${RUNSCRIPT}" "::/runscript${OPTIONS[EXT]}"
	fi
	
	# Admin supplied runscript(s)
	if [ -d "${CONFDIR}/adminrun" ]; then
		mcopy -s -i "${SLX_FLOPPY_IMG}" "${CONFDIR}/adminrun" "::/"
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
