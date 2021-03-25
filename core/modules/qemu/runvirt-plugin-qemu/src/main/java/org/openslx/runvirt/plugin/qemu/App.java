package org.openslx.runvirt.plugin.qemu;

import java.io.File;

import org.apache.log4j.BasicConfigurator;
import org.apache.log4j.LogManager;
import org.apache.log4j.Logger;
import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.xml.LibvirtXmlDocumentException;
import org.openslx.libvirt.xml.LibvirtXmlSerializationException;
import org.openslx.libvirt.xml.LibvirtXmlValidationException;
import org.openslx.runvirt.configuration.FilterException;
import org.openslx.runvirt.configuration.FilterManager;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs.CmdLnOption;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgsException;
import org.openslx.runvirt.plugin.qemu.configuration.FilterGenericCpu;
import org.openslx.runvirt.plugin.qemu.configuration.FilterGenericDiskCdromDevices;
import org.openslx.runvirt.plugin.qemu.configuration.FilterGenericDiskFloppyDevices;
import org.openslx.runvirt.plugin.qemu.configuration.FilterGenericDiskStorageDevices;
import org.openslx.runvirt.plugin.qemu.configuration.FilterGenericFileSystemDevices;
import org.openslx.runvirt.plugin.qemu.configuration.FilterGenericInterfaceDevices;
import org.openslx.runvirt.plugin.qemu.configuration.FilterGenericMemory;
import org.openslx.runvirt.plugin.qemu.configuration.FilterGenericName;
import org.openslx.runvirt.plugin.qemu.configuration.FilterGenericParallelDevices;
import org.openslx.runvirt.plugin.qemu.configuration.FilterSpecificQemuSerialDevices;
import org.openslx.runvirt.plugin.qemu.configuration.FilterGenericUuid;
import org.openslx.runvirt.plugin.qemu.configuration.FilterSpecificQemuArchitecture;
import org.openslx.runvirt.plugin.qemu.configuration.FilterSpecificQemuNvidiaGpuPassthrough;
import org.openslx.runvirt.plugin.qemu.virtualization.LibvirtHypervisorQemu;
import org.openslx.runvirt.plugin.qemu.virtualization.LibvirtHypervisorQemu.QemuSessionType;
import org.openslx.runvirt.virtualization.LibvirtHypervisor;
import org.openslx.runvirt.virtualization.LibvirtHypervisorException;
import org.openslx.runvirt.virtualization.LibvirtVirtualMachine;
import org.openslx.runvirt.virtualization.LibvirtVirtualMachineException;

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
	public static final String APP_NAME = "run-virt QEMU plugin";

	/**
	 * Stores description of the run-virt QEMU plugin (command line tool).
	 */
	public static final String APP_DESC = "Finalize a Libvirt VM (domain XML) configuration and manage the VM.";

	/**
	 * Stores additional information for the run-virt QEMU plugin (command line tool).
	 */
	public static final String APP_INFO = "The " + APP_NAME + " is part of the bwLehrpool infrastructure.";

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

		// create connection to the QEMU hypervisor via Libvirt
		LibvirtHypervisor hypervisor = null;
		try {
			hypervisor = new LibvirtHypervisorQemu( QemuSessionType.LOCAL_USER_SESSION );
		} catch ( LibvirtHypervisorException e ) {
			LOGGER.error( "Failed to connect to the QEMU virtualizer (Libvirt daemon): " + e.getLocalizedMessage() );
			System.exit( 2 );
		}

		// read Libvirt XML domain configuration template
		final String xmlInputFileName = cmdLn.getVmCfgInpFileName();
		Domain config = null;
		try {
			final File xmlInputFile = new File( xmlInputFileName );
			config = new Domain( xmlInputFile );
		} catch ( LibvirtXmlDocumentException | LibvirtXmlSerializationException | LibvirtXmlValidationException e ) {
			LOGGER.error( "Failed to read VM input configuration file: " + e.getLocalizedMessage() );
			hypervisor.close();
			System.exit( 3 );
		}

		// create filter manager to finalize VM configuration
		final FilterManager<Domain, CommandLineArgs> filterManager;
		filterManager = new FilterManager<Domain, CommandLineArgs>( config, cmdLn );

		// register necessary filters to finalize configuration template
		filterManager.register( new FilterGenericName(), true );
		filterManager.register( new FilterGenericUuid(), true );
		filterManager.register( new FilterGenericCpu(), true );
		filterManager.register( new FilterGenericMemory(), true );
		filterManager.register( new FilterGenericDiskStorageDevices(), true );
		filterManager.register( new FilterGenericDiskCdromDevices(), true );
		filterManager.register( new FilterGenericDiskFloppyDevices(), true );
		filterManager.register( new FilterGenericInterfaceDevices(), true );
		filterManager.register( new FilterGenericParallelDevices(), true );
		filterManager.register( new FilterGenericFileSystemDevices(), true );

		// register QEMU specific filters to finalize configuration template
		if ( hypervisor instanceof LibvirtHypervisorQemu ) {
			final LibvirtHypervisorQemu hypervisorQemu = LibvirtHypervisorQemu.class.cast( hypervisor );

			filterManager.register( new FilterSpecificQemuArchitecture( hypervisorQemu ), true );
			filterManager.register( new FilterSpecificQemuSerialDevices( hypervisorQemu ), true );
			filterManager.register( new FilterSpecificQemuNvidiaGpuPassthrough( hypervisorQemu ), false );
		}

		// finalize Libvirt VM configuration template
		try {
			filterManager.filterAll();
		} catch ( FilterException e ) {
			LOGGER.error( "Failed to finalize VM configuration file: " + e.getLocalizedMessage() );
			hypervisor.close();
			System.exit( 4 );
		}

		// write finalized configuration to file if output file is specified
		final String xmlOutputFileName = cmdLn.getVmCfgOutFileName();
		if ( xmlOutputFileName != null && !xmlOutputFileName.isEmpty() ) {
			try {
				final File xmlOutputFile = new File( xmlOutputFileName );
				config.toXml( xmlOutputFile );
			} catch ( LibvirtXmlSerializationException e ) {
				LOGGER.error( "Failed to write VM output configuration file: " + e.getLocalizedMessage() );
				hypervisor.close();
				System.exit( 5 );
			}
		}

		// define QEMU VM from finalized configuration
		LibvirtVirtualMachine vm = null;
		try {
			vm = hypervisor.registerVm( config );
		} catch ( LibvirtHypervisorException e ) {
			LOGGER.error( "Failed to define VM from configuration file: " + e.getLocalizedMessage() );
			hypervisor.close();
			System.exit( 6 );
		}

		try {
			vm.start();
		} catch ( LibvirtVirtualMachineException e ) {
			LOGGER.error( "Failed to start defined VM: " + e.getLocalizedMessage() );
			try {
				hypervisor.deregisterVm( vm );
			} catch ( LibvirtHypervisorException | LibvirtVirtualMachineException e1 ) {
				LOGGER.error( "Failed to undefine VM: " + e.getLocalizedMessage() );
			}
			hypervisor.close();
			System.exit( 7 );
		}

		// close connection and let VM be running
		hypervisor.close();

		// return with successful exit code
		System.exit( 0 );
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
