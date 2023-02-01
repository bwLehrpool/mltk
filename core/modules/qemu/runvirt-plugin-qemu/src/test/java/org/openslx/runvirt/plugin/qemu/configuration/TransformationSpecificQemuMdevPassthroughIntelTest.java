package org.openslx.runvirt.plugin.qemu.configuration;

import static org.junit.jupiter.api.Assertions.assertDoesNotThrow;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.junit.jupiter.api.Assertions.fail;

import java.io.InputStream;
import java.util.List;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.openslx.libvirt.capabilities.Capabilities;
import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.device.HostdevMdev;
import org.openslx.libvirt.domain.device.HostdevMdev.Model;
import org.openslx.libvirt.domain.device.HostdevMdevDeviceAddress;
import org.openslx.libvirt.domain.device.Video;
import org.openslx.libvirt.xml.LibvirtXmlDocumentException;
import org.openslx.libvirt.xml.LibvirtXmlSerializationException;
import org.openslx.libvirt.xml.LibvirtXmlTestResources;
import org.openslx.libvirt.xml.LibvirtXmlValidationException;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.virtualization.configuration.transformation.TransformationException;

class TransformationSpecificQemuMdevPassthroughIntelStub extends TransformationSpecificQemuMdevPassthroughIntel
{
	final String capabilityFileName;

	public TransformationSpecificQemuMdevPassthroughIntelStub( String capabilityFileName )
	{
		super( null );

		this.capabilityFileName = capabilityFileName;
	}

	@Override
	protected Capabilities getCapabilities() throws TransformationException
	{
		final InputStream capabilityContent = LibvirtXmlTestResources.getLibvirtXmlStream( this.capabilityFileName );
		Capabilities capabilites = null;

		try {
			capabilites = new Capabilities( capabilityContent );
		} catch ( LibvirtXmlDocumentException | LibvirtXmlSerializationException | LibvirtXmlValidationException e ) {
			fail( "Could not create stub for getCapabilities(): " + e.getLocalizedMessage() );
		}

		return capabilites;
	}
}

public class TransformationSpecificQemuMdevPassthroughIntelTest
{
	@Test
	@DisplayName( "Test transformation of VM mediated device passthrough configuration if mediated device passthrough is required" )
	public void testTransformationSpecificQemuMdevPassthroughIntel() throws TransformationException
	{
		final TransformationSpecificQemuMdevPassthroughIntelStub transformation;
		transformation = new TransformationSpecificQemuMdevPassthroughIntelStub( "qemu-kvm_capabilities_default.xml" );
		final Domain config = TransformationTestUtils.getDefaultDomain();
		final CommandLineArgs args = TransformationTestUtils.getDefaultCmdLnArgs();

		transformation.transform( config, args );

		final List<HostdevMdev> mdevDevices = config.getHostdevMdevDevices();
		assertNotNull( mdevDevices );
		assertEquals( 1, mdevDevices.size() );

		final HostdevMdev mdevDevice = mdevDevices.get( 0 );
		assertFalse( mdevDevice.isManaged() );
		assertTrue( mdevDevice.isDisplayOn() );
		assertTrue( mdevDevice.isMemoryFramebufferOn() );
		assertEquals( Model.VFIO_PCI, mdevDevice.getModel() );
		assertEquals( HostdevMdevDeviceAddress.valueOf( TransformationTestUtils.DEFAULT_VM_ILMDEVID0 ),
				mdevDevice.getSource() );

		// TODO: Check the override section of XML

		final List<Video> videoDevices = config.getVideoDevices();
		assertNotNull( videoDevices );
		for ( final Video videoDevice : videoDevices ) {
			assertEquals( Video.Model.NONE, videoDevice.getModel() );
		}

		assertDoesNotThrow( () -> config.validateXml() );
	}

	@Test
	@DisplayName( "Test transformation of VM mediated device passthrough configuration if mediated device passthrouh is not specified" )
	public void testTransformationSpecificQemuMdevPassthroughIntelNoMdev() throws TransformationException
	{
		final TransformationSpecificQemuMdevPassthroughIntelStub transformation;
		transformation = new TransformationSpecificQemuMdevPassthroughIntelStub( "qemu-kvm_capabilities_default.xml" );
		final Domain config = TransformationTestUtils.getDefaultDomain();
		final CommandLineArgs args = TransformationTestUtils.getEmptyCmdLnArgs();

		transformation.transform( config, args );

		final List<HostdevMdev> mdevDevices = config.getHostdevMdevDevices();
		assertNotNull( mdevDevices );
		assertEquals( 0, mdevDevices.size() );

		// TODO: Check the override section of XML

		final List<Video> videoDevices = config.getVideoDevices();
		assertNotNull( videoDevices );
		for ( final Video videoDevice : videoDevices ) {
			assertNotEquals( Video.Model.NONE, videoDevice.getModel() );
		}

		assertDoesNotThrow( () -> config.validateXml() );
	}
}
