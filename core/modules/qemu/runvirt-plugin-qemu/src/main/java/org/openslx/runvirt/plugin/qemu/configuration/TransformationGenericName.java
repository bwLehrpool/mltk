package org.openslx.runvirt.plugin.qemu.configuration;

import org.openslx.libvirt.domain.Domain;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.virtualization.configuration.transformation.TransformationException;
import org.openslx.virtualization.configuration.transformation.TransformationGeneric;

public class TransformationGenericName extends TransformationGeneric<Domain, CommandLineArgs>
{
	private static final String FILTER_NAME = "Name [(display) name]";

	public TransformationGenericName()
	{
		super( TransformationGenericName.FILTER_NAME );
	}

	@Override
	public void transform( Domain config, CommandLineArgs args ) throws TransformationException
	{
		config.setName( args.getVmName() );
		config.setTitle( args.getVmDisplayName() );
	}
}
