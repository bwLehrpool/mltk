package org.openslx.runvirt.plugin.qemu.configuration;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.openslx.libvirt.domain.Domain;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.virtualization.configuration.transformation.TransformationException;

public class TransformationGenericNameTest
{
	@Test
	@DisplayName( "Test transformation of VM (display) name configuration" )
	public void testTransformationGenericName() throws TransformationException
	{
		final TransformationGenericName transformation = new TransformationGenericName();
		final Domain config = TransformationTestUtils.getDefaultDomain();
		final CommandLineArgs args = TransformationTestUtils.getDefaultCmdLnArgs();

		assertNotEquals( TransformationTestUtils.DEFAULT_VM_NAME, config.getName() );
		assertNotEquals( TransformationTestUtils.DEFAULT_VM_DSPLNAME, config.getTitle() );

		transformation.transform( config, args );

		assertEquals( TransformationTestUtils.DEFAULT_VM_NAME, config.getName() );
		assertEquals( TransformationTestUtils.DEFAULT_VM_DSPLNAME, config.getTitle() );
	}

	@Test
	@DisplayName( "Test transformation of VM (display) name configuration with unspecified input data" )
	public void testTransformationGenericNameNoData() throws TransformationException
	{
		final TransformationGenericName transformation = new TransformationGenericName();
		final Domain config = TransformationTestUtils.getDefaultDomain();
		final CommandLineArgs args = TransformationTestUtils.getEmptyCmdLnArgs();

		assertThrows( TransformationException.class, () -> transformation.transform( config, args ) );
	}
}
