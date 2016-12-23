######################################################################
# Include: Parsing the non-legacy delivered vmxfile ${TMPCONFIG} #
######################################################################

writelog "Parsing virtual machine description file..."

HWVER=$(grep -i -m1 '^virtualHW.version *= *' "${TMPCONFIG}" | awk -F '=' '{print $2}' | sed -r 's/[^0-9]//g')

