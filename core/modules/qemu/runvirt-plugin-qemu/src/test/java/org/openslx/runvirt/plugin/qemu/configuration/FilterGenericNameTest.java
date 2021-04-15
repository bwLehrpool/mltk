package org.openslx.runvirt.plugin.qemu.configuration;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotEquals;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.openslx.libvirt.domain.Domain;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.virtualization.configuration.transformation.TransformationException;

public class FilterGenericNameTest
{
	@Test
	@DisplayName( "Test filtering of VM (display) name configuration" )
	public void testFilterGenericName() throws TransformationException
	{
		final TransformationGenericName filter = new TransformationGenericName();
		final Domain config = FilterTestUtils.getDefaultDomain();
		final CommandLineArgs args = FilterTestUtils.getDefaultCmdLnArgs();

		assertNotEquals( FilterTestUtils.DEFAULT_VM_NAME, config.getName() );
		assertNotEquals( FilterTestUtils.DEFAULT_VM_DSPLNAME, config.getTitle() );

		filter.transform( config, args );

		assertEquals( FilterTestUtils.DEFAULT_VM_NAME, config.getName() );
		assertEquals( FilterTestUtils.DEFAULT_VM_DSPLNAME, config.getTitle() );
	}
}
