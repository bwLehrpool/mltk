package org.openslx.runvirt.plugin.qemu.configuration;

import java.util.ArrayList;

import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.device.FileSystem;
import org.openslx.libvirt.domain.device.FileSystem.AccessMode;
import org.openslx.libvirt.domain.device.FileSystem.Type;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.virtualization.configuration.VirtualizationConfigurationQemuUtils;
import org.openslx.virtualization.configuration.transformation.TransformationException;
import org.openslx.virtualization.configuration.transformation.TransformationGeneric;

/**
 * Generic file system device (shared folder) transformation for Libvirt/QEMU virtualization
 * configurations.
 * 
 * @author Manuel Bentele
 * @version 1.0
 */
public class TransformationGenericFileSystemDevices extends TransformationGeneric<Domain, CommandLineArgs>
{
	/**
	 * Name of the configuration transformation.
	 */
	private static final String NAME = "File system devices";

	/**
	 * Creates a new file system device (shared folder) transformation for Libvirt/QEMU
	 * virtualization configurations.
	 */
	public TransformationGenericFileSystemDevices()
	{
		super( TransformationGenericFileSystemDevices.NAME );
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
	 * @param source path of the file system source on a host system.
	 * @param target path of the file system destination in a virtualization guest.
	 * @param index number of the file system device in the virtualization configuration that is
	 *           selected.
	 * @throws TransformationException transformation has failed.
	 */
	private void transformFileSystemDevice( Domain config, String source, String target, int index )
			throws TransformationException
	{
		final ArrayList<FileSystem> devices = config.getFileSystemDevices();
		final FileSystem fileSystem = VirtualizationConfigurationQemuUtils.getArrayIndex( devices, index );

		if ( fileSystem == null ) {
			// check if file system device source directory is specified
			if ( source != null && target != null ) {
				// file system device does not exist, so create new file system device
				final FileSystem newFileSystem = config.addFileSystemDevice();
				newFileSystem.setType( Type.MOUNT );
				newFileSystem.setAccessMode( AccessMode.MAPPED );
				newFileSystem.setSource( source );
				newFileSystem.setTarget( target );
			}
		} else {
			if ( source == null || target == null ) {
				// remove file system device since device source or target is not specified
				fileSystem.remove();
			} else {
				// change type, access mode, source and target of existing file system device
				fileSystem.setType( Type.MOUNT );
				fileSystem.setAccessMode( AccessMode.MAPPED );
				fileSystem.setSource( source );
				fileSystem.setTarget( target );
			}
		}
	}

	@Override
	public void transform( Domain config, CommandLineArgs args ) throws TransformationException
	{
		// validate configuration and input arguments
		this.validateInputs( config, args );

		// alter file system devices
		this.transformFileSystemDevice( config, args.getVmFsSrc0(), args.getVmFsTgt0(), 0 );
		this.transformFileSystemDevice( config, args.getVmFsSrc1(), args.getVmFsTgt1(), 1 );

		// remove all additional file system devices
		final ArrayList<FileSystem> devices = config.getFileSystemDevices();
		for ( int i = 1; i < devices.size(); i++ ) {
			devices.get( i ).remove();
		}
	}
}
