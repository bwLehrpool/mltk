package org.openslx.runvirt.plugin.qemu.configuration;

import java.util.Iterator;
import java.util.UUID;

import org.openslx.libvirt.domain.Domain;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.util.Util;
import org.openslx.virtualization.configuration.transformation.TransformationException;
import org.openslx.virtualization.configuration.transformation.TransformationGeneric;

/**
 * Generic UUID transformation for Libvirt/QEMU virtualization configurations.
 * 
 * @author Manuel Bentele
 * @version 1.0
 */
public class TransformationGenericUuid extends TransformationGeneric<Domain, CommandLineArgs>
{
	/**
	 * Name of the configuration transformation.
	 */
	private static final String NAME = "UUID";

	/**
	 * Creates a new UUID transformation for Libvirt/QEMU virtualization configurations.
	 */
	public TransformationGenericUuid()
	{
		super( TransformationGenericUuid.NAME );
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
		}
	}

	@Override
	public void transform( Domain config, CommandLineArgs args ) throws TransformationException
	{
		// validate configuration and input arguments
		this.validateInputs( config, args );

		// Get UUID which the user (most likely) uploaded the VM with
		String originalUuid = config.getUuid();

		// Check if there is a custom system UUID set and if not, make the original
		// libvirt uuid the system UUID, as this is what libvirt does by default
		// This is qemu specific but won't do anything for other hypervisors
		String systemUuid = null;
		for ( Iterator<String> it = config.getQemuCmdlnArguments().iterator(); it.hasNext(); ) {
			String arg = it.next();
			if ( "-uuid".equals( arg ) ) {
				if ( it.hasNext() ) {
					systemUuid = it.next();
				} else {
					it.remove();
				}
			}
		}
		if ( Util.isEmptyString( systemUuid ) ) {
			// No system-uuid on command line, use old libvirt uuid
			config.addQemuCmdlnArgument( "-uuid" );
			config.addQemuCmdlnArgument( originalUuid );
		}

		// Finally, apply new uuid, either from our cmdline, or random
		if ( !Util.isEmptyString( args.getVmUuid() ) ) {
			config.setUuid( args.getVmUuid() );
		} else {
			config.setUuid( UUID.randomUUID().toString() );
		}
	}
}
