package org.openslx.runvirt.plugin.qemu.configuration;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotEquals;

import java.math.BigInteger;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.DomainUtils;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.virtualization.configuration.transformation.TransformationException;

public class TransformationGenericMemoryTest
{
	@Test
	@DisplayName( "Test transformation of VM memory configuration" )
	public void testTransformationGenericMemory() throws TransformationException
	{
		final TransformationGenericMemory transformation = new TransformationGenericMemory();
		final Domain config = TransformationTestUtils.getDefaultDomain();
		final CommandLineArgs args = TransformationTestUtils.getDefaultCmdLnArgs();

		final BigInteger defaultMemory = DomainUtils.decodeMemory( TransformationTestUtils.DEFAULT_VM_MEM, "MiB" );

		assertNotEquals( defaultMemory.toString(), config.getMemory().toString() );
		assertNotEquals( defaultMemory.toString(), config.getCurrentMemory().toString() );

		transformation.transform( config, args );

		assertEquals( defaultMemory.toString(), config.getMemory().toString() );
		assertEquals( defaultMemory.toString(), config.getCurrentMemory().toString() );
	}
}
