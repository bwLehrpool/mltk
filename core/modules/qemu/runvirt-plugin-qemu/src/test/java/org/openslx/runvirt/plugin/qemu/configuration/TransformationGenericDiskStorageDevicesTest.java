package org.openslx.runvirt.plugin.qemu.configuration;

import static org.junit.jupiter.api.Assertions.assertDoesNotThrow;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotEquals;

import java.util.ArrayList;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.device.Disk.StorageType;
import org.openslx.libvirt.domain.device.DiskStorage;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.virtualization.configuration.transformation.TransformationException;

public class TransformationGenericDiskStorageDevicesTest
{
	@Test
	@DisplayName( "Test transformation of VM disk storage devices configuration with specified input data" )
	public void testTransformationGenericDiskStorageDevices() throws TransformationException
	{
		final TransformationGenericDiskStorageDevices transformation = new TransformationGenericDiskStorageDevices();
		final Domain config = TransformationTestUtils.getDefaultDomain();
		final CommandLineArgs args = TransformationTestUtils.getDefaultCmdLnArgs();

		final ArrayList<DiskStorage> devicesBeforeTransformation = config.getDiskStorageDevices();
		assertEquals( 1, devicesBeforeTransformation.size() );
		final DiskStorage diskDeviceBeforeTransformation = devicesBeforeTransformation.get( 0 );
		assertNotEquals( StorageType.FILE, diskDeviceBeforeTransformation.getStorageType() );
		assertNotEquals( TransformationTestUtils.DEFAULT_VM_HDD0, diskDeviceBeforeTransformation.getStorageSource() );

		transformation.transform( config, args );

		final ArrayList<DiskStorage> devicesAfterTransformation = config.getDiskStorageDevices();
		assertEquals( 1, devicesAfterTransformation.size() );
		final DiskStorage diskDeviceAfterTransformation = devicesAfterTransformation.get( 0 );
		assertEquals( StorageType.FILE, diskDeviceAfterTransformation.getStorageType() );
		assertEquals( TransformationTestUtils.DEFAULT_VM_HDD0, diskDeviceAfterTransformation.getStorageSource() );

		assertDoesNotThrow( () -> config.validateXml() );
	}

	@Test
	@DisplayName( "Test transformation of VM disk storage devices configuration with unspecified input data" )
	public void testTransformationGenericDiskStorageDevicesNoData() throws TransformationException
	{
		final TransformationGenericDiskStorageDevices transformation = new TransformationGenericDiskStorageDevices();
		final Domain config = TransformationTestUtils.getDefaultDomain();
		final CommandLineArgs args = TransformationTestUtils.getEmptyCmdLnArgs();

		final ArrayList<DiskStorage> devicesBeforeTransformation = config.getDiskStorageDevices();
		assertEquals( 1, devicesBeforeTransformation.size() );

		transformation.transform( config, args );

		final ArrayList<DiskStorage> devicesAfterTransformation = config.getDiskStorageDevices();
		assertEquals( 0, devicesAfterTransformation.size() );

		assertDoesNotThrow( () -> config.validateXml() );
	}
}
