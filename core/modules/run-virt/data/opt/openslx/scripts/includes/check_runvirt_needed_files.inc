#################################################################
# Include: Check for important files used by vmchooser_run-virt #
#################################################################

# WARNING: This is perhaps vestigial!

filecheck() {
        filecheck=$(LANG=us ls -lh ${diskfile} 2>&1)
        writelog "Filecheck:\n${filecheck}\n"
        noimage=$(echo ${filecheck} | grep -i "no such file or directory" | wc -l)
        rightsfile=${diskfile}

        # check if link
        if [ -L "${diskfile}" ]; then
                # take link target
                rightsfile=$(ls -lh ${diskfile} 2>&1 | awk -F '-> *' '{print $2}')
                rightsfile=${vmdir}/${rightsfile}
                filecheck=$(LANG=us ls -lh ${rightsfile} 2>&1)
        fi

        # does file exist
        if [ "${noimage}" -ge "1" ]; then
                writelog "Virtual Machine Image Problem:\c "
                writelog "\tThe image you've specified doesn't exist."
                writelog "Filecheck says:\c "
                writelog "\t\t${diskfile}:\n\t\t\tNo such file or directory"
                writelog "Hint:\c "
                writelog "\t\t\tCompare spelling of the image with your options.\n"
                exit 1
        fi
        # readable by calling user
        if ! [ -r "${diskfile}" >/dev/null 2>&1 \
                -o -r "${diskfile}" >/dev/null 2>&1 ]; then
                writelog "Vmware Image Problem:\c "
                writelog "\tThe image you've specified has wrong rights."
                writelog "Filecheck says:\t\t$(echo ${filecheck} \
                | awk '{print $1" "$3" "$4}') ${rightsfile}"
                writelog "Hint:\t\t\tChange rights with: chmod a+r ${rightsfile}\n"
                exit 1
        fi

        # writable (for persistent-mode)?
        if ! [ -w "${diskfile}" >/dev/null 2>&1 \
        -o -w "${diskfile}" >/dev/null 2>&1 ] \
        && [ "${np}" = "independent-persistent" ]; then
                writelog "Vmware Image Problem:\c "
                writelog "\tThe image you have specified has wrong rights."
                writelog "Filecheck says:\t\t$(echo ${filecheck} \
                | awk '{print $1" "$3" "$4}') ${rightsfile}"
                writelog "Hint:\t\t\tUse nonpersistent-mode or change rights to rw\n"
                exit 1
        fi
}
