#########################################################
# Include: Get xml file and directory (legacy, old way) #
#########################################################

## Added for persistent support.
diskmode='nonpersistent'
if [[ "$(grep --extended-regexp \
		'< *short_description *param=".+ --persistent--"' "$1")" ]]; then
	diskmode='persistent'
	imageFilePath="$(grep -io '<image_path param=.*"' "$1" | sed -e "s/&.*;/; /g" | awk -F '"' '{ print $2 }')"
	[ -z "$imageFilePath" ] && imageFilePath="$(grep -io '<image_name param=.*"' "$1" | sed -e "s/&.*;/; /g" | awk -F '"' '{ print $2 }')"
	originalXMLFilePath="$(grep -io '<original_xml_file_path param=.*"' "$1" \
		| sed -e "s/&.*;/; /g" | awk -F '"' '{ print $2 }')"
	originalImageFileName="$(grep -io '<image_name param=.*"' \
		"$originalXMLFilePath" | sed -e "s/&.*;/; /g" | awk -F '"' \
		'{ print $2 }')"
	originalVMDKFilePath="$(dirname \
		"$originalXMLFilePath")/${originalImageFileName}"
	# Added to workaround path prefix coming from temporary generated xml file
	# by vmchooser.
	if [[ "$(grep --extended-regexp '^/tmp/tmp\.[^\/]+/' <<< \
		"$imageFilePath")" ]]; then
		imageFilePath=$(sed -r 's/^\/?tmp\/[^\/]+\///g' \
		<<< "$imageFilePath")
	sed -ir \
		"s/(<image_name param=\")[^\"]*(\")/\\1$(sed -e \
		's/\\/\\\\/g' -e 's/\//\\\//g' -e \
		's/&/\\\&/g' <<< "$imageFilePath")\\2/g" "$1"
	fi
	# Create linked clone if not exists.
	if [ ! -f "$imageFilePath" ]; then
		mkdir -p "$(dirname "$imageFilePath")" && \
		bash "$(dirname "$0")/vmchooser-clc" "$originalXMLFilePath" \
			"$imageFilePath" --debug --verbose &>"${LOGFILE}"
	fi
fi

# absolute or relative path?
xmlfile=$1
if ls "${xmlfile}" 2>/dev/null | grep '/' >/dev/null 2>&1; then
	xmlpath=$(dirname "${xmlfile}")
	xmlfile=$(basename "${xmlfile}")
else
	xmlpath=${vmchooser_xmlpath}
fi
# full path
xmlfile="${xmlpath}/${xmlfile%.xml}.xml"

DO_PROFILE="$(grep -c -E -i '<\s*profile\s*param="?(true|yes)"?' "$xmlfile")"

