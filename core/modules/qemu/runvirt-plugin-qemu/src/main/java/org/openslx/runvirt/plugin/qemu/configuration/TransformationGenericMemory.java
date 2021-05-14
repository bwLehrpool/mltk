package org.openslx.runvirt.plugin.qemu.configuration;

import java.math.BigInteger;

import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.DomainUtils;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.virtualization.configuration.transformation.TransformationException;
import org.openslx.virtualization.configuration.transformation.TransformationGeneric;

/**
 * Generic memory transformation for Libvirt/QEMU virtualization configurations.
 * 
 * @author Manuel Bentele
 * @version 1.0
 */
public class TransformationGenericMemory extends TransformationGeneric<Domain, CommandLineArgs>
{
	/**
	 * Name of the configuration transformation.
	 */
	private static final String NAME = "Memory [normal, current (balloning)]";

	/**
	 * Creates a new memory transformation for Libvirt/QEMU virtualization configurations.
	 */
	public TransformationGenericMemory()
	{
		super( TransformationGenericMemory.NAME );
	}

	/**
	 * Validates a virtualization configuration and input arguments for this transformation.
	 * 
	 * @param config virtualization configuration for the validation.
	 * @param args input arguments for the validation.
	 * @throws TransformationException validation has failed.
	 */
	private void validateInputs( Domain config, CommandLineArgs args ) throws TransformationException
	{
		if ( config == null || args == null ) {
			throw new TransformationException( "Virtualization configuration or input arguments are missing!" );
		} else if ( args.getVmMemory() == null || args.getVmMemory().isEmpty() ) {
			throw new TransformationException( "Amount of memory in MiB is not specified!" );
		}
	}

	@Override
	public void transform( Domain config, CommandLineArgs args ) throws TransformationException
	{
		// validate configuration and input arguments
		this.validateInputs( config, args );

		BigInteger memory = DomainUtils.decodeMemory( args.getVmMemory(), "MiB" );

		config.setMemory( memory );
		config.setCurrentMemory( memory );
	}
}
