package org.openslx.runvirt.plugin.qemu.configuration;

import static org.junit.jupiter.api.Assertions.assertDoesNotThrow;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotEquals;

import java.util.ArrayList;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.device.Disk.StorageType;
import org.openslx.libvirt.domain.device.DiskFloppy;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.virtualization.configuration.transformation.TransformationException;

public class TransformationGenericDiskFloppyDevicesTest
{
	@Test
	@DisplayName( "Test transformation of VM disk floppy devices configuration with specified input data" )
	public void testTransformationGenericDiskFloppyDevices() throws TransformationException
	{
		final TransformationGenericDiskFloppyDevices transformation = new TransformationGenericDiskFloppyDevices();
		final Domain config = TransformationTestUtils.getDefaultDomain();
		final CommandLineArgs args = TransformationTestUtils.getDefaultCmdLnArgs();

		final ArrayList<DiskFloppy> devicesBeforeTransformation = config.getDiskFloppyDevices();
		assertEquals( 1, devicesBeforeTransformation.size() );
		final DiskFloppy floppyDeviceBeforeTransformation = devicesBeforeTransformation.get( 0 );
		assertEquals( StorageType.FILE, floppyDeviceBeforeTransformation.getStorageType() );
		assertNotEquals( TransformationTestUtils.DEFAULT_VM_FLOPPY0,
				floppyDeviceBeforeTransformation.getStorageSource() );

		transformation.transform( config, args );

		final ArrayList<DiskFloppy> devicesAfterTransformation = config.getDiskFloppyDevices();
		assertEquals( 2, devicesAfterTransformation.size() );
		final DiskFloppy floppyDevice1AfterTransformation = devicesAfterTransformation.get( 0 );
		final DiskFloppy floppyDevice2AfterTransformation = devicesAfterTransformation.get( 1 );
		assertEquals( StorageType.FILE, floppyDevice1AfterTransformation.getStorageType() );
		assertEquals( TransformationTestUtils.DEFAULT_VM_FLOPPY0, floppyDevice1AfterTransformation.getStorageSource() );
		assertEquals( StorageType.FILE, floppyDevice2AfterTransformation.getStorageType() );
		assertEquals( TransformationTestUtils.DEFAULT_VM_FLOPPY1, floppyDevice2AfterTransformation.getStorageSource() );

		assertDoesNotThrow( () -> config.validateXml() );
	}

	@Test
	@DisplayName( "Test transformation of VM disk floppy devices configuration with unspecified input data" )
	public void testTransformationGenericDiskFloppyDevicesNoData() throws TransformationException
	{
		final TransformationGenericDiskFloppyDevices transformation = new TransformationGenericDiskFloppyDevices();
		final Domain config = TransformationTestUtils.getDefaultDomain();
		final CommandLineArgs args = TransformationTestUtils.getEmptyCmdLnArgs();

		final ArrayList<DiskFloppy> devicesBeforeTransformation = config.getDiskFloppyDevices();
		assertEquals( 1, devicesBeforeTransformation.size() );

		transformation.transform( config, args );

		final ArrayList<DiskFloppy> devicesAfterTransformation = config.getDiskFloppyDevices();
		assertEquals( 0, devicesAfterTransformation.size() );

		assertDoesNotThrow( () -> config.validateXml() );
	}
}
