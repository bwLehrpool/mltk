package org.openslx.runvirt.plugin.qemu.configuration;

import java.math.BigInteger;

import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.DomainUtils;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.virtualization.configuration.transformation.TransformationException;
import org.openslx.virtualization.configuration.transformation.TransformationGeneric;

public class TransformationGenericMemory extends TransformationGeneric<Domain, CommandLineArgs>
{
	private static final String FILTER_NAME = "Memory [normal, current (balloning)]";

	public TransformationGenericMemory()
	{
		super( TransformationGenericMemory.FILTER_NAME );
	}

	@Override
	public void transform( Domain config, CommandLineArgs args ) throws TransformationException
	{
		BigInteger memory = DomainUtils.decodeMemory( args.getVmMemory(), "MiB" );

		config.setMemory( memory );
		config.setCurrentMemory( memory );
	}
}
