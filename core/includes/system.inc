# Helper to determine various system information

__init () {
	# determine architecture triplet from the path of libc needed by the executing shell.
	# please try not to use this to find/fetch libs in /lib or /usr/lib.
	# Use ARCH_LIB_DIR, like "$ARCH_LIB_DIR/somefile.so" or "/usr$ARCH_LIB_DIR/somefile.so"
	ARCH_TRIPLET=$(ldd $SHELL|grep "libc.so" | awk -F "/" '{print $3}')
	[[ $ARCH_TRIPLET == *\(*\) ]] && ARCH_TRIPLET=""

	ARCH_LIB_DIR=$(ldd $SHELL | grep "libc.so" | sed -r 's#^.*(/lib.*)/libc.so.*$#\1#g')
	[ -z "$ARCH_LIB_DIR" -o ! -d "$ARCH_LIB_DIR" ] && perror "Could not determine arch dependent lib dir (where libc.so resides)"

	# determine number of CPU cores
	declare -rg CPU_CORES=$(cat /proc/cpuinfo | grep processor | wc -l)
	export MAKEFLAGS="-j$CPU_CORES"

	# Determine if we have lib64
	if [ "$(uname -m)x" = "x86_64x" ]; then
		# Setting LIB64, as openSuse differentiates but Ubuntu does not:
		case $SYS_DISTRIBUTION in
			ubuntu | debian)		LIB64="lib" ;;
			opensuse | fedora | centos)	LIB64="lib64" ;;
			*)			perror "Cannot set LIB64, SYS_DISTRIBUTION: $SYS_DISTRIBUTION unknown!" ;;
		esac
		AMD64_I386=amd64
		X86_64_I586=x86_64
		AMD64_X86=amd64
		ARCHREGEX="(amd64|x86[_-]64)"
	else
		LIB64="lib"
		AMD64_I386=i386
		X86_64_I586=i586
		AMD64_X86=x86
		ARCHREGEX="(i[3456]86|x86[_-]32)"
	fi
}

