package org.openslx.runvirt.plugin.qemu.configuration;

import static org.junit.jupiter.api.Assertions.assertDoesNotThrow;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNull;

import java.nio.file.Paths;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.openslx.libvirt.domain.Domain;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.virtualization.configuration.transformation.TransformationException;

class TransformationSpecificQemuFirmwareStub extends TransformationSpecificQemuFirmware
{
	public TransformationSpecificQemuFirmwareStub()
	{
		super( null );
	}
}

public class TransformationSpecificQemuFirmwareTest
{
	@Test
	@DisplayName( "Test transformation of VM OS loader (firmware) configuration with specified input data" )
	public void testTransformationSpecificQemuFirmwareUefiLoader() throws TransformationException
	{
		final TransformationSpecificQemuFirmwareStub transformation = new TransformationSpecificQemuFirmwareStub();
		final Domain config = TransformationTestUtils
				.getDomain( "qemu-kvm_default-ubuntu-20-04-vm_transform-non-persistent_uefi.xml" );
		final CommandLineArgs args = TransformationTestUtils.getDefaultCmdLnArgs();

		assertEquals( Paths.get( "/usr/share/edk2-ovmf/x64/OVMF_CODE.fd" ).toString(), config.getOsLoader() );

		transformation.transform( config, args );

		assertEquals( Paths.get( "/usr/share/qemu/edk2-x86_64-code.fd" ).toString(), config.getOsLoader() );

		assertDoesNotThrow( () -> config.validateXml() );
	}

	@Test
	@DisplayName( "Test transformation of missing VM OS loader (firmware) configuration" )
	public void testTransformationSpecificQemuFirmwareNoLoader() throws TransformationException
	{
		final TransformationSpecificQemuFirmwareStub transformation = new TransformationSpecificQemuFirmwareStub();
		final Domain config = TransformationTestUtils
				.getDomain( "qemu-kvm_default-ubuntu-20-04-vm_transform-non-persistent.xml" );
		final CommandLineArgs args = TransformationTestUtils.getDefaultCmdLnArgs();

		assertNull( config.getOsLoader() );

		transformation.transform( config, args );

		assertNull( config.getOsLoader() );

		assertDoesNotThrow( () -> config.validateXml() );
	}
}
