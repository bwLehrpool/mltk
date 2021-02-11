package org.openslx.runvirt.plugin.qemu.cmdln;

/**
 * An exception during the parsing of command line arguments.
 * 
 * @author Manuel Bentele
 * @version 1.0
 */
public class CommandLineArgsException extends Exception
{
	/**
	 * Version number for serialization.
	 */
	private static final long serialVersionUID = 8371924151602194406L;

	/**
	 * Creates an command line argument parsing exception including an error message.
	 * 
	 * @param errorMsg message to describe a specific parsing error.
	 */
	public CommandLineArgsException( String errorMsg )
	{
		super( errorMsg );
	}
}
