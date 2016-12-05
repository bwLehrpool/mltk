#
#	Common functions to copy kernel related files
#
############################################################
#
#
# copies kernel modules as given in the module config file
# * depends on 'depmod'
# * requires REQUIRED_KERNEL_MODULES to be set.
#   (entries must be a relative path to /lib/modules/<SYSTEM_KERNEL_LONG>)
#
# ex: for 		/lib/modules/3.2.0/kernel/fs/nfs/nfs.ko
#     must be given as  kernel/fs/nfs/nfs.ko
#

# set global KERNEL_BASE_DIR as in the directory containing lib/modules and lib/firmware
# for system kernel, that is "/" and for an openslx kernel KERNEL_BUILD_DIR
KERNEL_BASE_DIR=""

get_kernel_version () {
	pinfo "Running fallback get_kernel_version()"
	# Running system kernel version
	if [ -z "$SYSTEM_KERNEL_LONG" ]; then
		declare -rg SYSTEM_KERNEL_LONG=$(uname -r)
		declare -rg SYSTEM_KERNEL_SHORT=$(grep -o -E '^[0-9\.]+' <<<$SYSTEM_KERNEL_LONG)
	fi
}

check_kernel_base_dir () {

	# check if KERNEL_BASE_DIR was set, if not we don't know
	# whether kernel-openslx or kernel-system has been built
	# and therefore not were to look for kernel modules,
	# firmware and the kernel itself

	if [ -z "${KERNEL_BASE_DIR}" ]; then
		# this is bad, abort
		perror "KERNEL_BASE_DIR is not set. The kernel module did not run properly"
	else
		pinfo "KERNEL_BASE_DIR is '$KERNEL_BASE_DIR'"
	fi

}

copy_kernel_modules () {
	pinfo "Copying kernel modules for kernel ${TARGET_KERNEL_LONG}..."
	[ -z "${REQUIRED_KERNEL_MODULES}" ] && perror "REQUIRED_KERNEL_MODULES is empty. Check your config file."
	[ -z "${KERNEL_HEADERS_DIR}" ] && perror "KERNEL_HEADERS_DIR is empty. Kernel headers appears to be missing."


	check_kernel_base_dir

	local OLD_DIR="$(pwd)"

	#
	# process modules list
	#
	# search for modules in KERNEL_BASE_DIR
	cd "${KERNEL_BASE_DIR}" || perror "Could not cd to ${KERNEL_BASE_DIR}"

	local KERNEL_MODULES_DIR="lib/modules/${TARGET_KERNEL_LONG}"
	local KERNEL_MODULES_LIST=""
	local REQUIRED_KERNEL_MODULES_EXPANDED=""
	local KERNEL_MODULE=""
	local KERNEL_MODULE_PATH=""
	local ELEM=""

	# Do some fancy stuff to allow wildcards etc. in required kernel modules.
	cd "${KERNEL_MODULES_DIR}"
	for KERNEL_MODULE in ${REQUIRED_KERNEL_MODULES}; do
		for ELEM in $KERNEL_MODULE; do
			echo $ELEM | grep '\*' && pwarning "Could not expand '$ELEM'." && continue
			REQUIRED_KERNEL_MODULES_EXPANDED+=" $ELEM"
		done
	done
	cd - 2>/dev/null
	pinfo "Expanded the list of $(echo "$REQUIRED_KERNEL_MODULES" | wc -w) required kernel modules to $(echo "$REQUIRED_KERNEL_MODULES_EXPANDED" | wc -w)"

	#
	# now loop over given modules and locate them
	#
	for KERNEL_MODULE in ${REQUIRED_KERNEL_MODULES_EXPANDED}; do
		local KERNEL_MODULE_PATH="${KERNEL_MODULES_DIR}/${KERNEL_MODULE}"
		if grep "^${KERNEL_MODULE}$" "${KERNEL_BASE_DIR}/${KERNEL_MODULES_DIR}/modules.builtin" >/dev/null; then
			pdebug "Already built-in ${KERNEL_MODULE}."
		elif [ -e "${KERNEL_MODULE_PATH}" ]; then
			pdebug "Copying '${KERNEL_MODULE_PATH}'"
			KERNEL_MODULES_LIST+=" ${KERNEL_MODULE_PATH}"
		else
			pwarning "Module ${KERNEL_MODULE} not found. Skipping. (might cause problems on certain clients!)"
			continue
		fi

		# check for dependencies
		local DEPS=$(grep "${KERNEL_MODULE}:" "${KERNEL_BASE_DIR}/${KERNEL_MODULES_DIR}/modules.dep" | cut -d ":" -f2-)
		if [ ! -z "$DEPS" ]; then
			for DEP in $DEPS; do
				pdebug "Adding dep: ${KERNEL_MODULES_DIR}/$DEP"
				KERNEL_MODULES_LIST+=" ${KERNEL_MODULES_DIR}/$DEP"
			done
		else
			pdebug "${KERNEL_MODULE} has no dependencies."
		fi
	done

	if [ ! -z "${KERNEL_MODULES_LIST}" ]; then
		local COUNT=$(echo "${KERNEL_MODULES_LIST}" | wc -w)
		pinfo "Copying $COUNT modules to target directory."
		tarcopy "${KERNEL_MODULES_LIST}" "${TARGET_BUILD_DIR}"
	fi

	#
	# generate modules map files
	#
	# first strip modules.order of all the modules we don't use
	cat "${KERNEL_MODULES_DIR}/modules.order" | grep -E "$(echo ${REQUIRED_KERNEL_MODULES} | tr '\ ' '|' | tr '_' '.' | tr '-' '.')" \
		>> "${TARGET_BUILD_DIR}/${KERNEL_MODULES_DIR}/modules.order"
	# copy list of builtin kernel modules
	cp "${KERNEL_MODULES_DIR}/modules.builtin" "${TARGET_BUILD_DIR}/${KERNEL_MODULES_DIR}"
	# with modules.order and modules.builtin, we can run depmod for the rest of the files
	depmod -b "${TARGET_BUILD_DIR}" -a "${TARGET_KERNEL_LONG}"

	# go back to wherever we were
	cd "${OLD_DIR}" || perror "Could not cd back to ${OLD_DIR}."
}

copy_firmware () {

	pinfo "Copying firmware for kernel ${SYSTEM_KERNEL_LONG}..."
	[ -z "${REQUIRED_FIRMWARE}" ] && perror "REQUIRED_FIRMWARE is empty. Check your config file."

	check_kernel_base_dir

	local OLD_DIR=$(pwd)
	#
	# process firmware list
	#
	cd "${KERNEL_BASE_DIR}" || perror "Could not cd to ${KERNEL_BASE_DIR}"
	local FIRMWARE_DIR="lib/firmware"
	local FIRMWARE_SYSTEM_LIST=""
	local FIRMWARE_BUILD_LIST=""
	for FIRMWARE in ${REQUIRED_FIRMWARE}; do
		local FOUND=0
		# check for firmware in the build directory of the kernel
		for CANDIDATE in "${FIRMWARE_DIR}/${FIRMWARE}" "${FIRMWARE_DIR}/${TARGET_KERNEL_LONG}/${FIRMWARE}"; do
			if [ -e "${CANDIDATE}" ]; then
				pdebug "Copying from kernel base dir ('$KERNEL_BASE_DIR'): '${CANDIDATE}'"
				FIRMWARE_BUILD_LIST+=" ${CANDIDATE}"
				FOUND=1
			fi
		done

		# dont look under / if KERNEL_BASE_DIR is already /
		#if [ "x${KERNEL_BASE_DIR}" == "x/" ]; then
		#	[ $FOUND -ne 1 ] && pwarning "Neither '${FIRMWARE_DIR}/${FIRMWARE}' nor '${FIRMWARE_DIR}/${TARGET_KERNEL_LONG}/${FIRMWARE}' found on the system"
		#	continue
		#fi

		# if we didn't found it in the kernel build directory, check for firmware in the system firmware directory
		#if [ $FOUND -ne 1 ]; then
		for CANDIDATE in "/${FIRMWARE_DIR}/${FIRMWARE}" "/${FIRMWARE_DIR}/${SYSTEM_KERNEL_LONG}/${FIRMWARE}"; do
			if [ -e "${CANDIDATE}" ]; then
				if [ $(echo "${CANDIDATE}" | grep -c "${SYSTEM_KERNEL_LONG}") -eq 0 ]; then
					pdebug "Copying from system: '${CANDIDATE}'"
					FIRMWARE_SYSTEM_LIST+=" ${CANDIDATE}"
				else
					pdebug "Copying from system: '${CANDIDATE}' to ${FIRMWARE_DIR}/${TARGET_KERNEL_LONG}/${FIRMWARE}"
					FIRMWARE_SYSTEM_LIST+=" /${FIRMWARE_DIR}/${SYSTEM_KERNEL_LONG}/${FIRMWARE}"
				fi
				FOUND=1
			fi
		done
		#fi

		[ $FOUND -ne 1 ] && pwarning "Neither '${FIRMWARE_DIR}/${FIRMWARE}' nor '${FIRMWARE_DIR}/${TARGET_KERNEL_LONG}/${FIRMWARE}' "\
		" was found on the system. Skipping. (might cause problems on certain clients!)"
	done

	for LIST in "${FIRMWARE_SYSTEM_LIST}" "${FIRMWARE_BUILD_LIST}"; do
		[ -z "${LIST}" ] && continue
		echo "${LIST}" >> "${MODULE_BUILD_DIR}/fwlist"
		local COUNT=$(echo "${LIST}" | wc -w)
		pinfo "Copying $COUNT firmware to target directory."
		tarcopy "${LIST}" "${TARGET_BUILD_DIR}"
	done

	# only for kernel-openslx
	# post-process to fix the path of the firmwares found on the system unter /lib/firmware/$(uname -r)
	# which have to be copied to /lib/firmware/${TARGET_KERNEL_LONG}
	if [ "x${KERNEL_BASE_DIR}" != "x/" ]; then
		if [ -d "${TARGET_BUILD_DIR}/lib/firmware/${SYSTEM_KERNEL_LONG}" ]; then
			mkdir -p "${TARGET_BUILD_DIR}/lib/firmware/${TARGET_KERNEL_LONG}/"
			cd "${TARGET_BUILD_DIR}/lib/firmware/${SYSTEM_KERNEL_LONG}" || perror "old kernel but no old kernel"
			tarcopy "$(ls)" "${TARGET_BUILD_DIR}/lib/firmware/${TARGET_KERNEL_LONG}/"
			cd -
			rm -r "${TARGET_BUILD_DIR}/lib/firmware/${SYSTEM_KERNEL_LONG}" || perror "something went very wrong..."
		else
			pdebug "No ${TARGET_BUILD_DIR}/lib/firmware/${SYSTEM_KERNEL_LONG} directory, skipping the merge."
		fi
	fi

	cd "${OLD_DIR}" || perror "Could not cd back to ${OLD_DIR}."
}

copy_kernel () {
	check_kernel_base_dir

	local TOOL_STR="$TOOL_STR copy_kernel:"
	local KERNEL_DIR="${ROOT_DIR}/var/builds/kernel"

	[ -d "${KERNEL_DIR}" ] || mkdir -p "${KERNEL_DIR}"

	cp "${ROOT_DIR}/tmp/work/kernel/build/kernel" "${KERNEL_DIR}" || perror "Could not copy '${ROOT_DIR}/tmp/work/kernel/build/kernel' to '${KERNEL_DIR}'"

	# kernel has 0600 perms since ubuntu 14.04, change that once we copied it
	chmod +r "${KERNEL_DIR}/kernel"
}

