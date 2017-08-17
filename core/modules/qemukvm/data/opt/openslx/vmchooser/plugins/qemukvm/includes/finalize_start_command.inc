# -----------------------------------------------------------------------------
# Copyright (c) 2009..2017 - OpenSLX GmbH
#
# This program/file is free software distributed under the GPL version 2.
# See http://openslx.org/COPYING
#
# If you have any feedback please consult http://openslx.org/feedback and
# send your feedback to feedback@openslx.org
#
# General information about OpenSLX can be found at http://openslx.org
# -----------------------------------------------------------------------------
# run-virt.include
#    - Include script for running the QEMU/Linux KVM virtual machine on an 
#      OpenSLX client via the run-virt.sh or run-vmgrid.sh
################################################################################

# set options that depend on previous includes
# and build the final start command
finalize_start_command() {

	# set cpu type
	VIRTCMDOPTS="${VIRTCMDOPTS} -cpu host"
	# set cpu cores
	VIRTCMDOPTS="${VIRTCMDOPTS} -smp ${CPU_CORES}"

	# set RAM
	VIRTCMDOPTS="${VIRTCMDOPTS} -m ${VM_MEM}"
	
	# special feature for real and virtual floppy disks
	isset FLOPPY_0 && VIRTCMDOPTS="${VIRTCMDOPTS} -fda ${FLOPPY_0}"
	isset SLX_FLOPPY_IMG && VIRTCMDOPTS="${VIRTCMDOPTS} -fdb ${SLX_FLOPPY_IMG}"
	
	# add optical drive if available
	isset CDROM_0 && VIRTCMDOPTS="${VIRTCMDOPTS} -cdrom ${CDROM_0}"
	
	# audio
	isset SOUND_DEV && VIRTCMDOPTS="${VIRTCMDOPTS} -soundhw ${SOUND_DEV}"
	
	# serial devices TODO test
	if isset SERIAL_PORTS; then
		for DEV in $SERIAL_PORTS; do
			: # buggeh: VIRTCMDOPTS="${VIRTCMDOPTS} -serial ${DEV}"
		done
	fi
}

call_post_source finalize_start_command
