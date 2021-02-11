package org.openslx.runvirt.plugin.qemu;

import org.apache.log4j.BasicConfigurator;
import org.apache.log4j.LogManager;
import org.apache.log4j.Logger;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs.CmdLnOption;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgsException;

/**
 * Run-virt QEMU plugin (command line tool) to finalize a Libvirt domain XML configuration.
 * 
 * @author Manuel Bentele
 * @version 1.0
 */
public class App
{
	/**
	 * Stores name of the run-virt QEMU plugin (command line tool).
	 */
	private static final String APP_NAME = "run-virt QEMU plugin";

	/**
	 * Stores description of the run-virt QEMU plugin (command line tool).
	 */
	private static final String APP_DESC = "Finalize a Libvirt VM (domain XML) configuration and manage the VM.";

	/**
	 * Stores additional information for the run-virt QEMU plugin (command line tool).
	 */
	private static final String APP_INFO = "The " + APP_NAME + " is part of the bwLehrpool infrastructure.";

	/**
	 * Instance of a logger to log messages.
	 */
	private static final Logger LOGGER = LogManager.getLogger( App.class );

	/**
	 * Entry point of the run-virt QEMU plugin (command line tool).
	 * 
	 * @param args command line arguments passed to the run-virt QEMU plugin (command line tool).
	 */
	public static void main( String[] args )
	{
		// initialize logging
		BasicConfigurator.configure();

		// parse command line arguments
		CommandLineArgs cmdLn = new CommandLineArgs();

		try {
			cmdLn.parseCmdLnArgs( args );
		} catch ( CommandLineArgsException e ) {
			LOGGER.error( "Parsing of command line arguments failed: " + e.getLocalizedMessage() );
			App.printUsage( cmdLn );
			System.exit( 1 );
		}

		// show help if 'help' command line option is set
		if ( cmdLn.isHelpAquired() ) {
			App.printUsage( cmdLn );
			System.exit( 0 );
		}

		// print command line arguments for debugging purposes
		App.printCmdLnArgs( cmdLn );
	}

	/**
	 * Helper utility to print the run-virt QEMU plugin help text.
	 * 
	 * @param cmdLn parsed command line arguments.
	 */
	public static void printUsage( CommandLineArgs cmdLn )
	{
		final String newLine = System.lineSeparator();
		final String fullAppDesc = newLine + App.APP_DESC + newLine + newLine;
		final String fullAppInfo = newLine + App.APP_INFO;

		cmdLn.printHelp( App.APP_NAME, fullAppDesc, fullAppInfo );
	}

	/**
	 * Helper utility to log the run-virt QEMU plugin's submitted command line arguments.
	 * 
	 * @param cmdLn parsed command line arguments.
	 * 
	 * @implNote This method is intended to be used for debugging purposes.
	 */
	public static void printCmdLnArgs( CommandLineArgs cmdLn )
	{
		// determine length of longest command line option
		int longOptionLengthMax = 0;

		for ( CmdLnOption option : CmdLnOption.values() ) {
			// store length of current long option
			final int longOptionLength = option.getLongOption().length();

			// if length is longer than every length before, store this length as longest length
			if ( longOptionLength > longOptionLengthMax ) {
				longOptionLengthMax = longOptionLength;
			}
		}

		LOGGER.debug( "Command line arguments: --------------" );

		for ( CmdLnOption option : CmdLnOption.values() ) {
			final String paddedLongOption = String.format( "%-" + longOptionLengthMax + "s", option.getLongOption() );
			final String longOptionArgument;

			// only request and log argument if option has an command line argument
			if ( option.hasArgument() ) {
				longOptionArgument = cmdLn.getArgument( option );
			} else {
				longOptionArgument = new String( "[option has no argument]" );
			}

			LOGGER.debug( "\t" + paddedLongOption + ": " + longOptionArgument );
		}
	}
}
