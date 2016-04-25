#######################################################
# Include: check xml file for sanity (legacy, old way #
#######################################################

# test if the xml file is valid
if ! [ -r "${xmlfile}" ]; then
	writelog "${xmlfile} not a readable XML file!"
	slxlog "virt-nonreadable-xml" "XML file not readable: $xmlfile"
	exit 1
fi

# test if XML file
if ! grep -q '<?xml' "${xmlfile}"; then
	writelog "Submitted configuration file ${xmlfile} seems to have wrong XML format"
	slxlog "virt-invalid-xml" "Malformed XML file: $xmlfile" "$xmlfile"
	exit 1
fi

# check for running in graphical environment otherwise no much use here
if [ -z "$DISPLAY" ]; then
	echo -e "\n\tStart only within a graphical desktop!\n"
	exit 1
fi
