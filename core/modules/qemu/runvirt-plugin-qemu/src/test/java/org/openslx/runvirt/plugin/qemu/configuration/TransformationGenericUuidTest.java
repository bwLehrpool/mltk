package org.openslx.runvirt.plugin.qemu.configuration;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotEquals;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.openslx.libvirt.domain.Domain;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.virtualization.configuration.transformation.TransformationException;

public class TransformationGenericUuidTest
{
	@Test
	@DisplayName( "Test filtering of VM UUID configuration" )
	public void testFilterGenericUuid() throws TransformationException
	{
		final TransformationGenericUuid filter = new TransformationGenericUuid();
		final Domain config = TransformationTestUtils.getDefaultDomain();
		final CommandLineArgs args = TransformationTestUtils.getDefaultCmdLnArgs();

		assertNotEquals( TransformationTestUtils.DEFAULT_VM_UUID, config.getUuid() );

		filter.transform( config, args );

		assertEquals( TransformationTestUtils.DEFAULT_VM_UUID, config.getUuid() );
	}
}
