package org.openslx.runvirt.plugin.qemu.configuration;

import static org.junit.jupiter.api.Assertions.assertDoesNotThrow;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.fail;

import java.io.InputStream;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.openslx.libvirt.capabilities.Capabilities;
import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.Domain.OsType;
import org.openslx.libvirt.domain.Domain.Type;
import org.openslx.libvirt.xml.LibvirtXmlDocumentException;
import org.openslx.libvirt.xml.LibvirtXmlSerializationException;
import org.openslx.libvirt.xml.LibvirtXmlTestResources;
import org.openslx.libvirt.xml.LibvirtXmlValidationException;
import org.openslx.virtualization.configuration.transformation.TransformationException;

class TransformationSpecificQemuArchitectureStub extends TransformationSpecificQemuArchitecture
{
	final String capabilityFileName;

	public TransformationSpecificQemuArchitectureStub( String capabilityFileName )
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

public class TransformationSpecificQemuArchitectureTest
{
	@Test
	@DisplayName( "Test transformation of VM architecture configuration if KVM required and available" )
	public void testTransformationSpecificQemuArchitectureKvm() throws TransformationException
	{
		final TransformationSpecificQemuArchitectureStub transformation;
		transformation = new TransformationSpecificQemuArchitectureStub( "qemu-kvm_capabilities_default.xml" );
		final Domain config = TransformationTestUtils.getDefaultDomain();

		assertEquals( Type.KVM, config.getType() );
		assertEquals( "x86_64", config.getOsArch() );
		assertEquals( "pc-q35-5.1", config.getOsMachine() );
		assertEquals( OsType.HVM, config.getOsType() );

		transformation.transform( config, null );

		assertEquals( Type.KVM, config.getType() );
		assertEquals( "x86_64", config.getOsArch() );
		assertEquals( "pc-q35-5.1", config.getOsMachine() );
		assertEquals( OsType.HVM, config.getOsType() );

		assertDoesNotThrow( () -> config.validateXml() );
	}

	@Test
	@DisplayName( "Test transformation of VM architecture configuration if KVM required but not available" )
	public void testTransformationSpecificQemuArchitectureNoKvm() throws TransformationException
	{
		final TransformationSpecificQemuArchitectureStub transformation;
		transformation = new TransformationSpecificQemuArchitectureStub( "qemu-kvm_capabilities_no-kvm.xml" );
		final Domain config = TransformationTestUtils.getDefaultDomain();

		assertEquals( Type.KVM, config.getType() );
		assertEquals( "x86_64", config.getOsArch() );
		assertEquals( "pc-q35-5.1", config.getOsMachine() );
		assertEquals( OsType.HVM, config.getOsType() );

		assertThrows( TransformationException.class, () -> transformation.transform( config, null ) );
	}

	@Test
	@DisplayName( "Test transformation of VM architecture configuration if version is not supported (machine version too new) and conversion required" )
	public void testTransformationSpecificQemuArchitectureMachineVersionDowngrade() throws TransformationException
	{
		final TransformationSpecificQemuArchitectureStub transformation;
		transformation = new TransformationSpecificQemuArchitectureStub( "qemu-kvm_capabilities_old-version.xml" );
		final Domain config = TransformationTestUtils.getDefaultDomain();

		assertEquals( Type.KVM, config.getType() );
		assertEquals( "x86_64", config.getOsArch() );
		assertEquals( "pc-q35-5.1", config.getOsMachine() );
		assertEquals( OsType.HVM, config.getOsType() );

		assertDoesNotThrow( () -> transformation.transform( config, null ) );

		assertEquals( Type.KVM, config.getType() );
		assertEquals( "x86_64", config.getOsArch() );
		assertEquals( "pc-q35-4.2", config.getOsMachine() );
		assertEquals( OsType.HVM, config.getOsType() );
	}
}
