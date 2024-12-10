#!/bin/bash

# Function to check if diretory exists and is not empty
# Param 1: Path to the directory
# Return : True (0) if directory exists and is not empty, otherwise false (1)
function pkg_config_non_empty_dir() {
	local DIR="${1}"

	if [ -d "${DIR}" ] && [ -n "$(ls -A "${DIR}")" ]; then
		return 0;
	else
		return 1;
	fi
}

# Function to process given directory and patch system root directory if necessary
#
# First, all system directories (path without system root prefix) are processed.
# Afterwards, all system root directories are checked (path with system root prefix).
#
# Param 1: Option of the argument, e.g. '-I'
# Param 2: Path to the directory (value of the argument) with/without prefixed '${PKG_CONFIG_SYSROOT_DIR}'
function pkg_config_process_dir() {
	local DIR_OPTION="${1}"
	local DIR_WTH_SYSROOT="${2}"
	local DIR_NON_SYSROOT="${DIR_WTH_SYSROOT##${PKG_CONFIG_SYSROOT_DIR}}"
	DIR_WTH_SYSROOT="${PKG_CONFIG_SYSROOT_DIR}/${DIR_NON_SYSROOT}"

	if pkg_config_non_empty_dir "${DIR_WTH_SYSROOT}"; then
		echo -n "${DIR_OPTION}${DIR_WTH_SYSROOT}"
	else
		if pkg_config_non_empty_dir "${DIR_NON_SYSROOT}"; then
			echo -n "${DIR_OPTION}${DIR_NON_SYSROOT}"
		else
			[[ "${DIR_WTH_SYSROOT}" == "${DIR_NON_SYSROOT}" ]] && \
				echo -n "Directory '${DIR_WTH_SYSROOT}' does not exist!" || \
				echo -n "Directory '${DIR_WTH_SYSROOT}' or '${DIR_NON_SYSROOT}' does not exist!"
			exit 1;
		fi
	fi
}

# Function to patch output of original pkg-config to fix include and library paths
# Param 1..n: Output of original pkg-config as separated commands
# Return    : Patched output of the original pkg-config input.
function pkg_config_patch_sysroot() {

	local OUTPUT_PATCHED=()

	# process all passed parameters
	for PARAM in "${@}"; do
		case "${PARAM}" in
			-I*)
				OUTPUT_PATCHED+=("$(pkg_config_process_dir "-I" "${PARAM##-I}")")
				;;
			-L*)
				OUTPUT_PATCHED+=("$(pkg_config_process_dir "-L" "${PARAM##-L}")")
				;;
			*)
				OUTPUT_PATCHED+=("${PARAM}")
				;;
		esac
	done

	echo "${OUTPUT_PATCHED[@]}"
}

# execute original pkg-config and save output and return code
PKG_CONFIG_OUTPUT="$($(which "pkg-config") "${@}")"
PKG_CONFIG_RETVAL=${?}

# abort with exit code if pkg-config call failed
if [[ ${PKG_CONFIG_RETVAL} -ne 0 ]]; then
	echo "${PKG_CONFIG_OUTPUT}"
	exit ${PKG_CONFIG_RETVAL}
fi

# patch system root directory in pkg-config output
pkg_config_patch_sysroot ${PKG_CONFIG_OUTPUT}
