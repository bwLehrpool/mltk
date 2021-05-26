package org.openslx.runvirt.plugin.qemu.configuration;

import java.util.ArrayList;

import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.device.Disk.BusType;
import org.openslx.libvirt.domain.device.Disk.StorageType;
import org.openslx.libvirt.domain.device.DiskFloppy;
import org.openslx.libvirt.domain.device.DiskStorage;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.virtualization.configuration.VirtualizationConfigurationQemuUtils;
import org.openslx.virtualization.configuration.transformation.TransformationException;
import org.openslx.virtualization.configuration.transformation.TransformationGeneric;

/**
 * Generic storage device (HDD, SSD, ...) transformation for Libvirt/QEMU virtualization
 * configurations.
 * 
 * @author Manuel Bentele
 * @version 1.0
 */
public class TransformationGenericDiskStorageDevices extends TransformationGeneric<Domain, CommandLineArgs>
{
	/**
	 * Name of the configuration transformation.
	 */
	private static final String NAME = "Disk storage devices [HDD, SSD, ...]";

	/**
	 * Creates a new storage device (HDD, SSD, ...) transformation for Libvirt/QEMU virtualization
	 * configurations.
	 */
	public TransformationGenericDiskStorageDevices()
	{
		super( TransformationGenericDiskStorageDevices.NAME );
	}

	/**
	 * Validates a virtualization configuration and input arguments for this transformation.
	 * 
	 * @param config virtualization configuration for the validation.
	 * @param args input arguments for the validation.
	 * @throws TransformationException validation has failed.
	 */
	private void validateInputs( Domain config, CommandLineArgs args ) throws TransformationException
	{
		if ( config == null || args == null ) {
			throw new TransformationException( "Virtualization configuration or input arguments are missing!" );
		}
	}

	/**
	 * Transforms a storage device in a virtualization configuration selected by its {@code index}.
	 * 
	 * @param config virtualization configuration for the transformation.
	 * @param fileName name of the image file for the storage device.
	 * @param index number of the storage device in the virtualization configuration that is
	 *           selected.
	 * @throws TransformationException transformation has failed.
	 */
	private void transformDiskStorageDevice( Domain config, String fileName, int index ) throws TransformationException
	{
		final ArrayList<DiskStorage> devices = config.getDiskStorageDevices();
		final DiskStorage disk = VirtualizationConfigurationQemuUtils.getArrayIndex( devices, index );

		if ( disk == null ) {
			if ( fileName != null && !fileName.isEmpty() ) {
				// storage device does not exist, so create new storage device
				final DiskFloppy newDisk = config.addDiskFloppyDevice();
				newDisk.setBusType( BusType.VIRTIO );
				String targetDevName = VirtualizationConfigurationQemuUtils.createAlphabeticalDeviceName( "vd", index );
				newDisk.setTargetDevice( targetDevName );
				newDisk.setStorage( StorageType.FILE, fileName );
			}
		} else {
			// storage device exists, so update existing storage device
			if ( fileName == null || fileName.isEmpty() ) {
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
		// validate configuration and input arguments
		this.validateInputs( config, args );

		// alter storage device
		this.transformDiskStorageDevice( config, args.getVmDiskFileNameHDD0(), 0 );

		// remove all additional disk storage devices
		final ArrayList<DiskStorage> devices = config.getDiskStorageDevices();
		for ( int i = 1; i < devices.size(); i++ ) {
			devices.get( i ).remove();
		}
	}
}
