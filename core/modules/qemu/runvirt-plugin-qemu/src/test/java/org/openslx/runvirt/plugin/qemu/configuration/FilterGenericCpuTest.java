package org.openslx.runvirt.plugin.qemu.configuration;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotEquals;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.Domain.CpuCheck;
import org.openslx.libvirt.domain.Domain.CpuMode;
import org.openslx.runvirt.configuration.FilterException;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;

public class FilterGenericCpuTest
{
	@Test
	@DisplayName( "Test filtering of VM CPU configuration" )
	public void testFilterGenericCpu() throws FilterException
	{
		final FilterGenericCpu filter = new FilterGenericCpu();
		final Domain config = FilterTestUtils.getDefaultDomain();
		final CommandLineArgs args = FilterTestUtils.getDefaultCmdLnArgs();

		assertNotEquals( Integer.parseInt( FilterTestUtils.DEFAULT_VM_NCPUS ), config.getVCpu() );
		assertNotEquals( CpuMode.HOST_PASSTHROUGH, config.getCpuMode() );
		assertEquals( CpuCheck.PARTIAL, config.getCpuCheck() );

		filter.filter( config, args );

		assertEquals( Integer.parseInt( FilterTestUtils.DEFAULT_VM_NCPUS ), config.getVCpu() );
		assertEquals( CpuMode.HOST_PASSTHROUGH, config.getCpuMode() );
		assertEquals( CpuCheck.PARTIAL, config.getCpuCheck() );
	}
}
