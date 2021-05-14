package org.openslx.runvirt.plugin.qemu.configuration;

import java.util.ArrayList;

import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.device.Disk.StorageType;
import org.openslx.libvirt.domain.device.DiskCdrom;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.virtualization.configuration.VirtualizationConfigurationQemu;
import org.openslx.virtualization.configuration.VirtualizationConfigurationQemuUtils;
import org.openslx.virtualization.configuration.transformation.TransformationException;
import org.openslx.virtualization.configuration.transformation.TransformationGeneric;

/**
 * Generic CDROM drive transformation for Libvirt/QEMU virtualization configurations.
 * 
 * @author Manuel Bentele
 * @version 1.0
 */
public class TransformationGenericDiskCdromDevices extends TransformationGeneric<Domain, CommandLineArgs>
{
	/**
	 * Name of the configuration transformation.
	 */
	private static final String NAME = "Disk CDROM devices";

	/**
	 * Creates a new generic CDROM drive transformation for Libvirt/QEMU virtualization
	 * configurations.
	 */
	public TransformationGenericDiskCdromDevices()
	{
		super( TransformationGenericDiskCdromDevices.NAME );
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
	 * Transforms a CDROM drive in a virtualization configuration selected by its {@code index}.
	 * 
	 * @param config virtualization configuration for the transformation.
	 * @param fileName name of the image file for the CDROM drive.
	 * @param index number of the CDROM drive in the virtualization configuration that is selected.
	 * @throws TransformationException transformation has failed.
	 */
	private void transformDiskCdromDevice( Domain config, String fileName, int index ) throws TransformationException
	{
		final ArrayList<DiskCdrom> devices = config.getDiskCdromDevices();
		final DiskCdrom disk = VirtualizationConfigurationQemuUtils.getArrayIndex( devices, index );

		if ( disk != null ) {
			if ( fileName == null ) {
				// do not remove disk CDROM drive, but set local physical drive as input source
				disk.setStorage( StorageType.BLOCK, VirtualizationConfigurationQemu.CDROM_DEFAULT_PHYSICAL_DRIVE );
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
		// validate configuration and input arguments
		this.validateInputs( config, args );

		// alter CDROM drives
		this.transformDiskCdromDevice( config, args.getVmDiskFileNameCdrom0(), 0 );
		this.transformDiskCdromDevice( config, args.getVmDiskFileNameCdrom1(), 1 );

		// remove all additional disk CDROM devices
		final ArrayList<DiskCdrom> devices = config.getDiskCdromDevices();
		for ( int i = 2; i < devices.size(); i++ ) {
			devices.get( i ).remove();
		}
	}
}
