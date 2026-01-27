package org.openslx.runvirt.plugin.qemu.cmdln;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.CommandLineParser;
import org.apache.commons.cli.DefaultParser;
import org.apache.commons.cli.HelpFormatter;
import org.apache.commons.cli.Option;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openslx.libvirt.domain.device.HostdevPciDeviceDescription;
import org.openslx.runvirt.plugin.qemu.configuration.TransformationSpecificQemuPciPassthrough;
import org.openslx.util.Util;

/**
 * Command line argument parser for the run-virt QEMU plugin (command line tool).
 * 
 * @author Manuel Bentele
 * @version 1.0
 */
public class CommandLineArgs
{
	/**
	 * Instance of a logger to log messages.
	 */
	private static final Logger LOGGER = LogManager.getLogger( CommandLineArgs.class );

	/**
	 * Parser for parsing command line arguments.
	 */
	private CommandLineParser cmdLnParser = null;

	/**
	 * Stores specified command line options.
	 */
	private Options cmdLnOptions = null;

	/**
	 * Stores the parsed command line arguments.
	 */
	private CommandLine cmdLn = null;

	/**
	 * Creates a new command line argument parser for the run-virt QEMU plugin.
	 * 
	 * @implNote Please call {@link CommandLineArgs#parseCmdLnArgs(String[])} manually after
	 *           obtaining the command line argument parser from this method.
	 */
	public CommandLineArgs()
	{
		this.createCmdLnParser();
		this.createCmdLnOptions();
	}

	/**
	 * Creates a new command line argument parser for the run-virt QEMU plugin and parses the command
	 * line arguments.
	 * 
	 * @param args command line arguments submitted to the application.
	 * 
	 * @throws CommandLineArgsException parsing of command line arguments failed.
	 */
	public CommandLineArgs( String[] args ) throws CommandLineArgsException
	{
		this();
		this.parseCmdLnArgs( args );
	}

	/**
	 * Creates a new parser and empty command line options for parsing command line arguments.
	 */
	private void createCmdLnParser()
	{
		this.cmdLnParser = new DefaultParser();
		this.cmdLnOptions = new Options();
	}

	/**
	 * Creates command line options specified by {@link CmdLnOption}.
	 */
	private void createCmdLnOptions()
	{
		for ( CmdLnOption option : CmdLnOption.values() ) {
			final Option cmdlnOption;

			final boolean hasArg = ( option.getNumArguments() > 0 ) ? true : false;
			cmdlnOption = new Option( option.getShortOption(), option.getLongOption(), hasArg, option.getDescription() );
			cmdlnOption.setValueSeparator( ',' );
			cmdlnOption.setArgs( option.getNumArguments() );

			this.cmdLnOptions.addOption( cmdlnOption );
		}
	}

	/**
	 * Prints command line help for the current application.
	 * 
	 * @param appName name of the current application.
	 * @param header header for the command line help.
	 * @param footer footer for the command line help.
	 */
	public void printHelp( String appName, String header, String footer )
	{
		HelpFormatter formatter = new HelpFormatter();
		formatter.setLeftPadding( 2 );
		formatter.printHelp( appName, header, this.cmdLnOptions, footer, true );
	}

	/**
	 * Parses command line arguments from a given argument {@link String}.
	 * 
	 * @param args command line arguments submitted to the application.
	 * 
	 * @throws CommandLineArgsException parsing of command line arguments failed.
	 */
	public void parseCmdLnArgs( String[] args ) throws CommandLineArgsException
	{
		try {
			this.cmdLn = this.cmdLnParser.parse( this.cmdLnOptions, args );
		} catch ( ParseException e ) {
			throw new CommandLineArgsException( e.getLocalizedMessage() );
		}
	}

	/**
	 * Returns the parsed argument of the specified command line option.
	 * 
	 * @param cmdLnOption command line option for that the parsed argument should be returned.
	 * @return parsed argument of the command line option.
	 */
	public String getArgument( CmdLnOption cmdLnOption )
	{
		return this.cmdLn.getOptionValue( cmdLnOption.getShortOption() );
	}

	/**
	 * Returns the parsed arguments of the specified command line option.
	 * 
	 * @param cmdLnOption command line option for that the parsed arguments should be returned.
	 * @return parsed argument of the command line option.
	 */
	public String[] getArguments( CmdLnOption cmdLnOption )
	{
		return this.cmdLn.getOptionValues( cmdLnOption.getShortOption() );
	}

	/**
	 * Returns the presence of the command line option {@link CmdLnOption#HELP}.
	 * 
	 * @return presence of the command line option {@link CmdLnOption#HELP}.
	 */
	public boolean isHelpAquired()
	{
		return this.cmdLn.hasOption( CmdLnOption.HELP.getShortOption() );
	}

	/**
	 * Returns the state of the command line option {@link CmdLnOption#DEBUG}.
	 * 
	 * @return state of the command line option {@link CmdLnOption#DEBUG}.
	 */
	public boolean isDebugEnabled()
	{
		final String debugArg = this.getArgument( CmdLnOption.DEBUG );
		return ( "true".equals( debugArg ) ) ? true : false;
	}

	/**
	 * Returns the presence of the command line option {@link CmdLnOption#MANAGER}.
	 * 
	 * @return presence of the command line option {@link CmdLnOption#MANAGER}.
	 */
	public boolean isManagerEnabled()
	{
		return this.cmdLn.hasOption( CmdLnOption.MANAGER.getShortOption() );
	}
	
	public boolean isValidateEnabled()
	{
		return this.cmdLn.hasOption( CmdLnOption.VALIDATE.getShortOption() );
	}
	
	public boolean isPersistentModeEnabled()
	{
		return this.cmdLn.hasOption( CmdLnOption.PERSISTENT.getShortOption() );
	}

	/**
	 * Returns the state of the command line option {@link CmdLnOption#DEBUG_PTH}.
	 * 
	 * @return state of the command line option {@link CmdLnOption#DEBUG_PTH}.
	 */
	public boolean isDebugDevicePassthroughEnabled()
	{
		final String debugArg = this.getArgument( CmdLnOption.DEBUG_PTH );
		return ( "true".equals( debugArg ) ) ? true : false;
	}

	/**
	 * Returns the argument of the command line option {@link CmdLnOption#FIRMWARE}.
	 * 
	 * @return argument of the command line option {@link CmdLnOption#FIRMWARE}.
	 */
	public String getFirmware()
	{
		return this.getArgument( CmdLnOption.FIRMWARE );
	}

	/**
	 * Returns the argument of the command line option {@link CmdLnOption#VM_CFGINP}.
	 * 
	 * @return argument of the command line option {@link CmdLnOption#VM_CFGINP}.
	 */
	public String getVmCfgInpFileName()
	{
		return this.getArgument( CmdLnOption.VM_CFGINP );
	}

	/**
	 * Returns the argument of the command line option {@link CmdLnOption#VM_CFGOUT}.
	 * 
	 * @return argument of the command line option {@link CmdLnOption#VM_CFGOUT}.
	 */
	public String getVmCfgOutFileName()
	{
		return this.getArgument( CmdLnOption.VM_CFGOUT );
	}

	/**
	 * Returns the argument of the command line option {@link CmdLnOption#VM_NAME}.
	 * 
	 * @return argument of the command line option {@link CmdLnOption#VM_NAME}.
	 */
	public String getVmName()
	{
		return this.getArgument( CmdLnOption.VM_NAME );
	}

	/**
	 * Returns the argument of the command line option {@link CmdLnOption#VM_UUID}.
	 * 
	 * @return argument of the command line option {@link CmdLnOption#VM_UUID}.
	 */
	public String getVmUuid()
	{
		return this.getArgument( CmdLnOption.VM_UUID );
	}

	/**
	 * Returns the argument of the command line option {@link CmdLnOption#VM_DSPLNAME}.
	 * 
	 * @return argument of the command line option {@link CmdLnOption#VM_DSPLNAME}.
	 */
	public String getVmDisplayName()
	{
		return this.getArgument( CmdLnOption.VM_DSPLNAME );
	}

	/**
	 * Returns the argument of the command line option {@link CmdLnOption#VM_OS}.
	 * 
	 * @return argument of the command line option {@link CmdLnOption#VM_OS}.
	 */
	public String getVmOperatingSystem()
	{
		return this.getArgument( CmdLnOption.VM_OS );
	}

	/**
	 * Returns the argument of the command line option {@link CmdLnOption#VM_NCPUS}.
	 * 
	 * @return argument of the command line option {@link CmdLnOption#VM_NCPUS}.
	 */
	public int getVmNumCpus()
	{
		final String numCpuArg = this.getArgument( CmdLnOption.VM_NCPUS );
		int numCpus = 0;

		if ( numCpuArg != null ) {
			numCpus = Integer.parseInt( numCpuArg );
		}

		return numCpus;
	}

	public List<List<Integer>> getCpuTopology()
	{
		String arg = this.getArgument( CmdLnOption.VM_CPU_TOPO );
		if ( Util.isEmptyString( arg ) )
			return null;
		String[] scores = arg.split( ";" );
		List<List<Integer>> retval = new ArrayList<>( scores.length );
		for ( int c = 0; c < scores.length; ++c ) {
			if ( Util.isEmptyString( scores[c] ) ) {
				LOGGER.warn( "Could not parse CPU topology: empty group element" );
				return null;
			}
			String[] coreThreads = scores[c].split( "," );
			ArrayList<Integer> current = new ArrayList<>();
			retval.add( current );
			for ( int t = 0; t < coreThreads.length; ++t ) {
				int from, to;
				String[] fromTo = coreThreads[t].split( "-" );
				if ( fromTo.length == 0 || fromTo.length > 2
						|| Util.isEmptyString( fromTo[0] ) || ( fromTo.length > 1 && Util.isEmptyString( fromTo[1] ) ) ) {
					LOGGER.warn( "Could not parse CPU topology: empty or malformed sibling element '" + coreThreads[t] + "'" );
					return null;
				}
				from = Util.parseInt( fromTo[0], -1 );
				to = fromTo.length == 2 ? Util.parseInt( fromTo[1], -1 ) : from;
				if ( from == -1 || to == -1 || to < from ) {
					LOGGER.warn( "Could not parse CPU topology sibling number '" + coreThreads[t] + "' from '" + scores[c] + "'" );
					return null;
				}
				for ( int i = from; i <= to; ++i ) {
					current.add( i );
				}
			}
		}
		return retval;
	}

	/**
	 * Returns the argument of the command line option {@link CmdLnOption#VM_MEM}.
	 * 
	 * @return argument of the command line option {@link CmdLnOption#VM_MEM}.
	 */
	public String getVmMemory()
	{
		return this.getArgument( CmdLnOption.VM_MEM );
	}

	/**
	 * Returns the argument of the command line option {@link CmdLnOption#VM_HDD0}.
	 * 
	 * @return argument of the command line option {@link CmdLnOption#VM_HDD0}.
	 */
	public String getVmDiskFileNameHDD0()
	{
		return this.getArgument( CmdLnOption.VM_HDD0 );
	}

	/**
	 * Returns the argument of the command line option {@link CmdLnOption#VM_FLOPPY0}.
	 * 
	 * @return argument of the command line option {@link CmdLnOption#VM_FLOPPY0}.
	 */
	public String getVmDiskFileNameFloppy0()
	{
		return this.getArgument( CmdLnOption.VM_FLOPPY0 );
	}

	/**
	 * Returns the argument of the command line option {@link CmdLnOption#VM_FLOPPY1}.
	 * 
	 * @return argument of the command line option {@link CmdLnOption#VM_FLOPPY1}.
	 */
	public String getVmDiskFileNameFloppy1()
	{
		return this.getArgument( CmdLnOption.VM_FLOPPY1 );
	}

	/**
	 * Returns the argument of the command line option {@link CmdLnOption#VM_CDROM0}.
	 * 
	 * @return argument of the command line option {@link CmdLnOption#VM_CDROM0}.
	 */
	public String getVmDiskFileNameCdrom0()
	{
		return this.getArgument( CmdLnOption.VM_CDROM0 );
	}

	/**
	 * Returns the argument of the command line option {@link CmdLnOption#VM_CDROM1}.
	 * 
	 * @return argument of the command line option {@link CmdLnOption#VM_CDROM1}.
	 */
	public String getVmDiskFileNameCdrom1()
	{
		return this.getArgument( CmdLnOption.VM_CDROM1 );
	}

	/**
	 * Returns the argument of the command line option {@link CmdLnOption#VM_PARALLEL0}.
	 * 
	 * @return argument of the command line option {@link CmdLnOption#VM_SERIAL0}.
	 */
	public String getVmDeviceParallel0()
	{
		return this.getArgument( CmdLnOption.VM_PARALLEL0 );
	}

	/**
	 * Returns the argument of the command line option {@link CmdLnOption#VM_SERIAL0}.
	 * 
	 * @return argument of the command line option {@link CmdLnOption#VM_SERIAL0}.
	 */
	public String getVmDeviceSerial0()
	{
		return this.getArgument( CmdLnOption.VM_SERIAL0 );
	}

	/**
	 * Returns the argument of the command line option {@link CmdLnOption#VM_MAC0}.
	 * 
	 * @return argument of the command line option {@link CmdLnOption#VM_MAC0}.
	 */
	public String getVmMacAddress0()
	{
		return this.getArgument( CmdLnOption.VM_MAC0 );
	}

	/**
	 * Returns the argument of the command line option {@link CmdLnOption#VM_FSSRC0}.
	 * 
	 * @return argument of the command line option {@link CmdLnOption#VM_FSSRC0}.
	 */
	public String getVmFsSrc0()
	{
		return this.getArgument( CmdLnOption.VM_FSSRC0 );
	}

	/**
	 * Returns the argument of the command line option {@link CmdLnOption#VM_FSTGT0}.
	 * 
	 * @return argument of the command line option {@link CmdLnOption#VM_FSTGT0}.
	 */
	public String getVmFsTgt0()
	{
		return this.getArgument( CmdLnOption.VM_FSTGT0 );
	}

	/**
	 * Returns the argument of the command line option {@link CmdLnOption#VM_FSSRC1}.
	 * 
	 * @return argument of the command line option {@link CmdLnOption#VM_FSSRC1}.
	 */
	public String getVmFsSrc1()
	{
		return this.getArgument( CmdLnOption.VM_FSSRC1 );
	}

	/**
	 * Returns the argument of the command line option {@link CmdLnOption#VM_FSTGT1}.
	 * 
	 * @return argument of the command line option {@link CmdLnOption#VM_FSTGT1}.
	 */
	public String getVmFsTgt1()
	{
		return this.getArgument( CmdLnOption.VM_FSTGT1 );
	}

	/**
	 * Returns the argument of the command line option {@link CmdLnOption#VM_NVGPUIDS0}.
	 * 
	 * @return argument of the command line option {@link CmdLnOption#VM_NVGPUIDS0}.
	 */
	public List<String> getVmNvGpuIds0()
	{
		final String[] nvidiaPciIdsRaw = this.getArguments( CmdLnOption.VM_NVGPUIDS0 );
		final ArrayList<String> nvidiaPciIds;

		if ( nvidiaPciIdsRaw == null || nvidiaPciIdsRaw.length <= 0 ) {
			nvidiaPciIds = new ArrayList<String>();
		} else {
			nvidiaPciIds = new ArrayList<String>( Arrays.asList( nvidiaPciIdsRaw ) );
		}

		return nvidiaPciIds;
	}
	
	public List<String> getUsbRedirDevices()
	{
		final String[] usbRaw = this.getArguments( CmdLnOption.USBREDIR );
		final ArrayList<String> retval;

		if ( usbRaw == null || usbRaw.length <= 0 ) {
			retval = new ArrayList<String>();
		} else {
			retval = new ArrayList<String>( Arrays.asList( usbRaw ) );
		}

		return retval;
	}
	
	/**
	 * Returns the state whether a passthrough of a NVIDIA GPU is requested.
	 * Do this by checking the vendor ID of each PCI device that's being passed
	 * through. If one of them is nvidia, assume we're running passthrough for
	 * an nvidia GPU.
	 */
	public boolean isNvidiaGpuPassthroughEnabled()
	{
		List<String> pciIds = this.getVmNvGpuIds0();
		// parse PCI device description and PCI device address
		for ( int i = 0; i < pciIds.size() - 1; i += 2 ) {
			// parse vendor and device ID
			HostdevPciDeviceDescription deviceDescription = null;
			try {
				deviceDescription = HostdevPciDeviceDescription.valueOf( pciIds.get( i ) );
			} catch ( IllegalArgumentException e ) {
				continue;
			}

			// validate vendor ID
			final int vendorId = deviceDescription.getVendorId();
			if ( TransformationSpecificQemuPciPassthrough.NVIDIA_PCI_VENDOR_ID != vendorId )
				continue;

			// Have at least one device by nvidia, just assume it's a GPU for now
			return true;
		}
		return false;
	}

	/**
	 * Returns the argument of the command line option {@link CmdLnOption#VM_ILMDEVID0}.
	 * 
	 * @return argument of the command line option {@link CmdLnOption#VM_ILMDEVID0}.
	 */
	public String getVmIlMdevId0()
	{
		return this.getArgument( CmdLnOption.VM_ILMDEVID0 );
	}

	/**
	 * Returns the state whether a passthrough of an Intel mediated device (virtual GPU) is required.
	 * 
	 * @return state whether a passthrough of an Intel mediated device (virtual GPU) is required.
	 */
	public boolean isIntelMdevPassthroughEnabled()
	{
		return this.getVmIlMdevId0() != null;
	}
	
	/**
	 * Returns whether the option for spawning a text editor with the final XML prior
	 * to launching the VM should be opened.
	 */
	public boolean isXmlEditorSpawningEnabled()
	{
		return this.cmdLn.hasOption( CmdLnOption.XML_EDIT.shortOption );
	}

	/**
	 * Command line options for the run-virt QEMU plugin (command line tool).
	 * 
	 * @author Manuel Bentele
	 * @version 1.0
	 */
	public enum CmdLnOption
	{
		// @formatter:off
		XML_EDIT    ( '0', "xmledit",     0, "Spawn a text editor with the final XML before starting, so it can be edited"
		                                   + " for testing and debugging purposes"),
		VM_CPU_TOPO ( '1', "cputopo",     1, "Set pairs of CPUs belonging to the same thread, semi-colon separated."
		                                   + " Each group can contain commas or dashes to mark ranges. E.g. 0,1;2-3;4;5;6;7;8,9,10,11" ),
		MANAGER     ( '2', "manager",     0, "Force using virt-manager even if not in debug mode" ),
		VALIDATE    ( '3', "validate",    0, "Validate input file only, exit immediately with exit code 0 on success, 42 otherwise" ),
		USBREDIR    ( '4', "usbredir",    1, "Add USB auto-redirect option to virt-viewer call. Can be passed multiple times." ),
		PERSISTENT  ( '5', "persistent",  0, "VM is running in persistent mode, don't sacrifice safety for speed" ),
		VM_MAC0     ( 'a', "vmmac0",      1, "MAC address for the first network interface" ),
		DEBUG       ( 'b', "debug",       1, "Enable or disable debug mode" ),
		VM_NCPUS    ( 'c', "vmncpus",     1, "Number of virtual CPUs for the virtual machine" ),
		VM_DSPLNAME ( 'd', "vmdsplname",  1, "Display name for the virtual machine" ),
		VM_FSTGT0   ( 'e', "vmfstgt0",    1, "Target directory for first file system passthrough (shared folder)" ),
		VM_FLOPPY0  ( 'f', "vmfloppy0",   1, "Disk image for the first floppy drive" ),
		VM_FLOPPY1  ( 'g', "vmfloppy1",   1, "Disk image for the second floppy drive" ),
		HELP        ( 'h', "help",        0, "" ),
		VM_CFGINP   ( 'i', "vmcfginp",    1, "File name of an existing and filtered Libvirt domain XML configuration file" ),
		DEBUG_PTH   ( 'j', "debugpth",    1, "Enable or disable device passthrough debug mode" ),
		VM_CDROM0   ( 'k', "vmcdrom0",    1, "Disk image for the first CDROM drive" ),
		VM_CDROM1   ( 'l', "vmcdrom1",    1, "Disk image for the second CDROM drive" ),
		VM_MEM      ( 'm', "vmmem",       1, "Amount of memory for the virtual machine" ),
		VM_NAME     ( 'n', "vmname",      1, "Name for the virtual machine" ),
		VM_CFGOUT   ( 'o', "vmcfgout",    1, "File name to output a finalized Libvirt domain XML configuration file" ),
		VM_PARALLEL0( 'p', "vmparallel0", 1, "Device for the first parallel port interface" ),
		VM_SERIAL0  ( 'q', "vmserial0",   1, "Device for the first serial port interface" ),
		VM_HDD0     ( 'r', "vmhdd0",      1, "Disk image for the first HDD device" ),
		VM_OS       ( 's', "vmos",        1, "Operating system running in the virtual machine" ),
		VM_FSSRC0   ( 't', "vmfssrc0",    1, "Source directory for first file system passthrough (shared folder)" ),
		VM_UUID     ( 'u', "vmuuid",      1, "UUID for the virtual machine" ),
		VM_FSSRC1   ( 'v', "vmfssrc1",    1, "Source directory for second file system passthrough (shared folder)" ),
		VM_FSTGT1   ( 'w', "vmfstgt1",    1, "Target directory for second file system passthrough (shared folder)" ),
		FIRMWARE    ( 'x', "firmware",    1, "Path to QEMU firmware specifications directory" ),
		VM_NVGPUIDS0( 'y', "vmnvgpuids0", 2, "PCI device description and address for passthrough of the first Nvidia GPU. " +
		                                     "The argument follow the pattern: " +
				                               "\"<VENDOR ID>:<PRODUCT ID>,<PCI DOMAIN>:<PCI DEVICE>:<PCI DEVICE>.<PCI FUNCTION>\"" ),
		VM_ILMDEVID0( 'z', "vmilmdevid0", 1, "Mediated device UUID for passthrough of the first Intel mediated device (virtual GPU)." );
		// @formatter:on

		/**
		 * Stores the {@link Character} of the short command line option.
		 */
		private final char shortOption;

		/**
		 * Stores the {@link String} of the long command line option.
		 */
		private final String longOption;

		/**
		 * Stores the number of arguments for the command line option.
		 */
		private final int numArguments;

		/**
		 * Stores the textual description of the command line option.
		 */
		private final String description;

		/**
		 * Creates a new command line option for the run-virt QEMU plugin (command line tool).
		 * 
		 * @param shortOption {@link Character} for the short command line option.
		 * @param longOption {@link String} for the long command line option.
		 * @param numArguments number of arguments for the command line option.
		 * @param description textual description of the command line option.
		 */
		CmdLnOption( char shortOption, String longOption, int numArguments, String description )
		{
			this.shortOption = shortOption;
			this.longOption = longOption;
			this.numArguments = numArguments;
			this.description = description;
		}

		/**
		 * Returns the {@link Character} of the short command line option.
		 * 
		 * @return {@link Character} of the short command line option.
		 */
		public String getShortOption()
		{
			return Character.toString( this.shortOption );
		}

		/**
		 * Returns the {@link String} of the long command line option.
		 * 
		 * @return {@link String} of the long command line option.
		 */
		public String getLongOption()
		{
			return this.longOption;
		}

		/**
		 * Returns the number of arguments for the command line option.
		 * 
		 * @return number of arguments for the command line option.
		 */
		public int getNumArguments()
		{
			return this.numArguments;
		}

		/**
		 * Returns the textual description of the command line option.
		 * 
		 * @return textual description of the command line option.
		 */
		public String getDescription()
		{
			return this.description;
		}
	}
}
