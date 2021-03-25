package org.openslx.runvirt.plugin.qemu.configuration;

import java.math.BigInteger;

import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.DomainUtils;
import org.openslx.runvirt.configuration.FilterException;
import org.openslx.runvirt.configuration.FilterGeneric;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;

public class FilterGenericMemory extends FilterGeneric<Domain, CommandLineArgs>
{
	private static final String FILTER_NAME = "Memory [normal, current (balloning)]";

	public FilterGenericMemory()
	{
		super( FilterGenericMemory.FILTER_NAME );
	}

	@Override
	public void filter( Domain config, CommandLineArgs args ) throws FilterException
	{
		BigInteger memory = DomainUtils.decodeMemory( args.getVmMemory(), "MiB" );

		config.setMemory( memory );
		config.setCurrentMemory( memory );
	}
}
