package org.openslx.runvirt.plugin.qemu.configuration;

import static org.junit.jupiter.api.Assertions.assertEquals;

import java.util.ArrayList;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.device.Interface;
import org.openslx.libvirt.domain.device.Interface.Model;
import org.openslx.libvirt.domain.device.Interface.Type;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.virtualization.configuration.VirtualizationConfigurationQemu;
import org.openslx.virtualization.configuration.transformation.TransformationException;

public class TransformationGenericInterfaceDevicesTest
{
	@Test
	@DisplayName( "Test transformation of VM network interface devices configuration with specified input data" )
	public void testTransformationGenericInterfaceDevices() throws TransformationException
	{
		final TransformationGenericInterfaceDevices transformation = new TransformationGenericInterfaceDevices();
		final Domain config = TransformationTestUtils.getDefaultDomain();
		final CommandLineArgs args = TransformationTestUtils.getDefaultCmdLnArgs();

		final ArrayList<Interface> devicesBeforeTransformation = config.getInterfaceDevices();
		assertEquals( 1, devicesBeforeTransformation.size() );

		transformation.transform( config, args );

		final ArrayList<Interface> devicesAfterTransformation = config.getInterfaceDevices();
		assertEquals( 1, devicesAfterTransformation.size() );
		final Interface interfaceAfterTransformation = devicesAfterTransformation.get( 0 );
		assertEquals( Type.BRIDGE, interfaceAfterTransformation.getType() );
		assertEquals( Model.VIRTIO, interfaceAfterTransformation.getModel() );
		assertEquals( TransformationTestUtils.DEFAULT_VM_MAC0, interfaceAfterTransformation.getMacAddress() );
		assertEquals( VirtualizationConfigurationQemu.NETWORK_BRIDGE_NAT_DEFAULT,
				interfaceAfterTransformation.getSource() );
	}

	@Test
	@DisplayName( "Test transformation of VM network interface devices configuration with unspecified input data" )
	public void testTransformationGenericInterfaceDevicesNoData() throws TransformationException
	{
		final TransformationGenericInterfaceDevices transformation = new TransformationGenericInterfaceDevices();
		final Domain config = TransformationTestUtils.getDefaultDomain();
		final CommandLineArgs args = TransformationTestUtils.getEmptyCmdLnArgs();

		final ArrayList<Interface> devicesBeforeTransformation = config.getInterfaceDevices();
		assertEquals( 1, devicesBeforeTransformation.size() );

		transformation.transform( config, args );

		final ArrayList<Interface> devicesAfterTransformation = config.getInterfaceDevices();
		assertEquals( 0, devicesAfterTransformation.size() );
	}
}
