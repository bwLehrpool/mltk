# Helper to determine various system information

__init () {
	# determine architecture triplet from the path of libc needed by the executing shell.
	# please try not to use this to find/fetch libs in /lib or /usr/lib.
	# Use ARCH_LIB_DIR, like "$ARCH_LIB_DIR/somefile.so" or "/usr$ARCH_LIB_DIR/somefile.so"
	ARCH_TRIPLET=$(ldd $SHELL|grep "libc.so" | awk -F "/" '{print $3}')
	[[ $ARCH_TRIPLET == *\(*\) ]] && ARCH_TRIPLET=""

	ARCH_LIB_DIR=$(ldd $SHELL | grep "libc.so" | sed -r 's#^.*(/lib.*)/libc.so.*$#\1#g')
	[ -z "$ARCH_LIB_DIR" -o ! -d "$ARCH_LIB_DIR" ] && perror "Could not determine arch dependent lib dir (where libc.so resides)"

	# determine number of available CPU cores
	declare -rg CPU_CORES="$( nproc || cat /proc/cpuinfo | grep processor | wc -l )"

	# Determine if we have lib64
	if [ "$(uname -m)" = "x86_64" ]; then
		# Setting LIB64, as openSuse differentiates but Ubuntu does not:
		case "$SYS_DISTRIBUTION" in
			ubuntu | debian)		LIB64="lib" ;;
			opensuse | fedora | centos)	LIB64="lib64" ;;
			*)			perror "Cannot set LIB64, SYS_DISTRIBUTION: $SYS_DISTRIBUTION unknown!" ;;
		esac
		X86_64_I386=x86_64
		PLATFORM_DPKG=amd64
		ARCHREGEX="(amd64|x86[_-]64)"
	else
		perror "Unsupported architecture/platform."
	fi

	# Set up make args
	export MAKEFLAGS="-j$CPU_CORES"
	ver=$(gcc -dumpversion)
	ver=${ver%%.*}
	if [ -n "$ver" ]; then
		# Enable ccache?
		if [ -n "$MLTK_CCACHE" ] && which ccache &> /dev/null; then
			for cdir in "/usr/lib/ccache" "/lib/ccache" "/usr/local/ccache" ""; do
				[ -d "$cdir" ] && break
			done
			if [ -n "$cdir" ] && mountpoint "/mnt/ccache"; then
				export CCACHE_DIR="/mnt/ccache/gcc-${ver}"
				export CCACHE_TEMPDIR="/tmp/ccache"
				export CC="$cdir/gcc-${ver}"
				export CXX="$cdir/g++-${ver}"
				mkdir -p "$CCACHE_DIR" "$CCACHE_TEMPDIR"
				pinfo "ccache detected"
				export MAKEFLAGS="-j$(( CPU_CORES * 2 ))"
			fi
		fi
		# Enable distcc?
		if [ -n "$MLTK_DISTCC" ]; then
			testfile="/tmp/test.$$.${RANDOM}.c"
			echo "int main() { return 0; }" > "$testfile"
			DISTCC_FALLBACK=0 distcc "gcc-${ver}" -o "${testfile}.bin" -c "$testfile" &>/dev/null
			ret=$?
			if [ "$ret" = 0 ] && [ -f "${testfile}.bin" ]; then
				if [[ "$CC$CXX" == */ccache* ]]; then
					# Both
					export CCACHE_PREFIX="distcc"
					pinfo "Using distcc + ccache"
				else
					for cdir in "/usr/lib/distcc" "/lib/distcc" "/usr/local/distcc" ""; do
						[ -d "$cdir" ] && break
					done
					if [ -n "$cdir" ]; then
						export CC="$cdir/gcc-${ver}"
						export CXX="$cdir/g++-${ver}"
					fi
					pinfo "distcc detected"
				fi
				export MAKEFLAGS="-j40"
			fi
			rm -f -- "$testfile" "${testfile}.bin"
		fi
	fi
}

