package org.openslx.runvirt.plugin.qemu.configuration;

import java.util.ArrayList;

import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.device.Disk.StorageType;
import org.openslx.libvirt.domain.device.DiskFloppy;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.virtualization.configuration.VirtualizationConfigurationQemuUtils;
import org.openslx.virtualization.configuration.transformation.TransformationException;
import org.openslx.virtualization.configuration.transformation.TransformationGeneric;

public class TransformationGenericDiskFloppyDevices extends TransformationGeneric<Domain, CommandLineArgs>
{
	private static final String FILTER_NAME = "Disk floppy devices";

	public TransformationGenericDiskFloppyDevices()
	{
		super( TransformationGenericDiskFloppyDevices.FILTER_NAME );
	}

	private void filterDiskFloppyDevice( Domain config, String fileName, int index ) throws TransformationException
	{
		final ArrayList<DiskFloppy> devices = config.getDiskFloppyDevices();
		final DiskFloppy disk = VirtualizationConfigurationQemuUtils.getArrayIndex( devices, index );

		if ( disk != null ) {
			if ( fileName == null ) {
				// remove disk floppy device if disk image file name is not set
				disk.remove();
			} else {
				// set image file of disk storage if disk floppy device is available
				disk.setStorage( StorageType.FILE, fileName );
			}
		}
	}

	@Override
	public void transform( Domain config, CommandLineArgs args ) throws TransformationException
	{
		this.filterDiskFloppyDevice( config, args.getVmDiskFileNameFloppy0(), 0 );
		this.filterDiskFloppyDevice( config, args.getVmDiskFileNameFloppy1(), 1 );

		// remove all additional disk storage devices
		final ArrayList<DiskFloppy> devices = config.getDiskFloppyDevices();
		for ( int i = 2; i < devices.size(); i++ ) {
			devices.get( i ).remove();
		}
	}
}
