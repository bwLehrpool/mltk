##############################################
# Include: Setup virtual floppy for drive b: #
##############################################
## Functions ##
setup_virtual_floppy() {
	declare -rg floppy_img="${TMPDIR}/floppy.img"
	declare -r TMPHOME="${HOME}"

	# create floppy disk file + filesystem
	dd "if=/dev/zero" "of=${floppy_img}" count=1440 bs=1024
	chmod 0600 "${floppy_img}"
	mkfs.fat "${floppy_img}" || mkdosfs "${floppy_img}"
	
	if [ -s "${TMPHOME}/.openslx/shares" ]; then
		$(safesource "${TMPHOME}/.openslx/shares")
	elif [ -s "/opt/openslx/inc/shares" ]; then
		$(safesource "/opt/openslx/inc/shares")
	fi

	# Directory for files that go into the virtual floppy
	declare -r FLOPPYDIR="$TMPDIR/floppy"
	mkdir -p "$FLOPPYDIR"
	chmod 0700 "$FLOPPYDIR"

	if notempty SHARE_REMAP_MODE; then
		local SHARE_REMAP_MODE_INI="$SHARE_REMAP_MODE"
	else
		local SHARE_REMAP_MODE_INI="0"
		local SHARE_REMAP_MODE="3"
	fi
	notempty SHARE_CREATE_MISSING_REMAP || local SHARE_CREATE_MISSING_REMAP="1"
	
	# Create file for network shares to mount
	declare -g SHARES="${FLOPPYDIR}/shares.dat"
	touch "${SHARES}"
	chmod 0600 "${SHARES}"
	if ! pwdaemon --query "${TMPHOME}/.pwsocket" > "${SHARES}"; then
		slxlog "run-virt-pwdaemon" "Could not query pwdaemon"
		echo -e '-\t-\t-\t-\t-' > "${SHARES}"
	else
		sed -i 's/^/192.168.101.1\t/' "${SHARES}" # TODO: Depending on nettype (in case we have != nat some day)
		if [ "${SHARE_REMAP_MODE}" = 1 -o "${SHARE_REMAP_MODE}" = 2 ] && [ -s "${TMPHOME}/.openslx/home" ]; then
			NETHOME=$(cat "${TMPHOME}/.openslx/home")
			notempty SHARE_HOME_DRIVE || local SHARE_HOME_DRIVE="H:"
			# Tab between items, so spaces can be used!
			echo "${NETHOME}	${SHARE_HOME_DRIVE}	Home-Verzeichnis" >> "${SHARES}"
		fi
		for VAR in ${!SHARE_LINE_*}; do
			echo "${!VAR}" >> "${SHARES}"
		done
	fi

	declare -r NETSHARES="$CONFDIR/netshares"
	declare -r RUNSCRIPT="$CONFDIR/runscript"
	## Network shares
	# openslx.exe expects network shares line in the following format:
	#		<path> <letter> <shortcut> <username> <password>
	# which is coincidentally the one we received from dmsd :)
	if [ -f "$NETSHARES" ] && [ -s "${NETSHARES}" ]; then
		# try to detect if we are running windows and replace '/' with '\'
		# in an attempt to fix an improperly given samba path
		cmd=(cat)
		if [[ "$( tr 'A-Z' 'a-z' <<< ${VM_OS_TYPE})" = *windows* ]]; then
			cmd=(tr / '\\')
		fi
		# replace the tag for the username, if present; the variable expansion escapes all ':' in $USER
		sed -i "s:%loginuser%:${USER//:/\\:}:g" "${NETSHARES}"
		< "${NETSHARES}"  "${cmd[@]}" >> "${SHARES}"
	fi

	## Runscript
	declare -A OPTIONS
	# Check downloaded runscript and handle options like
	# extension, visibility flag, ...
	# options are 'key=value' pairs separated with ';'
	if [ -f "$RUNSCRIPT" ] && [ -s "$RUNSCRIPT" ]; then
		IFS=$'\n'
		for OPT in $( head -n 1 "$RUNSCRIPT" | tr ";" "\n" ); do
			KEY=$( cut -s -d '=' -f 1 <<< "${OPT^^}" )
			[ -n "$KEY" ] && OPTIONS["$KEY"]="$( cut -s -d '=' -f 2- <<< "$OPT" )"
		done
		unset IFS

		[ -n "${OPTIONS[EXT]}" ] && [ "x${OPTIONS[EXT]:0:1}" != "x." ] && OPTIONS[EXT]=".${OPTIONS[EXT]}"
	fi

	# Default to muted
	local MUTED=1
	# Satellite/location setting can override
	[[ "$SLX_VM_SOUND" = *_ACTIVE ]] && MUTED="0"
	# finally, lecture override
	if [[ "$SLX_VM_SOUND" != FORCE_* ]]; then
		if [ -n "${OPTIONS[SOUNDMUTED]}" ] && [ "${OPTIONS[SOUNDMUTED]}" != -1 ]; then
			MUTED="${OPTIONS[SOUNDMUTED]}"
		fi
	fi

	# Write info file
	local UNAME=
	[ -s "${HOME}/.openslx/account" ] && UNAME=$(cat "${HOME}/.openslx/account")
	notempty UNAME || UNAME=$(whoami)

	# Only warn for non-demo
	if [ "$SHARE_NO_HOME_WARN" = "2" ] && [ "$UNAME" != "demo" ]; then
		SHARE_NO_HOME_WARN=0
	fi

	local resolution=
	# Now that we fixed openslx.exe for VBox, we need the same hack for VMware
	# since using VMwareResolutionSet can freeze the guest. Instead, we additionally
	# show the full-screen toolbar again for > 1 display and let the user hammer
	# the multi-screen button until it works.
	if [ "$PLUGIN_ID" != "vmware" ] || [ "$DISPLAY_COUNT" -eq 1 ]; then
		resolution="${RESOLUTIONS}"
	fi
	# Enable openslx.exe debug output if we're running in debug mode
	local debug=
	$DEBUG && debug=1
	local editmode=0
	if is_on "$VMCHOOSER_ADMIN_MODE"; then
		editmode=1
	fi

	cat > "${FLOPPYDIR}/openslx.ini" <<-EOF
	[openslx]
	username=${UNAME}
	resolution=${resolution%% *}
	resolution2=${resolution}
	createMissingRemap=${SHARE_CREATE_MISSING_REMAP}
	remapMode=${SHARE_REMAP_MODE_INI}
	homeDrive=${SHARE_HOME_DRIVE}
	scriptExt=${OPTIONS[EXT]}
	scriptVisibility=${OPTIONS[VISIBILITY]}
	passCreds=${SHARE_CREDENTIAL_PASSTHROUGH}
	bootIp=${SLX_PXE_CLIENT_IP}
	muteSound=${MUTED}
	noHomeWarn=${SHARE_NO_HOME_WARN}
	runMode=${SLX_RUNMODE_MODULE}
	debug=${debug}
	persistentMode=${editmode}
	
	[remap]
	documents=${SHARE_DOCUMENTS}
	downloads=${SHARE_DOWNLOADS}
	desktop=${SHARE_DESKTOP}
	media=${SHARE_MEDIA}
	other=${SHARE_OTHER}
	EOF

	ln -n -s "${VMCHOOSER_DIR}/data/openslx.exe" "${FLOPPYDIR}/openslx.exe"
	# If we have a kerberos ticket, copy that one too (TODO: Copy keytab too?)
	if [ -n "$KRB5CCNAME" ]; then
		kf=${KRB5CCNAME#FILE:}
		kf=${kf#file:}
		if [ -n "$kf" ] && [ -f "$kf" ]; then
			ln -n -s "$kf" "${FLOPPYDIR}/krb5user.key"
		fi
	fi
	
	# Linux directory
	ln -n -s "$VMCHOOSER_DIR/data/linux" "${FLOPPYDIR}/linux"
	
	# User supplied runscript
	if [ -n "${OPTIONS[EXT]}" ]; then
		sed -i '1d' "${RUNSCRIPT}"
		ln -n -s "${RUNSCRIPT}" "${FLOPPYDIR}/runscript${OPTIONS[EXT]}"
	fi
	
	# Admin supplied runscript(s)
	if [ -d "${CONFDIR}/adminrun" ]; then
		ln -n -s "${CONFDIR}/adminrun" "${FLOPPYDIR}/adminrun"
	fi

	# Hook
	run_hooks "floppy" "$FLOPPYDIR"

	local size="$( du -s -l -L "${FLOPPYDIR}" | awk '{print $1}' )"
	if [ -n "$size" ] && [ "$size" -ge 1400 ]; then
		writelog "WARNING: Floppydir contains $size kb of data, will most likely not fit..."
	fi
	
	if ! mcopy -s -i "${floppy_img}" "${FLOPPYDIR}"/* "::/"; then
		writelog "Error creating floppy image from ${FLOPPYDIR}"
	elif ! $DEBUG; then
		rm -rf -- "${FLOPPYDIR}"
	fi
}

## MAIN ##
# Sanity checks #
# Check if we have any filesystem tool
if ! check_dep --oneof mkfs.fat mkdosfs; then
	# no tools found in minilinux, this means no floppy/openslx.exe will be supported
	slxlog "virt-fstool-missing" "Could not find any of 'mkfs.fat', 'mkdosfs'!"
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
