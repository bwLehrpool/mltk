# -----------------------------------------------------------------------------
#
# Copyright (c) 2009..2018 bwLehrpool-Projektteam
#
# This program/file is free software distributed under the GPL version 2.
# See https://www.gnu.org/licenses/old-licenses/gpl-2.0-standalone.html
#
# If you have any feedback please consult https://bwlehrpool.de and
# send your feedback to support@bwlehrpool.de.
#
# General information about bwLehrpool can be found at https://bwlehrpool.de
#
# -----------------------------------------------------------------------------
# run-virt.include
#    - Include script for running the QEMU/Linux KVM virtual machine on an 
#      OpenSLX client via the run-virt.sh or run-vmgrid.sh
################################################################################

# set options that depend on previous includes
# and build the final start command
finalize_start_command() {

	# set cpu type
	VIRTCMDOPTS+=( "-cpu" "host" )
	# set cpu cores
	VIRTCMDOPTS+=( "-smp" "${CPU_CORES}" )

	# set RAM
	VIRTCMDOPTS+=( "-m" "${VM_MEM}" )

	# special feature for real and virtual floppy disks
	isset FLOPPY_0 && VIRTCMDOPTS+=( "-fda" "${FLOPPY_0}" )
	isset SLX_FLOPPY_IMG && VIRTCMDOPTS+=( "-fdb" "${SLX_FLOPPY_IMG}" )

	# add optical drive if available
	isset CDROM_0 && VIRTCMDOPTS+=( "-cdrom" "${CDROM_0}" )

	# audio
	isset SOUND_DEV && VIRTCMDOPTS+=( "-soundhw" "${SOUND_DEV}" )

	# serial devices TODO test
	if isset SERIAL_PORTS; then
		for DEV in $SERIAL_PORTS; do
			: # buggeh: VIRTCMDOPTS+=( "-serial" "${DEV}" )
		done
	fi
}

call_post_source finalize_start_command
