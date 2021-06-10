package org.openslx.runvirt.plugin.qemu.configuration;

import static org.junit.jupiter.api.Assertions.assertDoesNotThrow;
import static org.junit.jupiter.api.Assertions.assertEquals;

import java.util.ArrayList;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.device.Parallel;
import org.openslx.libvirt.domain.device.Parallel.Type;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.virtualization.configuration.transformation.TransformationException;

public class TransformationGenericParallelDevicesTest
{
	@Test
	@DisplayName( "Test transformation of VM parallel devices configuration with specified input data" )
	public void testTransformationGenericParallelDevices() throws TransformationException
	{
		final TransformationGenericParallelDevices transformation = new TransformationGenericParallelDevices();
		final Domain config = TransformationTestUtils.getDefaultDomain();
		final CommandLineArgs args = TransformationTestUtils.getDefaultCmdLnArgs();

		final ArrayList<Parallel> devicesBeforeTransformation = config.getParallelDevices();
		assertEquals( 0, devicesBeforeTransformation.size() );

		transformation.transform( config, args );

		final ArrayList<Parallel> devicesAfterTransformation = config.getParallelDevices();
		assertEquals( 1, devicesAfterTransformation.size() );
		final Parallel parallelDeviceAfterTransformation = devicesAfterTransformation.get( 0 );
		assertEquals( Type.DEV, parallelDeviceAfterTransformation.getType() );
		assertEquals( TransformationTestUtils.DEFAULT_VM_PARALLEL0, parallelDeviceAfterTransformation.getSource() );

		assertDoesNotThrow( () -> config.validateXml() );
	}

	@Test
	@DisplayName( "Test transformation of VM parallel devices configuration with unspecified input data" )
	public void testTransformationGenericParallelDevicesNoData() throws TransformationException
	{
		final TransformationGenericParallelDevices transformation = new TransformationGenericParallelDevices();
		final Domain config = TransformationTestUtils.getDefaultDomain();
		final CommandLineArgs args = TransformationTestUtils.getEmptyCmdLnArgs();

		final ArrayList<Parallel> devicesBeforeTransformation = config.getParallelDevices();
		assertEquals( 0, devicesBeforeTransformation.size() );

		transformation.transform( config, args );

		final ArrayList<Parallel> devicesAfterTransformation = config.getParallelDevices();
		assertEquals( 0, devicesAfterTransformation.size() );

		assertDoesNotThrow( () -> config.validateXml() );
	}
}
