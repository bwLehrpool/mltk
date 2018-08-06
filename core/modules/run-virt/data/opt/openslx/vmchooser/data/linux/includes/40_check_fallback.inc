check_fallback() {
	if [ $(mount | grep -c " $USERHOMEDIR type cifs \| $USERHOMEDIR type nfs") -lt 1 ]; then
		logger "openslx sharemapper: fallback mode: home share check failed, doing fallback to vmware mode."
		preliminaries_vmware
	else
		logger "openslx sharemapper: fallback mode: home share check passed, no fallback necessary."
	fi
}
