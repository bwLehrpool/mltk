###########################################
# Include: Set hardware related variables #
###########################################

writelog "Starting configuration..."
writelog "\tLogfile:\t\t${LOGFILE}"
writelog "\t/tmp info:\t\t$(df -h | grep " /tmp$" | awk '{print $2}') \c"
writelog "$(grep "/tmp " /proc/mounts | awk '{print $1" "$2" "$3" "$4}')"
writelog "\tVM XML dir:\t\t$(dirname "${xmlfile}")"
writelog "\tXML file:\t\t${xmlfile}"
writelog "Enable 3D:\t\t${enable3d}"
writelog "VM config:"

# # Name of the virt image
imgname=$(grep -io '<image_path param=.*"' "${xmlfile}" | sed -e "s/&.*;/; /g" | awk -F '"' '{ print $2 }')
imgrelpath=$(grep -io '<image_name param=.*"' "${xmlfile}" | sed -e "s/&.*;/; /g" | awk -F '"' '{ print $2 }')
# [ -z "$imgname" ] && imgname="${imgrelpath}"
imgrelpath=${imgrelpath#/mnt/vmstore/}

writelog "Virtual image file name: $imgname"

# Imagename w/ full path
if echo "${imgname}" | grep -q '^/'; then
	imgpath="$(dirname "${imgname}")"
	imgname="$(basename "${imgname}")"
	vmpath="${imgpath}/${imgname}"
	# If old vmchooser binary stuff
	# We do not need folder name as it is already included by vmchooser
elif echo "${xmlfile}" | grep -q '^/tmp/'; then
	vmpath="$imgname"
	imgname="$(basename "${imgname}")"
	## Added for persistent support.
else
	# Else use same path as xml
	imgpath="${xmlpath}"
	vmpath="${imgpath}/${imgname}"
fi

# If ${vm_name} not defined use ${xmlfile}
vm_name="${vm_name:-${xmlfile%.xml}}"

# Define vm_shortname since vm_name can be very long
vm_shortname="$(basename "${xmlfile%.xml}" | sed -e "s, ,-,g")"

# vm_name = displayname, define for old scripts
displayname="${vm_name}"

# image is for the following virtual machine
xmlvirt=$(grep -o 'virtualmachine param=.*"' "${xmlfile}" \
	| sed -e "s/&.*;/; /g" | awk -F '"' '{print $2}')

# choose the proper virtualization/emulator plugin
[ "x${xmlvirt}" != "x" -a "x${xmlvirt}" != "xqemukvm" -a "x${xmlvirt}" != "xvirtualbox" -a "x${xmlvirt}" != "xvmware" ] && xmlvirt="emufe"

# Extracting OS type (vmostype) from xml file. We don't care here whether vmostype is empty, as then 
# it will yield the default entries later on.
vmostype=$(grep -io '<os param=.*"' "${xmlfile}" | sed -e "s/&.*;/; /g" | awk -F '"' '{ print $2 }')

# make a guess from the filename extension if ${xmlvirt} is empty
# (not set within the XML file)
# TODO: implement possibility to submit own configuration files
if [ -z "${xmlvirt}" ] && [ -n "${additional_config}" ]; then
	writelog "No virtual machine parameter defined in ${xmlfile}"
	writelog "Trying to guess VM...\c"
	case "$(cat ${additional_config} | tr \"[A-Z]\" \"[a-z]\")" in
		*config.version*|*virtualhw.version*|*independent-nonpersistent*|*vmdk*)
			xmlvirt="vmware"
		;;
		*innotek*|*virtualbox*)
			xmlvirt="virtualbox"
		;;
		*qemu*|*kvm*)
			xmlvirt="qemukvm"
		;;
		*)
			xmlvirt="none"
		;;
	esac
	elif [ -z "${xmlvirt}" ]; then
	case "$(echo ${imgname##*.} | tr \"[A-Z]\" \"[a-z]\")" in
		vmdk)
			xmlvirt="vmware"
		;;
		vbox|vdi)
			xmlvirt="virtualbox"
		;;
		qcow*)
			xmlvirt="qemukvm"
		;;
		*)
			xmlvirt="emufe"
		;;
	esac
	writelog "result:\t${xmlvirt}"
fi

# Set redirects to 0, see vmgrid if you want to define some
redirects=0

writelog "\tVirtualization:\t\t$xmlvirt"
writelog "\tVM name:\t\t$vm_name"
writelog "\tVM short name:\t\t$vm_shortname"
writelog "\tVM OS:\t\t$vmostype" 
