package org.openslx.runvirt.plugin.qemu.configuration;

import org.openslx.libvirt.domain.Domain;
import org.openslx.runvirt.configuration.FilterException;
import org.openslx.runvirt.configuration.FilterGeneric;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;

public class FilterGenericName extends FilterGeneric<Domain, CommandLineArgs>
{
	private static final String FILTER_NAME = "Name [(display) name]";

	public FilterGenericName()
	{
		super( FilterGenericName.FILTER_NAME );
	}

	@Override
	public void filter( Domain config, CommandLineArgs args ) throws FilterException
	{
		config.setName( args.getVmName() );
		config.setTitle( args.getVmDisplayName() );
	}
}
