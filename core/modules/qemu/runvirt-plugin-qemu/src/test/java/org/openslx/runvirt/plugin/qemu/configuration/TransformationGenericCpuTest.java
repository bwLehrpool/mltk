package org.openslx.runvirt.plugin.qemu.configuration;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotEquals;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.Domain.CpuCheck;
import org.openslx.libvirt.domain.Domain.CpuMode;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.virtualization.configuration.transformation.TransformationException;

public class TransformationGenericCpuTest
{
	@Test
	@DisplayName( "Test filtering of VM CPU configuration" )
	public void testFilterGenericCpu() throws TransformationException
	{
		final TransformationGenericCpu filter = new TransformationGenericCpu();
		final Domain config = TransformationTestUtils.getDefaultDomain();
		final CommandLineArgs args = TransformationTestUtils.getDefaultCmdLnArgs();

		assertNotEquals( Integer.parseInt( TransformationTestUtils.DEFAULT_VM_NCPUS ), config.getVCpu() );
		assertNotEquals( CpuMode.HOST_PASSTHROUGH, config.getCpuMode() );
		assertEquals( CpuCheck.PARTIAL, config.getCpuCheck() );

		filter.transform( config, args );

		assertEquals( Integer.parseInt( TransformationTestUtils.DEFAULT_VM_NCPUS ), config.getVCpu() );
		assertEquals( CpuMode.HOST_PASSTHROUGH, config.getCpuMode() );
		assertEquals( CpuCheck.PARTIAL, config.getCpuCheck() );
	}
}
