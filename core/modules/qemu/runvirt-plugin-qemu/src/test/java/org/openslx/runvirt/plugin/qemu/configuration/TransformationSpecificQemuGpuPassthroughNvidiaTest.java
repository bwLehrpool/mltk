package org.openslx.runvirt.plugin.qemu.configuration;

import static org.junit.jupiter.api.Assertions.assertDoesNotThrow;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.junit.jupiter.api.Assertions.fail;

import java.io.InputStream;
import java.math.BigInteger;
import java.util.List;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.openslx.libvirt.capabilities.Capabilities;
import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.device.HostdevPci;
import org.openslx.libvirt.domain.device.HostdevPciDeviceAddress;
import org.openslx.libvirt.domain.device.Shmem;
import org.openslx.libvirt.domain.device.Video;
import org.openslx.libvirt.xml.LibvirtXmlDocumentException;
import org.openslx.libvirt.xml.LibvirtXmlSerializationException;
import org.openslx.libvirt.xml.LibvirtXmlTestResources;
import org.openslx.libvirt.xml.LibvirtXmlValidationException;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.virtualization.configuration.transformation.TransformationException;

class TransformationSpecificQemuGpuPassthroughNvidiaStub extends TransformationSpecificQemuGpuPassthroughNvidia
{
	final String capabilityFileName;

	public TransformationSpecificQemuGpuPassthroughNvidiaStub( String capabilityFileName )
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

public class TransformationSpecificQemuGpuPassthroughNvidiaTest
{
	@Test
	@DisplayName( "Test transformation of VM GPU passthrough configuration if NVIDIA GPU passthrouh is required" )
	public void testTransformationSpecificQemuGpuPassthroughNvidia() throws TransformationException
	{
		final TransformationSpecificQemuGpuPassthroughNvidiaStub transformation;
		transformation = new TransformationSpecificQemuGpuPassthroughNvidiaStub( "qemu-kvm_capabilities_default.xml" );
		final Domain config = TransformationTestUtils.getDefaultDomain();
		final CommandLineArgs args = TransformationTestUtils.getDefaultCmdLnArgs();

		transformation.transform( config, args );

		final List<HostdevPci> pciDevices = config.getHostdevPciDevices();
		assertNotNull( pciDevices );
		assertEquals( 1, pciDevices.size() );

		final HostdevPci pciDevice = pciDevices.get( 0 );
		assertTrue( pciDevice.isManaged() );
		assertEquals( HostdevPciDeviceAddress.valueOf( TransformationTestUtils.DEFAULT_VM_GPU0_ADDR ),
				pciDevice.getSource() );

		final List<Shmem> shmemDevices = config.getShmemDevices();
		assertNotNull( shmemDevices );
		assertEquals( 1, shmemDevices.size() );

		final Shmem shmemDevice = shmemDevices.get( 0 );
		assertEquals( "looking-glass", shmemDevice.getName() );
		assertEquals( Shmem.Model.IVSHMEM_PLAIN, shmemDevice.getModel() );
		assertEquals( BigInteger.valueOf( 67108864 ).toString(), shmemDevice.getSize().toString() );

		assertEquals( TransformationSpecificQemuGpuPassthroughNvidia.HYPERV_VENDOR_ID,
				config.getFeatureHypervVendorIdValue() );
		assertTrue( config.isFeatureHypervVendorIdStateOn() );
		assertTrue( config.isFeatureKvmHiddenStateOn() );

		final List<Video> videoDevices = config.getVideoDevices();
		assertNotNull( videoDevices );
		for ( final Video videoDevice : videoDevices ) {
			assertEquals( Video.Model.NONE, videoDevice.getModel() );
		}

		assertDoesNotThrow( () -> config.validateXml() );
	}

	@Test
	@DisplayName( "Test transformation of VM GPU passthrough configuration if NVIDIA GPU passthrouh is not specified" )
	public void testTransformationSpecificQemuGpuPassthroughNvidiaNoGpu() throws TransformationException
	{
		final TransformationSpecificQemuGpuPassthroughNvidiaStub transformation;
		transformation = new TransformationSpecificQemuGpuPassthroughNvidiaStub( "qemu-kvm_capabilities_default.xml" );
		final Domain config = TransformationTestUtils.getDefaultDomain();
		final CommandLineArgs args = TransformationTestUtils.getEmptyCmdLnArgs();

		transformation.transform( config, args );

		final List<HostdevPci> pciDevices = config.getHostdevPciDevices();
		assertNotNull( pciDevices );
		assertEquals( 0, pciDevices.size() );

		final List<Shmem> shmemDevices = config.getShmemDevices();
		assertNotNull( shmemDevices );
		assertEquals( 0, shmemDevices.size() );

		assertNotEquals( TransformationSpecificQemuGpuPassthroughNvidia.HYPERV_VENDOR_ID,
				config.getFeatureHypervVendorIdValue() );
		assertFalse( config.isFeatureHypervVendorIdStateOn() );
		assertFalse( config.isFeatureKvmHiddenStateOn() );

		final List<Video> videoDevices = config.getVideoDevices();
		assertNotNull( videoDevices );
		for ( final Video videoDevice : videoDevices ) {
			assertNotEquals( Video.Model.NONE, videoDevice.getModel() );
		}

		assertDoesNotThrow( () -> config.validateXml() );
	}
}
