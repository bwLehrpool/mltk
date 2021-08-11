package org.openslx.runvirt.plugin.qemu.configuration;

import static org.junit.jupiter.api.Assertions.assertDoesNotThrow;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;

import java.util.ArrayList;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.device.Graphics;
import org.openslx.libvirt.domain.device.GraphicsSpice;
import org.openslx.libvirt.domain.device.GraphicsSpice.ImageCompression;
import org.openslx.libvirt.domain.device.GraphicsSpice.StreamingMode;
import org.openslx.libvirt.domain.device.GraphicsVnc;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.virtualization.configuration.transformation.TransformationException;

class TransformationSpecificQemuGraphicsStub extends TransformationSpecificQemuGraphics
{
	public TransformationSpecificQemuGraphicsStub()
	{
		super( null );
	}
}

public class TransformationSpecificQemuGraphicsTest
{
	@Test
	@DisplayName( "Test transformation of VM SPICE graphic device configuration" )
	public void testTransformationSpecificQemuGraphicsSpice() throws TransformationException
	{
		final TransformationSpecificQemuGraphicsStub transformation = new TransformationSpecificQemuGraphicsStub();
		final Domain config = TransformationTestUtils.getDefaultDomain();
		final CommandLineArgs args = TransformationTestUtils.getDefaultCmdLnArgs();

		final ArrayList<Graphics> devicesBeforeTransformation = config.getGraphicDevices();
		assertEquals( 1, devicesBeforeTransformation.size() );
		final ArrayList<GraphicsSpice> spiceDevicesBeforeTransformation = config.getGraphicSpiceDevices();
		assertEquals( 1, spiceDevicesBeforeTransformation.size() );
		final ArrayList<GraphicsVnc> vncDevicesBeforeTransformation = config.getGraphicVncDevices();
		assertEquals( 0, vncDevicesBeforeTransformation.size() );

		transformation.transform( config, args );

		final ArrayList<Graphics> devicesAfterTransformation = config.getGraphicDevices();
		assertEquals( 1, devicesAfterTransformation.size() );
		final ArrayList<GraphicsSpice> spiceDevicesAfterTransformation = config.getGraphicSpiceDevices();
		assertEquals( 1, spiceDevicesAfterTransformation.size() );
		final ArrayList<GraphicsVnc> vncDevicesAfterTransformation = config.getGraphicVncDevices();
		assertEquals( 0, vncDevicesAfterTransformation.size() );

		final GraphicsSpice spiceDeviceAfterTransformation = spiceDevicesAfterTransformation.get( 0 );
		assertEquals( ImageCompression.OFF, spiceDeviceAfterTransformation.getImageCompression() );
		assertFalse( spiceDeviceAfterTransformation.isPlaybackCompressionOn() );
		assertEquals( StreamingMode.OFF, spiceDeviceAfterTransformation.getStreamingMode() );
		assertFalse( spiceDeviceAfterTransformation.isOpenGlEnabled() );

		assertDoesNotThrow( () -> config.validateXml() );
	}

	@Test
	@DisplayName( "Test transformation of VM VNC graphic device" )
	public void testTransformationSpecificQemuGraphicsVnc() throws TransformationException
	{
		final TransformationSpecificQemuGraphicsStub transformation = new TransformationSpecificQemuGraphicsStub();
		final Domain config = TransformationTestUtils
				.getDomain( "qemu-kvm_default-ubuntu-20-04-vm_transform-non-persistent_vnc.xml" );
		final CommandLineArgs args = TransformationTestUtils.getEmptyCmdLnArgs();

		final ArrayList<Graphics> devicesBeforeTransformation = config.getGraphicDevices();
		assertEquals( 1, devicesBeforeTransformation.size() );
		final ArrayList<GraphicsSpice> spiceDevicesBeforeTransformation = config.getGraphicSpiceDevices();
		assertEquals( 0, spiceDevicesBeforeTransformation.size() );
		final ArrayList<GraphicsVnc> vncDevicesBeforeTransformation = config.getGraphicVncDevices();
		assertEquals( 1, vncDevicesBeforeTransformation.size() );

		transformation.transform( config, args );

		final ArrayList<Graphics> devicesAfterTransformation = config.getGraphicDevices();
		assertEquals( 1, devicesAfterTransformation.size() );
		final ArrayList<GraphicsSpice> spiceDevicesAfterTransformation = config.getGraphicSpiceDevices();
		assertEquals( 1, spiceDevicesAfterTransformation.size() );
		final ArrayList<GraphicsVnc> vncDevicesAfterTransformation = config.getGraphicVncDevices();
		assertEquals( 0, vncDevicesAfterTransformation.size() );

		final GraphicsSpice spiceDeviceAfterTransformation = spiceDevicesAfterTransformation.get( 0 );
		assertEquals( ImageCompression.OFF, spiceDeviceAfterTransformation.getImageCompression() );
		assertFalse( spiceDeviceAfterTransformation.isPlaybackCompressionOn() );
		assertEquals( StreamingMode.OFF, spiceDeviceAfterTransformation.getStreamingMode() );
		assertFalse( spiceDeviceAfterTransformation.isOpenGlEnabled() );

		assertDoesNotThrow( () -> config.validateXml() );
	}
}
