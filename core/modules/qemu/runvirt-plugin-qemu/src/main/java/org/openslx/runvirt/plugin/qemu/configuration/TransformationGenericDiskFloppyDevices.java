package org.openslx.runvirt.plugin.qemu.configuration;

import java.util.ArrayList;

import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.device.Disk.BusType;
import org.openslx.libvirt.domain.device.Disk.StorageType;
import org.openslx.libvirt.domain.device.DiskFloppy;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.virtualization.configuration.VirtualizationConfigurationQemuUtils;
import org.openslx.virtualization.configuration.transformation.TransformationException;
import org.openslx.virtualization.configuration.transformation.TransformationGeneric;

/**
 * Generic floppy drive transformation for Libvirt/QEMU virtualization configurations.
 * 
 * @author Manuel Bentele
 * @version 1.0
 */
public class TransformationGenericDiskFloppyDevices extends TransformationGeneric<Domain, CommandLineArgs>
{
	/**
	 * Name of the configuration transformation.
	 */
	private static final String NAME = "Disk floppy devices";

	/**
	 * Creates a new floppy drive transformation for Libvirt/QEMU virtualization configurations.
	 */
	public TransformationGenericDiskFloppyDevices()
	{
		super( TransformationGenericDiskFloppyDevices.NAME );
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
	 * Transforms a floppy drive in a virtualization configuration selected by its {@code index}.
	 * 
	 * @param config virtualization configuration for the transformation.
	 * @param fileName name of the image file for the floppy drive.
	 * @param index number of the floppy drive in the virtualization configuration that is selected.
	 * @throws TransformationException transformation has failed.
	 */
	private void transformDiskFloppyDevice( Domain config, String fileName, int index ) throws TransformationException
	{
		final ArrayList<DiskFloppy> devices = config.getDiskFloppyDevices();
		final DiskFloppy disk = VirtualizationConfigurationQemuUtils.getArrayIndex( devices, index );

		if ( disk == null ) {
			if ( fileName != null ) {
				// floppy device does not exist, so create new floppy device
				final DiskFloppy newDisk = config.addDiskFloppyDevice();
				newDisk.setBusType( BusType.FDC );
				String targetDevName = VirtualizationConfigurationQemuUtils.createAlphabeticalDeviceName( "fd", index );
				newDisk.setTargetDevice( targetDevName );

				if ( fileName.isEmpty() ) {
					newDisk.removeStorage();
				} else {
					newDisk.setStorage( StorageType.FILE, fileName );
				}
			}
		} else {
			// floppy device exists, so update existing floppy device
			if ( fileName == null ) {
				disk.remove();
			} else if ( fileName.isEmpty() ) {
				disk.removeStorage();
			} else {
				disk.setStorage( StorageType.FILE, fileName );
			}
		}
	}

	@Override
	public void transform( Domain config, CommandLineArgs args ) throws TransformationException
	{
		// validate configuration and input arguments
		this.validateInputs( config, args );

		// alter floppy drives
		this.transformDiskFloppyDevice( config, args.getVmDiskFileNameFloppy0(), 0 );
		this.transformDiskFloppyDevice( config, args.getVmDiskFileNameFloppy1(), 1 );

		// remove all additional disk storage devices
		final ArrayList<DiskFloppy> devices = config.getDiskFloppyDevices();
		for ( int i = 2; i < devices.size(); i++ ) {
			devices.get( i ).remove();
		}
	}
}
