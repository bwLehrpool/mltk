package org.openslx.runvirt.plugin.qemu.configuration;

import static org.junit.jupiter.api.Assertions.assertDoesNotThrow;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotEquals;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import java.nio.file.attribute.BasicFileAttributes;
import java.util.ArrayList;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.device.Disk.StorageType;
import org.openslx.libvirt.domain.device.DiskCdrom;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.virtualization.configuration.transformation.TransformationException;

class TransformationGenericDiskCdromDevicesStub extends TransformationGenericDiskCdromDevices
{
	@Override
	protected BasicFileAttributes getFileAttributes( String fileName )
	{
		final BasicFileAttributes fileAttrs;

		if ( fileName.equals( TransformationTestUtils.DEFAULT_VM_CDROM0 ) ) {
			fileAttrs = mock( BasicFileAttributes.class );
			when( fileAttrs.isRegularFile() ).thenReturn( false );
			when( fileAttrs.isOther() ).thenReturn( true );
		} else if ( fileName.equals( TransformationTestUtils.DEFAULT_VM_CDROM1 ) ) {
			fileAttrs = mock( BasicFileAttributes.class );
			when( fileAttrs.isRegularFile() ).thenReturn( true );
			when( fileAttrs.isOther() ).thenReturn( false );
		} else {
			fileAttrs = null;
		}

		return fileAttrs;
	}
}

public class TransformationGenericDiskCdromDevicesTest
{
	@Test
	@DisplayName( "Test transformation of VM disk CDROM devices configuration with specified input data" )
	public void testTransformationGenericDiskCdromDevices() throws TransformationException
	{
		final TransformationGenericDiskCdromDevices transformation = new TransformationGenericDiskCdromDevicesStub();
		final Domain config = TransformationTestUtils.getDefaultDomain();
		final CommandLineArgs args = TransformationTestUtils.getDefaultCmdLnArgs();

		final ArrayList<DiskCdrom> devicesBeforeTransformation = config.getDiskCdromDevices();
		assertEquals( 1, devicesBeforeTransformation.size() );
		final DiskCdrom cdromDeviceBeforeTransformation = devicesBeforeTransformation.get( 0 );
		assertEquals( StorageType.FILE, cdromDeviceBeforeTransformation.getStorageType() );
		assertNotEquals( TransformationTestUtils.DEFAULT_VM_CDROM0, cdromDeviceBeforeTransformation.getStorageSource() );

		transformation.transform( config, args );

		final ArrayList<DiskCdrom> devicesAfterTransformation = config.getDiskCdromDevices();
		assertEquals( 2, devicesAfterTransformation.size() );
		final DiskCdrom cdromDevice1AfterTransformation = devicesAfterTransformation.get( 0 );
		final DiskCdrom cdromDevice2AfterTransformation = devicesAfterTransformation.get( 1 );
		assertEquals( StorageType.BLOCK, cdromDevice1AfterTransformation.getStorageType() );
		assertEquals( TransformationTestUtils.DEFAULT_VM_CDROM0, cdromDevice1AfterTransformation.getStorageSource() );
		assertEquals( StorageType.FILE, cdromDevice2AfterTransformation.getStorageType() );
		assertEquals( TransformationTestUtils.DEFAULT_VM_CDROM1, cdromDevice2AfterTransformation.getStorageSource() );

		assertDoesNotThrow( () -> config.validateXml() );
	}

	@Test
	@DisplayName( "Test transformation of VM disk CDROM devices configuration with unspecified input data" )
	public void testTransformationGenericDiskCdromDevicesNoData() throws TransformationException
	{
		final TransformationGenericDiskCdromDevices transformation = new TransformationGenericDiskCdromDevicesStub();
		final Domain config = TransformationTestUtils.getDefaultDomain();
		final CommandLineArgs args = TransformationTestUtils.getEmptyCmdLnArgs();

		final ArrayList<DiskCdrom> devicesBeforeTransformation = config.getDiskCdromDevices();
		assertEquals( 1, devicesBeforeTransformation.size() );

		transformation.transform( config, args );

		final ArrayList<DiskCdrom> devicesAfterTransformation = config.getDiskCdromDevices();
		assertEquals( 0, devicesAfterTransformation.size() );

		assertDoesNotThrow( () -> config.validateXml() );
	}
}
