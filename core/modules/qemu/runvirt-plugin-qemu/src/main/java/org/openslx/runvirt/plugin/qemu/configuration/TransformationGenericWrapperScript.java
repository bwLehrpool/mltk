package org.openslx.runvirt.plugin.qemu.configuration;

import java.io.File;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openslx.libvirt.domain.Domain;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.util.Util;
import org.openslx.virtualization.configuration.transformation.TransformationException;
import org.openslx.virtualization.configuration.transformation.TransformationGeneric;

/**
 * Use openslx wrapper script for launching the emulator (virtualizer)
 * 
 * @author sr
 */
public class TransformationGenericWrapperScript extends TransformationGeneric<Domain, CommandLineArgs>
{
	/**
	 * Instance of a logger to log messages.
	 */
	private static final Logger LOGGER = LogManager.getLogger( TransformationGenericWrapperScript.class );

	/**
	 * Name of the configuration transformation.
	 */
	private static final String NAME = "Use wrapper script for emulator if found";

	/**
	 * Extension for the wrapper we look for
	 */
	private static final String WRAPPER_EXT = ".openslx";

	public TransformationGenericWrapperScript()
	{
		super( TransformationGenericWrapperScript.NAME );
	}

	/**
	 * Validates a virtualization configuration and input arguments for this transformation.
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

		// Use our wrapper if it exists
		String emu = config.getDevicesEmulator();
		if ( !Util.isEmptyString( emu ) && new File( emu + WRAPPER_EXT ).canExecute() ) {
			LOGGER.info( "Using emulator wrapper " + emu + WRAPPER_EXT );
			config.setDevicesEmulator( emu + WRAPPER_EXT );
		}
	}
}
