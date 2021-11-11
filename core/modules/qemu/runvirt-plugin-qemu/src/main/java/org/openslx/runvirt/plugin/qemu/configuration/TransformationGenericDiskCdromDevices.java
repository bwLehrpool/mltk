package org.openslx.runvirt.plugin.qemu.configuration;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.InvalidPathException;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.attribute.BasicFileAttributes;
import java.util.ArrayList;

import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.device.Disk.BusType;
import org.openslx.libvirt.domain.device.Disk.StorageType;
import org.openslx.libvirt.domain.device.DiskCdrom;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
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
	 * Returns attributes of a file specified by its <i>fileName</i>.
	 * 
	 * @param fileName file name of the file
	 * @return attributes of the specified file.
	 */
	protected BasicFileAttributes getFileAttributes( String fileName )
	{
		BasicFileAttributes fileAttrs = null;

		try {
			final Path diskFilePath = Paths.get( fileName );
			fileAttrs = Files.readAttributes( diskFilePath, BasicFileAttributes.class );
		} catch ( InvalidPathException e ) {
			fileAttrs = null;
		} catch ( IOException e ) {
			fileAttrs = null;
		}

		return fileAttrs;
	}

	/**
	 * Sets the storage of a CDROM drive from a virtualization configuration.
	 * 
	 * @param disk CDROM drive from a virtualization configuration.
	 * @param fileName path to a storage that is set for the specified CDROM drive (e.g. block device
	 *           or image)
	 */
	private void setDiskCdromStorage( DiskCdrom disk, String fileName )
	{
		if ( fileName == null ) {
			// remove disk storage device if disk image file name is not set
			disk.remove();
		} else if ( fileName.isEmpty() ) {
			// remove storage source if empty string is specified to emulate an empty CDROM drive
			disk.removeStorage();
		} else {
			// set disk image file as storage source of the disk CDROM drive
			// check before, whether the referenced file is a regular file or a block device file
			final BasicFileAttributes diskFileAttrs = this.getFileAttributes( fileName );

			// set storage type according to the file attributes of the referenced file
			if ( diskFileAttrs != null ) {
				if ( diskFileAttrs.isOther() && !diskFileAttrs.isRegularFile() ) {
					// referenced file is a block device file
					// set block device storage type
					disk.setStorage( StorageType.BLOCK, fileName );
				} else {
					// referenced file is a regular file
					// set file storage type
					disk.setStorage( StorageType.FILE, fileName );
				}
			}
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

		if ( disk == null ) {
			if ( fileName != null ) {
				// CDROM drive does not exist, so create new CDROM drive
				final BusType devBusType = BusType.SATA;
				final String targetDevName = VirtualizationConfigurationQemuUtils.createDeviceName( config, devBusType );
				final DiskCdrom newDisk = config.addDiskCdromDevice();
				newDisk.setBusType( devBusType );
				newDisk.setTargetDevice( targetDevName );

				this.setDiskCdromStorage( newDisk, fileName );
			}
		} else {
			// CDROM drive exists, so update existing CDROM drive
			this.setDiskCdromStorage( disk, fileName );
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
