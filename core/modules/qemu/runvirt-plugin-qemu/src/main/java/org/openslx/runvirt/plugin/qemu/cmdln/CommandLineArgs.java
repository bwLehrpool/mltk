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

/**
 * Command line argument parser for the run-virt QEMU plugin (command line tool).
 * 
 * @author Manuel Bentele
 * @version 1.0
 */
public class CommandLineArgs
{
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

	/**
	 * Returns the state whether a passthrough of a NVIDIA GPU is required.
	 * 
	 * @return state whether a passthrough of a NVIDIA GPU is required.
	 */
	public boolean isNvidiaGpuPassthroughEnabled()
	{
		return this.getVmNvGpuIds0().size() > 0;
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
	 * Command line options for the run-virt QEMU plugin (command line tool).
	 * 
	 * @author Manuel Bentele
	 * @version 1.0
	 */
	public enum CmdLnOption
	{
		// @formatter:off
		HELP        ( 'h', "help",        0, "" ),
		DEBUG       ( 'b', "debug",       1, "Enable or disable debug mode" ),
		FIRMWARE    ( 'x', "firmware",    1, "Path to QEMU firmware specifications directory" ),
		VM_CFGINP   ( 'i', "vmcfginp",    1, "File name of an existing and filtered Libvirt domain XML configuration file" ),
		VM_CFGOUT   ( 'o', "vmcfgout",    1, "File name to output a finalized Libvirt domain XML configuration file" ),
		VM_NAME     ( 'n', "vmname",      1, "Name for the virtual machine" ),
		VM_UUID     ( 'u', "vmuuid",      1, "UUID for the virtual machine" ),
		VM_DSPLNAME ( 'd', "vmdsplname",  1, "Display name for the virtual machine" ),
		VM_OS       ( 's', "vmos",        1, "Operating system running in the virtual machine" ),
		VM_NCPUS    ( 'c', "vmncpus",     1, "Number of virtual CPUs for the virtual machine" ),
		VM_MEM      ( 'm', "vmmem",       1, "Amount of memory for the virtual machine" ),
		VM_HDD0     ( 'r', "vmhdd0",      1, "Disk image for the first HDD device" ),
		VM_FLOPPY0  ( 'f', "vmfloppy0",   1, "Disk image for the first floppy drive" ),
		VM_FLOPPY1  ( 'g', "vmfloppy1",   1, "Disk image for the second floppy drive" ),
		VM_CDROM0   ( 'k', "vmcdrom0",    1, "Disk image for the first CDROM drive" ),
		VM_CDROM1   ( 'l', "vmcdrom1",    1, "Disk image for the second CDROM drive" ),
		VM_PARALLEL0( 'p', "vmparallel0", 1, "Device for the first parallel port interface" ),
		VM_SERIAL0  ( 'q', "vmserial0",   1, "Device for the first serial port interface" ),
		VM_MAC0     ( 'a', "vmmac0",      1, "MAC address for the first network interface" ),
		VM_FSSRC0   ( 't', "vmfssrc0",    1, "Source directory for first file system passthrough (shared folder)" ),
		VM_FSTGT0   ( 'e', "vmfstgt0",    1, "Target directory for first file system passthrough (shared folder)" ),
		VM_FSSRC1   ( 'v', "vmfssrc1",    1, "Source directory for second file system passthrough (shared folder)" ),
		VM_FSTGT1   ( 'w', "vmfstgt1",    1, "Target directory for second file system passthrough (shared folder)" ),
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
