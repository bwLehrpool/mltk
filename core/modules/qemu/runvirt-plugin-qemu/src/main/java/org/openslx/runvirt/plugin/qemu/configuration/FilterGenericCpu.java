package org.openslx.runvirt.plugin.qemu.configuration;

import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.Domain.CpuCheck;
import org.openslx.libvirt.domain.Domain.CpuMode;
import org.openslx.runvirt.configuration.FilterException;
import org.openslx.runvirt.configuration.FilterGeneric;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;

public class FilterGenericCpu extends FilterGeneric<Domain, CommandLineArgs>
{
	private static final String FILTER_NAME = "CPU [number of cores, mode, ...]";

	public FilterGenericCpu()
	{
		super( FilterGenericCpu.FILTER_NAME );
	}

	@Override
	public void filter( Domain config, CommandLineArgs args ) throws FilterException
	{
		config.setVCpu( args.getVmNumCpus() );
		config.setCpuMode( CpuMode.HOST_PASSTHROUGH );
		config.setCpuCheck( CpuCheck.PARTIAL );
	}
}
