function ausgang()
{	rm -f "$KEYTEMP"
	rm -f "$RAWKEYTEMP"
	# umount "$DISKMOUNTDIR"	# as mounted by systemd now.
}

function set_trapping()
{
	### Trap ##########################################
	trap ausgang EXIT SIGHUP SIGINT SIGTERM
	###################################################
}

set_trapping
