package org.openslx.runvirt.plugin.qemu.configuration;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotEquals;

import java.math.BigInteger;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.DomainUtils;
import org.openslx.runvirt.configuration.FilterException;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;

public class FilterGenericMemoryTest
{
	@Test
	@DisplayName( "Test filtering of VM memory configuration" )
	public void testFilterGenericMemory() throws FilterException
	{
		final FilterGenericMemory filter = new FilterGenericMemory();
		final Domain config = FilterTestUtils.getDefaultDomain();
		final CommandLineArgs args = FilterTestUtils.getDefaultCmdLnArgs();

		final BigInteger defaultMemory = DomainUtils.decodeMemory( FilterTestUtils.DEFAULT_VM_MEM, "MiB" );

		assertNotEquals( defaultMemory.toString(), config.getMemory().toString() );
		assertNotEquals( defaultMemory.toString(), config.getCurrentMemory().toString() );

		filter.filter( config, args );

		assertEquals( defaultMemory.toString(), config.getMemory().toString() );
		assertEquals( defaultMemory.toString(), config.getCurrentMemory().toString() );
	}
}
