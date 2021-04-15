package org.openslx.runvirt.plugin.qemu.configuration;

import java.util.ArrayList;

import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.device.FileSystem;
import org.openslx.libvirt.domain.device.FileSystem.AccessMode;
import org.openslx.libvirt.domain.device.FileSystem.Type;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.virtualization.configuration.machine.QemuMetaDataUtils;
import org.openslx.virtualization.configuration.transformation.TransformationException;
import org.openslx.virtualization.configuration.transformation.TransformationGeneric;

public class TransformationGenericFileSystemDevices extends TransformationGeneric<Domain, CommandLineArgs>
{
	private static final String FILTER_NAME = "File system devices";

	public TransformationGenericFileSystemDevices()
	{
		super( TransformationGenericFileSystemDevices.FILTER_NAME );
	}

	private void filterFileSystemDevice( Domain config, String source, String target, int index ) throws TransformationException
	{
		final ArrayList<FileSystem> devices = config.getFileSystemDevices();
		final FileSystem fileSystem = QemuMetaDataUtils.getArrayIndex( devices, index );

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
		this.filterFileSystemDevice( config, args.getVmFsSrc0(), args.getVmFsTgt0(), 0 );
		this.filterFileSystemDevice( config, args.getVmFsSrc1(), args.getVmFsTgt1(), 1 );

		// remove all additional file system devices
		final ArrayList<FileSystem> devices = config.getFileSystemDevices();
		for ( int i = 1; i < devices.size(); i++ ) {
			devices.get( i ).remove();
		}
	}
}
