package org.openslx.runvirt.plugin.qemu.configuration;

import java.util.ArrayList;

import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.device.Disk.StorageType;
import org.openslx.libvirt.domain.device.DiskCdrom;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.virtualization.configuration.machine.QemuMetaData;
import org.openslx.virtualization.configuration.machine.QemuMetaDataUtils;
import org.openslx.virtualization.configuration.transformation.TransformationException;
import org.openslx.virtualization.configuration.transformation.TransformationGeneric;

public class TransformationGenericDiskCdromDevices extends TransformationGeneric<Domain, CommandLineArgs>
{
	private static final String FILTER_NAME = "Disk CDROM devices";

	public TransformationGenericDiskCdromDevices()
	{
		super( TransformationGenericDiskCdromDevices.FILTER_NAME );
	}

	private void filterDiskCdromDevice( Domain config, String fileName, int index ) throws TransformationException
	{
		final ArrayList<DiskCdrom> devices = config.getDiskCdromDevices();
		final DiskCdrom disk = QemuMetaDataUtils.getArrayIndex( devices, index );

		if ( disk != null ) {
			if ( fileName == null ) {
				// do not remove disk CDROM drive, but set local physical drive as input source
				disk.setStorage( StorageType.BLOCK, QemuMetaData.CDROM_DEFAULT_PHYSICAL_DRIVE );
			} else if ( fileName.equals( "" ) ) {
				// remove storage source if empty string is specified to emulate an empty CDROM drive
				disk.removeStorage();
			} else {
				// set disk image file as storage source of the disk CDROM drive
				disk.setStorage( StorageType.FILE, fileName );
			}
		}
	}

	@Override
	public void transform( Domain config, CommandLineArgs args ) throws TransformationException
	{
		this.filterDiskCdromDevice( config, args.getVmDiskFileNameCdrom0(), 0 );
		this.filterDiskCdromDevice( config, args.getVmDiskFileNameCdrom1(), 1 );

		// remove all additional disk CDROM devices
		final ArrayList<DiskCdrom> devices = config.getDiskCdromDevices();
		for ( int i = 2; i < devices.size(); i++ ) {
			devices.get( i ).remove();
		}
	}
}
