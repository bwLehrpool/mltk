package org.openslx.runvirt.plugin.qemu;

import java.io.File;
import java.io.IOException;
import java.util.Arrays;

import org.apache.logging.log4j.Level;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.core.LoggerContext;
import org.apache.logging.log4j.core.config.Configuration;
import org.apache.logging.log4j.core.config.Configurator;
import org.apache.logging.log4j.core.config.DefaultConfiguration;
import org.apache.logging.log4j.core.config.LoggerConfig;
import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.xml.LibvirtXmlDocumentException;
import org.openslx.libvirt.xml.LibvirtXmlSerializationException;
import org.openslx.libvirt.xml.LibvirtXmlValidationException;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs.CmdLnOption;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgsException;
import org.openslx.runvirt.plugin.qemu.configuration.TransformationGenericCpu;
import org.openslx.runvirt.plugin.qemu.configuration.TransformationGenericDiskCdromDevices;
import org.openslx.runvirt.plugin.qemu.configuration.TransformationGenericDiskFloppyDevices;
import org.openslx.runvirt.plugin.qemu.configuration.TransformationGenericDiskStorageDevices;
import org.openslx.runvirt.plugin.qemu.configuration.TransformationGenericFileSystemDevices;
import org.openslx.runvirt.plugin.qemu.configuration.TransformationGenericInterfaceDevices;
import org.openslx.runvirt.plugin.qemu.configuration.TransformationGenericMemory;
import org.openslx.runvirt.plugin.qemu.configuration.TransformationGenericName;
import org.openslx.runvirt.plugin.qemu.configuration.TransformationGenericParallelDevices;
import org.openslx.runvirt.plugin.qemu.configuration.TransformationGenericUuid;
import org.openslx.runvirt.plugin.qemu.configuration.TransformationGenericWrapperScript;
import org.openslx.runvirt.plugin.qemu.configuration.TransformationSpecificQemuArchitecture;
import org.openslx.runvirt.plugin.qemu.configuration.TransformationSpecificQemuFirmware;
import org.openslx.runvirt.plugin.qemu.configuration.TransformationSpecificQemuGraphics;
import org.openslx.runvirt.plugin.qemu.configuration.TransformationSpecificQemuMdevPassthroughIntel;
import org.openslx.runvirt.plugin.qemu.configuration.TransformationSpecificQemuPciPassthrough;
import org.openslx.runvirt.plugin.qemu.configuration.TransformationSpecificQemuSerialDevices;
import org.openslx.runvirt.plugin.qemu.virtualization.LibvirtHypervisorQemu;
import org.openslx.runvirt.plugin.qemu.virtualization.LibvirtHypervisorQemu.QemuSessionType;
import org.openslx.runvirt.viewer.Viewer;
import org.openslx.runvirt.viewer.ViewerException;
import org.openslx.runvirt.viewer.ViewerLookingGlassClient;
import org.openslx.runvirt.viewer.ViewerVirtManager;
import org.openslx.runvirt.viewer.ViewerVirtViewer;
import org.openslx.runvirt.virtualization.LibvirtHypervisor;
import org.openslx.runvirt.virtualization.LibvirtHypervisorException;
import org.openslx.runvirt.virtualization.LibvirtVirtualMachine;
import org.openslx.runvirt.virtualization.LibvirtVirtualMachineException;
import org.openslx.util.AppUtil;
import org.openslx.virtualization.configuration.transformation.TransformationException;
import org.openslx.virtualization.configuration.transformation.TransformationManager;

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
		Configurator.initialize( new DefaultConfiguration() );

		// parse command line arguments
		CommandLineArgs cmdLn = new CommandLineArgs();

		try {
			cmdLn.parseCmdLnArgs( args );
		} catch ( CommandLineArgsException e ) {
			LOGGER.error( "Parsing of command line arguments failed: " + e.getLocalizedMessage() );
			App.printUsage( cmdLn );
			System.exit( 1 );
		}
		
		if ( cmdLn.isDebugEnabled() || cmdLn.isDebugDevicePassthroughEnabled() ) {
			// Someone please exterminate all Java devs. What's wrong with those fuckheads who come up with this?
			// https://stackoverflow.com/a/65151249/2043481
			LoggerContext ctx = (LoggerContext) LogManager.getContext(App.class.getClassLoader(), false);
			Configuration config = ctx.getConfiguration();
			LoggerConfig loggerConfig = config.getLoggerConfig(LogManager.ROOT_LOGGER_NAME);
			loggerConfig.setLevel(Level.DEBUG);
			ctx.updateLoggers();
		} else {
			LoggerContext ctx = (LoggerContext) LogManager.getContext(App.class.getClassLoader(), false);
			Configuration config = ctx.getConfiguration();
			LoggerConfig loggerConfig = config.getLoggerConfig(LogManager.ROOT_LOGGER_NAME);
			loggerConfig.setLevel(Level.INFO);
			ctx.updateLoggers();
		}

		// show help if 'help' command line option is set
		if ( cmdLn.isHelpAquired() ) {
			App.printUsage( cmdLn );
			System.exit( 0 );
		}

		// log generic application information
		AppUtil.logHeader( LOGGER, APP_NAME, App.class.getPackage().getImplementationVersion() );

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
		} catch ( NullPointerException | LibvirtXmlDocumentException | LibvirtXmlSerializationException
				| LibvirtXmlValidationException e ) {
			LOGGER.error( "Failed to read VM input configuration file: " + e.getLocalizedMessage() );
			hypervisor.close();
			System.exit( 3 );
		}
		
		// Validation run?
		if ( cmdLn.isValidateEnabled() ) {
			try {
				config.validateXml();
			} catch ( LibvirtXmlValidationException e ) {
				// TODO Auto-generated catch block
				e.printStackTrace();
				System.exit( 42 );
			}
			System.exit( 0 );
		}

		// create transformation manager to finalize VM configuration
		final TransformationManager<Domain, CommandLineArgs> transformationManager;
		transformationManager = new TransformationManager<Domain, CommandLineArgs>( config, cmdLn );

		// register necessary transformations to finalize configuration template
		transformationManager.register( new TransformationGenericName(), true );
		transformationManager.register( new TransformationGenericUuid(), true );
		transformationManager.register( new TransformationGenericCpu(), true );
		transformationManager.register( new TransformationGenericMemory(), true );
		transformationManager.register( new TransformationGenericDiskStorageDevices(), true );
		transformationManager.register( new TransformationGenericDiskCdromDevices(), true );
		transformationManager.register( new TransformationGenericDiskFloppyDevices(), true );
		transformationManager.register( new TransformationGenericInterfaceDevices(), true );
		transformationManager.register( new TransformationGenericParallelDevices(), true );
		transformationManager.register( new TransformationGenericFileSystemDevices(), true );

		// register QEMU specific transformations to finalize configuration template
		if ( hypervisor instanceof LibvirtHypervisorQemu ) {
			final LibvirtHypervisorQemu hypervisorQemu = LibvirtHypervisorQemu.class.cast( hypervisor );

			transformationManager.register( new TransformationSpecificQemuArchitecture( hypervisorQemu ), true );
			transformationManager.register( new TransformationSpecificQemuFirmware( hypervisorQemu ), true );
			transformationManager.register( new TransformationSpecificQemuGraphics( hypervisorQemu ), true );
			transformationManager.register( new TransformationSpecificQemuSerialDevices( hypervisorQemu ), true );
			transformationManager.register( new TransformationSpecificQemuMdevPassthroughIntel( hypervisorQemu ), false );
			transformationManager.register( new TransformationSpecificQemuPciPassthrough( hypervisorQemu ), false );
		}

		// Needs to be last one since TransformationSpecificQemuArchitecture sets this too
		transformationManager.register( new TransformationGenericWrapperScript(), true );

		// finalize Libvirt VM configuration template
		try {
			transformationManager.transform();
		} catch ( TransformationException e ) {
			LOGGER.error( "Failed to transform VM configuration file", e );
			hypervisor.close();
			System.exit( 4 );
		}

		// spawn xml editor on final xml if desired
		if ( cmdLn.isXmlEditorSpawningEnabled() ) {
			try {
				File tmp = File.createTempFile( "run-virt-qemu", ".xml" );
				boolean ok = false;
				while ( !ok ) {
					config.toXml( tmp );
					LOGGER.info( "Opening text editor for XML" );
					EditorRunner.open( tmp.getAbsolutePath() );
					try {
						Domain nc = new Domain( tmp );
						ok = true;
						config = nc;
					} catch ( LibvirtXmlSerializationException | LibvirtXmlDocumentException | LibvirtXmlValidationException e ) {
						LOGGER.error( "Failed to create Domain from edited XML file", e );
					}
				}
			} catch ( IOException e ) {
				LOGGER.error( "Failed to create temp file for XML editing: " + e.getLocalizedMessage() );
			} catch ( LibvirtXmlSerializationException e ) {
				LOGGER.error( "Failed to write VM output temporary file" + e );
			}
		}

		// write finalized configuration to file if output file is specified
		final String xmlOutputFileName = cmdLn.getVmCfgOutFileName();
		if ( xmlOutputFileName != null && !xmlOutputFileName.isEmpty() ) {
			try {
				final File xmlOutputFile = new File( xmlOutputFileName );
				config.toXml( xmlOutputFile );
			} catch ( NullPointerException | LibvirtXmlSerializationException e ) {
				// We only call this for debugging purposes, don't bail out but log
				LOGGER.error( "Failed to write VM output configuration file: " + e.getLocalizedMessage() );
			}
		}

		// define Libvirt VM from finalized configuration
		LibvirtVirtualMachine vm = null;
		try {
			vm = hypervisor.registerVm( config );
		} catch ( LibvirtHypervisorException e ) {
			LOGGER.error( "Failed to define VM from configuration file: " + e.getLocalizedMessage() );
			hypervisor.close();
			System.exit( 6 );
		}

		// start defined Libvirt VM
		try {
			vm.start();
		} catch ( LibvirtVirtualMachineException e ) {
			LOGGER.error( "Failed to start defined VM: " + e.getLocalizedMessage() );
			hypervisor.close();
			System.exit( 7 );
		}

		// create specific viewer to display Libvirt VM
		final Viewer vmViewer;
		if ( cmdLn.isNvidiaGpuPassthroughEnabled() && !cmdLn.isDebugDevicePassthroughEnabled() ) {
			// viewer for GPU passthrough (framebuffer access) is required
			vmViewer = new ViewerLookingGlassClient( vm, hypervisor, cmdLn.isDebugEnabled() );
		} else {
			// viewer for non-GPU passthrough (no framebuffer access) is required
			if ( cmdLn.isDebugEnabled() || cmdLn.isManagerEnabled() ) {
				// create specific Virtual Machine Manager viewer if debug mode is enabled
				vmViewer = new ViewerVirtManager( vm, hypervisor );
			} else {
				// create Virtual Viewer if debug mode is disabled
				vmViewer = new ViewerVirtViewer( vm, hypervisor );
			}
		}

		// display Libvirt VM with the specific viewer on the screen
		try {
			vmViewer.display();
		} catch ( ViewerException e ) {
			LOGGER.error( "Failed to display VM: " + e.getLocalizedMessage() );
			hypervisor.close();
			System.exit( 8 );
		}

		// stop VM if VM isn't turned off already (or if VM viewer) died
		try {
			vm.stop();
		} catch ( LibvirtVirtualMachineException e ) {
			LOGGER.error( "Failed to shutdown orphaned VM: " + e.getLocalizedMessage() );
			hypervisor.close();
			System.exit( 9 );
		}

		// undefine VM after usage
		try {
			hypervisor.deregisterVm( vm );
		} catch ( LibvirtHypervisorException | LibvirtVirtualMachineException e ) {
			LOGGER.error( "Failed to undefine VM: " + e.getLocalizedMessage() );
			hypervisor.close();
			System.exit( 10 );
		}

		// close connection to hypervisor
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
			String[] longOptionArguments;

			// only request and log argument if option has an command line argument
			if ( option.getNumArguments() > 0 ) {
				longOptionArguments = cmdLn.getArguments( option );

				if ( longOptionArguments == null ) {
					longOptionArguments = new String[] { "no argument specified" };
				}
			} else {
				longOptionArguments = new String[] { "option has no argument" };
			}

			LOGGER.debug( "\t" + paddedLongOption + ": " + Arrays.toString( longOptionArguments ) );
		}
	}
}
