package org.openslx.runvirt.plugin.qemu.configuration;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.openslx.libvirt.domain.Domain;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.virtualization.configuration.transformation.TransformationException;

public class TransformationGenericUuidTest
{
	@Test
	@DisplayName( "Test transformation of VM UUID configuration" )
	public void testTransformationGenericUuid() throws TransformationException
	{
		final TransformationGenericUuid transformation = new TransformationGenericUuid();
		final Domain config = TransformationTestUtils.getDefaultDomain();
		final CommandLineArgs args = TransformationTestUtils.getDefaultCmdLnArgs();

		assertNotEquals( TransformationTestUtils.DEFAULT_VM_UUID, config.getUuid() );

		transformation.transform( config, args );

		assertEquals( TransformationTestUtils.DEFAULT_VM_UUID, config.getUuid() );
	}

	@Test
	@DisplayName( "Test transformation of VM UUID configuration with unspecified input data" )
	public void testTransformationGenericUuidNoData() throws TransformationException
	{
		final TransformationGenericUuid transformation = new TransformationGenericUuid();
		final Domain config = TransformationTestUtils.getDefaultDomain();
		final CommandLineArgs args = TransformationTestUtils.getEmptyCmdLnArgs();

		assertThrows( TransformationException.class, () -> transformation.transform( config, args ) );
	}
}
