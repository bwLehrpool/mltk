package org.openslx.runvirt.plugin.qemu.configuration;

import static org.junit.jupiter.api.Assertions.assertEquals;

import java.util.ArrayList;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.device.FileSystem;
import org.openslx.libvirt.domain.device.FileSystem.AccessMode;
import org.openslx.libvirt.domain.device.FileSystem.Type;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.virtualization.configuration.transformation.TransformationException;

public class TransformationGenericFileSystemDevicesTest
{
	@Test
	@DisplayName( "Test transformation of VM file system devices configuration with specified input data" )
	public void testTransformationGenericFileSystemDevices() throws TransformationException
	{
		final TransformationGenericFileSystemDevices transformation = new TransformationGenericFileSystemDevices();
		final Domain config = TransformationTestUtils.getDefaultDomain();
		final CommandLineArgs args = TransformationTestUtils.getDefaultCmdLnArgs();

		final ArrayList<FileSystem> devicesBeforeTransformation = config.getFileSystemDevices();
		assertEquals( 0, devicesBeforeTransformation.size() );

		transformation.transform( config, args );

		final ArrayList<FileSystem> devicesAfterTransformation = config.getFileSystemDevices();
		assertEquals( 2, devicesAfterTransformation.size() );
		final FileSystem fs1AfterTransformation = devicesAfterTransformation.get( 0 );
		final FileSystem fs2AfterTransformation = devicesAfterTransformation.get( 1 );
		assertEquals( Type.MOUNT, fs1AfterTransformation.getType() );
		assertEquals( AccessMode.MAPPED, fs1AfterTransformation.getAccessMode() );
		assertEquals( TransformationTestUtils.DEFAULT_VM_FSSRC0, fs1AfterTransformation.getSource() );
		assertEquals( TransformationTestUtils.DEFAULT_VM_FSTGT0, fs1AfterTransformation.getTarget() );
		assertEquals( Type.MOUNT, fs2AfterTransformation.getType() );
		assertEquals( AccessMode.MAPPED, fs2AfterTransformation.getAccessMode() );
		assertEquals( TransformationTestUtils.DEFAULT_VM_FSSRC1, fs2AfterTransformation.getSource() );
		assertEquals( TransformationTestUtils.DEFAULT_VM_FSTGT1, fs2AfterTransformation.getTarget() );
	}

	@Test
	@DisplayName( "Test transformation of VM file system devices configuration with unspecified input data" )
	public void testTransformationGenericFileSystemDevicesNoData() throws TransformationException
	{
		final TransformationGenericFileSystemDevices transformation = new TransformationGenericFileSystemDevices();
		final Domain config = TransformationTestUtils.getDefaultDomain();
		final CommandLineArgs args = TransformationTestUtils.getEmptyCmdLnArgs();

		final ArrayList<FileSystem> devicesBeforeTransformation = config.getFileSystemDevices();
		assertEquals( 0, devicesBeforeTransformation.size() );

		transformation.transform( config, args );

		final ArrayList<FileSystem> devicesAfterTransformation = config.getFileSystemDevices();
		assertEquals( 0, devicesAfterTransformation.size() );
	}
}
