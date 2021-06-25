package org.openslx.runvirt.plugin.qemu.configuration;

import static org.junit.jupiter.api.Assertions.assertDoesNotThrow;
import static org.junit.jupiter.api.Assertions.assertEquals;

import java.util.ArrayList;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.device.Serial;
import org.openslx.libvirt.domain.device.Serial.Type;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.virtualization.configuration.transformation.TransformationException;

class TransformationSpecificQemuSerialDevicesStub extends TransformationSpecificQemuSerialDevices
{
	public TransformationSpecificQemuSerialDevicesStub()
	{
		super( null );
	}
}

public class TransformationSpecificQemuSerialDevicesTest
{
	@Test
	@DisplayName( "Test transformation of VM serial devices configuration with specified input data" )
	public void testTransformationGenericSerialDevices() throws TransformationException
	{
		final TransformationSpecificQemuSerialDevicesStub transformation = new TransformationSpecificQemuSerialDevicesStub();
		final Domain config = TransformationTestUtils.getDefaultDomain();
		final CommandLineArgs args = TransformationTestUtils.getDefaultCmdLnArgs();

		final ArrayList<Serial> devicesBeforeTransformation = config.getSerialDevices();
		assertEquals( 1, devicesBeforeTransformation.size() );
		final Serial serialDeviceBeforeTransformation = devicesBeforeTransformation.get( 0 );
		assertEquals( Type.PTY, serialDeviceBeforeTransformation.getType() );

		transformation.transform( config, args );

		final ArrayList<Serial> devicesAfterTransformation = config.getSerialDevices();
		assertEquals( 2, devicesAfterTransformation.size() );
		final Serial serialDevice1AfterTransformation = devicesAfterTransformation.get( 0 );
		assertEquals( Type.PTY, serialDevice1AfterTransformation.getType() );
		final Serial serialDevice2AfterTransformation = devicesAfterTransformation.get( 1 );
		assertEquals( Type.DEV, serialDevice2AfterTransformation.getType() );
		assertEquals( TransformationTestUtils.DEFAULT_VM_SERIAL0, serialDevice2AfterTransformation.getSource() );

		assertDoesNotThrow( () -> config.validateXml() );
	}

	@Test
	@DisplayName( "Test transformation of VM serial devices configuration with unspecified input data" )
	public void testTransformationGenericSerialDevicesNoData() throws TransformationException
	{
		final TransformationSpecificQemuSerialDevicesStub transformation = new TransformationSpecificQemuSerialDevicesStub();
		final Domain config = TransformationTestUtils.getDefaultDomain();
		final CommandLineArgs args = TransformationTestUtils.getEmptyCmdLnArgs();

		final ArrayList<Serial> devicesBeforeTransformation = config.getSerialDevices();
		assertEquals( 1, devicesBeforeTransformation.size() );
		final Serial serialDeviceBeforeTransformation = devicesBeforeTransformation.get( 0 );
		assertEquals( Type.PTY, serialDeviceBeforeTransformation.getType() );

		transformation.transform( config, args );

		final ArrayList<Serial> devicesAfterTransformation = config.getSerialDevices();
		assertEquals( 1, devicesAfterTransformation.size() );
		final Serial serialDeviceAfterTransformation = devicesBeforeTransformation.get( 0 );
		assertEquals( Type.PTY, serialDeviceAfterTransformation.getType() );

		assertDoesNotThrow( () -> config.validateXml() );
	}
}
