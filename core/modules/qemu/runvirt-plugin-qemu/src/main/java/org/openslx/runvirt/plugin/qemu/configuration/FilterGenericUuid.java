package org.openslx.runvirt.plugin.qemu.configuration;

import org.openslx.libvirt.domain.Domain;
import org.openslx.runvirt.configuration.FilterException;
import org.openslx.runvirt.configuration.FilterGeneric;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;

public class FilterGenericUuid extends FilterGeneric<Domain, CommandLineArgs>
{
	private static final String FILTER_NAME = "UUID";

	public FilterGenericUuid()
	{
		super( FilterGenericUuid.FILTER_NAME );
	}

	@Override
	public void filter( Domain config, CommandLineArgs args ) throws FilterException
	{
		config.setUuid( args.getVmUuid() );
	}
}
