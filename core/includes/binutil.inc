#
# Common functions to copy binaries and their dependancies.
#
############################################################
#	This will parse the output of ldd on given binaries
#	and echo the location of these libs to STDOUT
#	The output of this function has to be used in some
#	way, it only echos!
#
# About local search:
#	It is required that we can search for the dynamic
#	libraries in a specific directory, namely the one
#	where we (potentially) built the binary. If a
#	corresponding library is found, it should take
#	precedence over ones found on the system.
#	This can be done by using the '-l' switch, see below.
#
############################################################
# We use a blacklist mechanism to exclude common libraries.
# This improves runtime quite a bit...
declare -rg BLACKLIST="ld-linux linux-gate linux-vdso libc.so"


# replace ' ' by '|' in the blacklist, so grep can use it directly.
CURRENT_BLACKLIST=$(echo ${BLACKLIST} | sed 's/ /\\|/g')

# Initialise flag and path for local search
LOCALSEARCH=0
LOCALSEARCHDIR=""

DONEDONE=$(mktemp)
echo "-----------------------" > "$DONEDONE"

############################################################
#
# Usage:
#	get_dynamic_dependencies [-l <searchdir>] <binary_list>
# 	  * the list must be seperated by spaces
#	  * the search for lib needed by a binary can be done locally,
#	    using the -l <searchdir> option
#
# Ouput:
#	 Will simply echo list of required libraries

get_dynamic_dependencies() {
	# check if local search is activated by the '-l' switch
	# if so the following argument is the path.
	if [ "x$1" == "x-l" ]; then
		local LOCALSEARCH=1
		shift
		[ ! -d "$1" ] && perror "Directory '$1' does not exist, exiting."
		local LOCALSEARCHDIR="$1"
		shift
	fi
	
	# main loop over the list of binaries
	while [ $# != 0 ]; do
		local BINARY="$1"
		shift
		[ -f "$BINARY" ] || continue
		
		# now run ldd on it and save the output in $LDD_OUT
		local LDD_OUT="ldd_output"
		if ldd "$BINARY" > "$LDD_OUT"; then
			# Case 1: file is a dynamic executable
			for LIB in $(grep -v "${CURRENT_BLACKLIST}${REQUIRED_LDD_BLACKLIST}" "$LDD_OUT" | awk '{print $1 $2 $3}'); do
				# split the entry into an array, ex:
				# libm.so.6 => /lib/libm.so.6 would be split into:
				# LIBLINK[0]   LIBLINK[1]
				local LIBLINK=(${LIB//=>/ })
				# call helper function to find the correct lib
				lib_search
			done
			#TODO: check if "statically linked" is output
		else
			# Case 2: not a dynamic, do nothing
			pdebug "\t\t\t(Not a dynamic.)"
		fi
		rm -f -- "$LDD_OUT"
	done
	
}

############################################################
#
# Usage:
#	lib_search
#
# Output:
#	List of the path including any possible symbolic links
#	of the found libraries.
#
# Note: This function takes no argument. It takes the library
#	to look for from the local array LIBLINK.
#	If the local was activated in get_dynamic_dependencies
#	this will search for the library in LOCALSEARCHDIR first.
#	If its not found, then it will look system-wide.
lib_search() {
	
	# if activated, start by searching the lib locally
	if [ "x$LOCALSEARCH" == "x1" ]; then
		cd "$LOCALSEARCHDIR"
		local LOCAL_MATCHES=$(find . -name "${LIBLINK[0]}") # | awk -F '.' '{print $1}')".so\*)
		cd - >/dev/null
		if [ "x${LOCAL_MATCHES}" != "x" ]; then
			for LOCALLIB in ${LOCAL_MATCHES}; do
				grep -q "^${LOCALLIB}\$" "$DONEDONE" && continue
				echo "${LOCALLIB}" >> "$DONEDONE"
				get_link_chain "${LOCALSEARCHDIR}/${LOCALLIB}" "${LOCALSEARCHDIR}"
				get_dynamic_dependencies -l "${LOCALSEARCHDIR}" "${LOCALLIB}"
			done
			# found the libs, we are done
			return
		fi
		# mark local search as done
	fi
	
	# search the lib on the system since it was not found earlier
	if [ -n "${LIBLINK[1]}" ] && [ "x${LIBLINK[1]}" != "xnot" ]; then
		grep -q "^${LIBLINK[1]}\$" "$DONEDONE" && return
		echo "${LIBLINK[1]}" >> "$DONEDONE"
		# get chain of symlink for that lib
		get_link_chain "${LIBLINK[1]}"
	else
		pwarning "\t\tLib '${LIBLINK[0]}' from required dir '$ENTRY' neither found in build directory nor on this system."
		pwarning "\t\tIf this lib is not supplied by another module, this module will probably fail in your final system"
	fi
}
############################################################
#
# Usage:
#	get_link_chain <link> [prefix]
#	  * <link> must be in absolute form-
#	  * [prefix] is the prefix to strip from the ouput.
#
# Output:
#	 Lists the symlink chain until a hardlink is found.
#
get_link_chain() {
	
	# sanity checks
	[[ "$1" == /* ]] || perror "get_link_chain() requires absolute paths, given: $1"
	if [ ! -e $1 -a ! -L $1 ]; then
		perror "'$1' is a link but its target '$LINK_TARGET' is not in '${LOCALSEARCHDIR}'"
	fi
	local PREFIX=
	if [ $# == 2 ] ; then
		[ -d "$2" ] || perror "get_link_chain: '$2' is not a directory. Local search can't work..."
		# got a prefix
		local PREFIX=$(dirname "$(canonicalize "$2/foo")")
		[ -d "$PREFIX" ] || perror "Could not canonicalize $2"
		[[ "$PREFIX" == */ ]] || PREFIX="$PREFIX/"
	fi
	
	# canonalize
	local LINK=$(canonicalize "$1")
	
	local CHAIN="$LINK"
	
	# write the first link in the chain
	if [ "x$PREFIX" != "x" ]; then
		if [ "x${LINK#$PREFIX}" == "x${LINK}" ]; then
			# prefix was not in the link
			echo "$LINK"
		else
			# prefix was in the link
			echo ./"${LINK#$PREFIX}"
		fi
	else
		# no prefix, copy like it is
		echo "$LINK"
	fi
	
	# now we check for symlinks
	local TRY=0
	while [ -L "$LINK" ] && [ $TRY -lt 10 ]; do
		let TRY=TRY+1
		
		# save the directory prefix
		CURRENTDIR=$(dirname "${LINK}")
		# first follow the link
		local NEWLINK=$(readlink "$LINK")
		[ -z "$NEWLINK" -o "$NEWLINK" = "$LINK" ] && break
		LINK=$NEWLINK
		CHAIN+=" -> $LINK"
		# $LINK can be absolute or relative, check cases
		[[ "$LINK" == /* ]] || LINK=$(canonicalize "$CURRENTDIR"/"${LINK}")
		# write the first link in the chain
		if [ "x$PREFIX" != "x" ]; then
			if [ "x${LINK#$PREFIX}" == "x${LINK}" ]; then
				# prefix was not in the link
				if [ ! -e "$LINK" ]; then
					[ -e "$PREFIX/$LINK" ] && echo "./$LINK"
				else
					echo "$LINK"
				fi
			else
				# prefix was in the link
				echo ./"${LINK#$PREFIX}"
			fi
		else
			# no prefix, copy like it is
			echo "$LINK"
		fi
	done
	pdebug "\t\t$CHAIN"
}
############################################################
#
# Usage:
#	list_basic_libs
#
# Output:
#	list the path of following basic system libraries:
#	- libc.so, ld-linux.so
#
list_basic_libs() {
	for i in $(ldd ${SHELL}); do
		[ $(echo $i | grep '^/' | grep -c ld) -eq 1 -o  $(echo $i | grep '^/' | grep -c libc.so) -eq 1 ] && get_link_chain $i
	done
}

