package org.openslx.runvirt.plugin.qemu.configuration;

import org.openslx.libvirt.domain.Domain;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.virtualization.configuration.transformation.TransformationException;
import org.openslx.virtualization.configuration.transformation.TransformationGeneric;

public class TransformationGenericUuid extends TransformationGeneric<Domain, CommandLineArgs>
{
	private static final String FILTER_NAME = "UUID";

	public TransformationGenericUuid()
	{
		super( TransformationGenericUuid.FILTER_NAME );
	}

	@Override
	public void transform( Domain config, CommandLineArgs args ) throws TransformationException
	{
		config.setUuid( args.getVmUuid() );
	}
}
