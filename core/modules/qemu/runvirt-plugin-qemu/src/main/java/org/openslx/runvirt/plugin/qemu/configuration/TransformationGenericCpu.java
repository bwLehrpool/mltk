package org.openslx.runvirt.plugin.qemu.configuration;

import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.Domain.CpuCheck;
import org.openslx.libvirt.domain.Domain.CpuMode;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.virtualization.configuration.transformation.TransformationException;
import org.openslx.virtualization.configuration.transformation.TransformationGeneric;

public class TransformationGenericCpu extends TransformationGeneric<Domain, CommandLineArgs>
{
	private static final String FILTER_NAME = "CPU [number of cores, mode, ...]";

	public TransformationGenericCpu()
	{
		super( TransformationGenericCpu.FILTER_NAME );
	}

	@Override
	public void transform( Domain config, CommandLineArgs args ) throws TransformationException
	{
		config.setVCpu( args.getVmNumCpus() );
		config.setCpuMode( CpuMode.HOST_PASSTHROUGH );
		config.setCpuCheck( CpuCheck.PARTIAL );
	}
}
