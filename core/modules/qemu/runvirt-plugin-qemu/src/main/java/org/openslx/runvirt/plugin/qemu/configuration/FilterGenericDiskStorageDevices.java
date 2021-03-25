package org.openslx.runvirt.plugin.qemu.configuration;

import java.util.ArrayList;

import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.device.Disk.StorageType;
import org.openslx.libvirt.domain.device.DiskStorage;
import org.openslx.runvirt.configuration.FilterException;
import org.openslx.runvirt.configuration.FilterGeneric;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.vm.QemuMetaDataUtils;

public class FilterGenericDiskStorageDevices extends FilterGeneric<Domain, CommandLineArgs>
{
	private static final String FILTER_NAME = "Disk storage devices [HDD, SSD, ...]";

	public FilterGenericDiskStorageDevices()
	{
		super( FilterGenericDiskStorageDevices.FILTER_NAME );
	}

	private void filterDiskStorageDevice( Domain config, String fileName, int index ) throws FilterException
	{
		final ArrayList<DiskStorage> devices = config.getDiskStorageDevices();
		final DiskStorage disk = QemuMetaDataUtils.getArrayIndex( devices, index );

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
	public void filter( Domain config, CommandLineArgs args ) throws FilterException
	{
		this.filterDiskStorageDevice( config, args.getVmDiskFileNameHDD0(), 0 );

		// remove all additional disk storage devices
		final ArrayList<DiskStorage> devices = config.getDiskStorageDevices();
		for ( int i = 1; i < devices.size(); i++ ) {
			devices.get( i ).remove();
		}
	}
}
