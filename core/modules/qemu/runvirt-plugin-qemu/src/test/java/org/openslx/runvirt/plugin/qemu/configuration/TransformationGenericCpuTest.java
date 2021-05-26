package org.openslx.runvirt.plugin.qemu.configuration;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;

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
	@DisplayName( "Test transformation of VM CPU configuration" )
	public void testTransformationGenericCpu() throws TransformationException
	{
		final TransformationGenericCpu transformation = new TransformationGenericCpu();
		final Domain config = TransformationTestUtils.getDefaultDomain();
		final CommandLineArgs args = TransformationTestUtils.getDefaultCmdLnArgs();

		assertNotEquals( Integer.parseInt( TransformationTestUtils.DEFAULT_VM_NCPUS ), config.getVCpu() );
		assertNotEquals( CpuMode.HOST_PASSTHROUGH, config.getCpuMode() );
		assertEquals( CpuCheck.PARTIAL, config.getCpuCheck() );

		transformation.transform( config, args );

		assertEquals( Integer.parseInt( TransformationTestUtils.DEFAULT_VM_NCPUS ), config.getVCpu() );
		assertEquals( CpuMode.HOST_PASSTHROUGH, config.getCpuMode() );
		assertEquals( CpuCheck.PARTIAL, config.getCpuCheck() );
	}

	@Test
	@DisplayName( "Test transformation of VM CPU configuration with unspecified input data" )
	public void testTransformationGenericCpuNoData() throws TransformationException
	{
		final TransformationGenericCpu transformation = new TransformationGenericCpu();
		final Domain config = TransformationTestUtils.getDefaultDomain();
		final CommandLineArgs args = TransformationTestUtils.getEmptyCmdLnArgs();

		assertThrows( TransformationException.class, () -> transformation.transform( config, args ) );
	}
}
