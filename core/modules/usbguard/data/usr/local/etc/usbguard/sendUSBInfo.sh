#!/bin/bash

# use SLX_KCL_SERVERS
. /opt/openslx/config

SERVER_IP=${SLX_PXE_SERVER_IP}
CLIENT_IP=${SLX_PXE_CLIENT_IP}
NADAPTER="br0"

if [ "$USBGUARD_DEVICE_EVENT" == "Insert" ]
then
	id=$(echo $USBGUARD_DEVICE_RULE | grep -Pzo '(?s)(?<=id\s).*(?=\sserial)')
	serial=$(echo $USBGUARD_DEVICE_RULE | grep -Pzo '(?s)(?<=serial\s").*(?="\sname)')
	name=$(echo $USBGUARD_DEVICE_RULE | grep -Pzo '(?s)(?<=name\s").*(?="\shash)')
	vhash=$(echo $USBGUARD_DEVICE_RULE | grep -Pzo '(?s)(?<=\shash\s").*(?="\sparent-hash)')
	phash=$(echo $USBGUARD_DEVICE_RULE | grep -Pzo '(?s)(?<=\sparent-hash\s").*(?="\svia-port)')
	vport=$(echo $USBGUARD_DEVICE_RULE | grep -Pzo '(?s)(?<=\svia-port\s").*(?="\swith-interface)')
	interface=$(echo $USBGUARD_DEVICE_RULE | grep -Pzo '(?s)(?<=\swith-interface\s).*')
	# nat1 ONLY WORKS FOR some VM's THIS NEEDS TO BE EDITED. IP Info only needed for getting the machineuuid -> location of the machine.
	# interface-policy is not needed here is it? --> Once it's implemented in usbguard it is.. but currently the device rule doesn't have those information.
	url=$(echo "http://$SERVER_IP/slx-admin/api.php?do=usbguard&action=newdevice&id=$id&serial=$serial&name=$name&ip=$CLIENT_IP&hash=$vhash&parent-hash=$phash&via-port=$vport&with-interface=$interface" | sed 's/ /%20/g')
	curl $url
fi
