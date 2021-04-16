package org.openslx.runvirt.plugin.qemu.configuration;

import java.util.ArrayList;

import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.device.Disk.StorageType;
import org.openslx.libvirt.domain.device.DiskStorage;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.virtualization.configuration.VirtualizationConfigurationQemuUtils;
import org.openslx.virtualization.configuration.transformation.TransformationException;
import org.openslx.virtualization.configuration.transformation.TransformationGeneric;

public class TransformationGenericDiskStorageDevices extends TransformationGeneric<Domain, CommandLineArgs>
{
	private static final String FILTER_NAME = "Disk storage devices [HDD, SSD, ...]";

	public TransformationGenericDiskStorageDevices()
	{
		super( TransformationGenericDiskStorageDevices.FILTER_NAME );
	}

	private void filterDiskStorageDevice( Domain config, String fileName, int index ) throws TransformationException
	{
		final ArrayList<DiskStorage> devices = config.getDiskStorageDevices();
		final DiskStorage disk = VirtualizationConfigurationQemuUtils.getArrayIndex( devices, index );

		if ( disk != null ) {
			if ( fileName == null ) {
				// remove disk storage device if disk image file name is not set
				disk.remove();
			} else {
				// set image file of disk storage if disk storage device is available
				disk.setStorage( StorageType.FILE, fileName );
			}
		}
	}

	@Override
	public void transform( Domain config, CommandLineArgs args ) throws TransformationException
	{
		this.filterDiskStorageDevice( config, args.getVmDiskFileNameHDD0(), 0 );

		// remove all additional disk storage devices
		final ArrayList<DiskStorage> devices = config.getDiskStorageDevices();
		for ( int i = 1; i < devices.size(); i++ ) {
			devices.get( i ).remove();
		}
	}
}
