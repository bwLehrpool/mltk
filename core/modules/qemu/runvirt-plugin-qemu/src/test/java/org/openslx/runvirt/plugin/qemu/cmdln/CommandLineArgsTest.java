package org.openslx.runvirt.plugin.qemu.cmdln;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.io.File;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs.CmdLnOption;

public class CommandLineArgsTest
{
	// @formatter:off
	public static final String CMDLN_PREFIX_OPTION_SHORT = "-";
	public static final String CMDLN_PREFIX_OPTION_LONG  = "--";

	private static final String CMDLN_TEST_DEBUG_OFF   = "false";
	private static final String CMDLN_TEST_DEBUG_ON    = "true";
	private static final String CMDLN_TEST_NAME        = "test";
	private static final String CMDLN_TEST_FILENAME    = System.getProperty( "user.dir" ) + File.separator + CMDLN_TEST_NAME;
	private static final String CMDLN_TEST_UUID        = "c9570672-cbae-4cbd-801a-881b281b8d79";
	private static final String CMDLN_TEST_OS          = "Windows 10 (x64)";
	private static final int    CMDLN_TEST_NCPUS       = 4;
	private static final String CMDLN_TEST_MEM         = "1024";
	private static final String CMDLN_TEST_PARPORT     = "/dev/parport0";
	private static final String CMDLN_TEST_SERPORT     = "/dev/ttyS0";
	private static final String CMDLN_TEST_MAC         = "02:42:8e:77:1b:e6";
	// @formatter:on

	@Test
	@DisplayName( "Test the parsing of wrong command line options" )
	public void testCmdlnOptionsIncorrect()
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_SHORT + "hello",
				"world argument",
				CMDLN_PREFIX_OPTION_LONG + "info",
				"description"
		};

		CommandLineArgs cmdLn = new CommandLineArgs();

		assertThrows( CommandLineArgsException.class, () -> cmdLn.parseCmdLnArgs( args ) );
	}

	@Test
	@DisplayName( "Test the parsing of the help command line option (short version)" )
	public void testCmdlnOptionHelpShort() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_SHORT + CmdLnOption.HELP.getShortOption()
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertTrue( cmdLn.isHelpAquired() );
	}

	@Test
	@DisplayName( "Test the parsing of the help command line option (long version)" )
	public void testCmdlnOptionHelpLong() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_LONG + CmdLnOption.HELP.getLongOption()
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertTrue( cmdLn.isHelpAquired() );
	}

	@Test
	@DisplayName( "Test the parsing of the enabled debug command line option (short version)" )
	public void testCmdlnOptionDebugEnabledShort() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_SHORT + CmdLnOption.DEBUG.getShortOption(),
				CMDLN_TEST_DEBUG_ON
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertTrue( cmdLn.isDebugEnabled() );
	}

	@Test
	@DisplayName( "Test the parsing of the enabled debug command line option (long version)" )
	public void testCmdlnOptionDebugEnabledLong() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_LONG + CmdLnOption.DEBUG.getLongOption(),
				CMDLN_TEST_DEBUG_ON
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertTrue( cmdLn.isDebugEnabled() );
	}

	@Test
	@DisplayName( "Test the parsing of the disabled debug command line option (short version)" )
	public void testCmdlnOptionDebugDisabledShort() throws CommandLineArgsException
	{
		final String[] argsDebugOff = {
				CMDLN_PREFIX_OPTION_SHORT + CmdLnOption.DEBUG.getShortOption(),
				CMDLN_TEST_DEBUG_OFF
		};

		final String[] argsDebugMissing = {};

		CommandLineArgs cmdLnDebugOff = new CommandLineArgs( argsDebugOff );
		CommandLineArgs cmdLnDebugMissing = new CommandLineArgs( argsDebugMissing );

		assertFalse( cmdLnDebugOff.isDebugEnabled() );
		assertFalse( cmdLnDebugMissing.isDebugEnabled() );
	}

	@Test
	@DisplayName( "Test the parsing of the disabled debug command line option (long version)" )
	public void testCmdlnOptionDebugDisabledLong() throws CommandLineArgsException
	{
		final String[] argsDebugOff = {
				CMDLN_PREFIX_OPTION_LONG + CmdLnOption.DEBUG.getLongOption(),
				CMDLN_TEST_DEBUG_OFF
		};

		final String[] argsDebugMissing = {};

		CommandLineArgs cmdLnDebugOff = new CommandLineArgs( argsDebugOff );
		CommandLineArgs cmdLnDebugMissing = new CommandLineArgs( argsDebugMissing );

		assertFalse( cmdLnDebugOff.isDebugEnabled() );
		assertFalse( cmdLnDebugMissing.isDebugEnabled() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM configuration input command line option (short version)" )
	public void testCmdlnOptionVmCfgInpShort() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_SHORT + CmdLnOption.VM_CFGINP.getShortOption(),
				CMDLN_TEST_FILENAME
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_FILENAME, cmdLn.getVmCfgInpFileName() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM configuration input command line option (long version)" )
	public void testCmdlnOptionVmCfgInpLong() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_CFGINP.getLongOption(),
				CMDLN_TEST_FILENAME
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_FILENAME, cmdLn.getVmCfgInpFileName() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM configuration output command line option (short version)" )
	public void testCmdlnOptionVmCfgOutShort() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_SHORT + CmdLnOption.VM_CFGOUT.getShortOption(),
				CMDLN_TEST_FILENAME
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_FILENAME, cmdLn.getVmCfgOutFileName() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM configuration output command line option (long version)" )
	public void testCmdlnOptionVmCfgOutLong() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_CFGOUT.getLongOption(),
				CMDLN_TEST_FILENAME
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_FILENAME, cmdLn.getVmCfgOutFileName() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM name command line option (short version)" )
	public void testCmdlnOptionVmNameShort() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_SHORT + CmdLnOption.VM_NAME.getShortOption(),
				CMDLN_TEST_NAME
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_NAME, cmdLn.getVmName() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM name command line option (long version)" )
	public void testCmdlnOptionVmNameLong() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_NAME.getLongOption(),
				CMDLN_TEST_NAME
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_NAME, cmdLn.getVmName() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM UUID command line option (short version)" )
	public void testCmdlnOptionVmUuidShort() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_SHORT + CmdLnOption.VM_UUID.getShortOption(),
				CMDLN_TEST_UUID
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_UUID, cmdLn.getVmUuid() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM UUID command line option (long version)" )
	public void testCmdlnOptionVmUuidLong() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_UUID.getLongOption(),
				CMDLN_TEST_UUID
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_UUID, cmdLn.getVmUuid() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM display name command line option (short version)" )
	public void testCmdlnOptionVmDsplNameShort() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_SHORT + CmdLnOption.VM_DSPLNAME.getShortOption(),
				CMDLN_TEST_NAME
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_NAME, cmdLn.getVmDisplayName() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM display name command line option (long version)" )
	public void testCmdlnOptionVmDsplNameLong() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_DSPLNAME.getLongOption(),
				CMDLN_TEST_NAME
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_NAME, cmdLn.getVmDisplayName() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM OS command line option (short version)" )
	public void testCmdlnOptionVmOsShort() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_SHORT + CmdLnOption.VM_OS.getShortOption(),
				CMDLN_TEST_OS
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_OS, cmdLn.getVmOperatingSystem() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM OS command line option (long version)" )
	public void testCmdlnOptionVmOsLong() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_OS.getLongOption(),
				CMDLN_TEST_OS
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_OS, cmdLn.getVmOperatingSystem() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM CPU number command line option (short version)" )
	public void testCmdlnOptionVmNCpusShort() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_SHORT + CmdLnOption.VM_NCPUS.getShortOption(),
				Integer.toString( CMDLN_TEST_NCPUS )
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_NCPUS, cmdLn.getVmNumCpus() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM CPU number command line option (long version)" )
	public void testCmdlnOptionVmNCpusLong() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_NCPUS.getLongOption(),
				Integer.toString( CMDLN_TEST_NCPUS )
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_NCPUS, cmdLn.getVmNumCpus() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM memory command line option (short version)" )
	public void testCmdlnOptionVmMemShort() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_SHORT + CmdLnOption.VM_MEM.getShortOption(),
				CMDLN_TEST_MEM
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_MEM, cmdLn.getVmMemory() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM memory command line option (long version)" )
	public void testCmdlnOptionVmMemLong() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_MEM.getLongOption(),
				CMDLN_TEST_MEM
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_MEM, cmdLn.getVmMemory() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM first HDD disk image command line option (short version)" )
	public void testCmdlnOptionVmHdd0Short() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_SHORT + CmdLnOption.VM_HDD0.getShortOption(),
				CMDLN_TEST_FILENAME
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_FILENAME, cmdLn.getVmDiskFileNameHDD0() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM first HDD disk image command line option (long version)" )
	public void testCmdlnOptionVmHdd0Long() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_HDD0.getLongOption(),
				CMDLN_TEST_FILENAME
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_FILENAME, cmdLn.getVmDiskFileNameHDD0() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM first floppy disk image command line option (short version)" )
	public void testCmdlnOptionVmFloppy0Short() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_SHORT + CmdLnOption.VM_FLOPPY0.getShortOption(),
				CMDLN_TEST_FILENAME
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_FILENAME, cmdLn.getVmDiskFileNameFloppy0() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM first floppy disk image command line option (long version)" )
	public void testCmdlnOptionVmFloppy0Long() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_FLOPPY0.getLongOption(),
				CMDLN_TEST_FILENAME
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_FILENAME, cmdLn.getVmDiskFileNameFloppy0() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM second floppy disk image command line option (short version)" )
	public void testCmdlnOptionVmFloppy1Short() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_SHORT + CmdLnOption.VM_FLOPPY1.getShortOption(),
				CMDLN_TEST_FILENAME
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_FILENAME, cmdLn.getVmDiskFileNameFloppy1() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM second floppy disk image command line option (long version)" )
	public void testCmdlnOptionVmFloppy1Long() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_FLOPPY1.getLongOption(),
				CMDLN_TEST_FILENAME
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_FILENAME, cmdLn.getVmDiskFileNameFloppy1() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM first CDROM disk image command line option (short version)" )
	public void testCmdlnOptionVmCdrom0Short() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_SHORT + CmdLnOption.VM_CDROM0.getShortOption(),
				CMDLN_TEST_FILENAME
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_FILENAME, cmdLn.getVmDiskFileNameCdrom0() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM first CDROM disk image command line option (long version)" )
	public void testCmdlnOptionVmCdrom0Long() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_CDROM0.getLongOption(),
				CMDLN_TEST_FILENAME
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_FILENAME, cmdLn.getVmDiskFileNameCdrom0() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM second CDROM disk image command line option (short version)" )
	public void testCmdlnOptionVmCdrom1Short() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_SHORT + CmdLnOption.VM_CDROM1.getShortOption(),
				CMDLN_TEST_FILENAME
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_FILENAME, cmdLn.getVmDiskFileNameCdrom1() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM second CDROM disk image command line option (long version)" )
	public void testCmdlnOptionVmCdrom1Long() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_CDROM1.getLongOption(),
				CMDLN_TEST_FILENAME
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_FILENAME, cmdLn.getVmDiskFileNameCdrom1() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM first parallel interface command line option (short version)" )
	public void testCmdlnOptionVmParallel0Short() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_SHORT + CmdLnOption.VM_PARALLEL0.getShortOption(),
				CMDLN_TEST_PARPORT
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_PARPORT, cmdLn.getVmDeviceParallel0() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM first parallel interface command line option (long version)" )
	public void testCmdlnOptionVmParallel0Long() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_PARALLEL0.getLongOption(),
				CMDLN_TEST_PARPORT
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_PARPORT, cmdLn.getVmDeviceParallel0() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM first serial interface command line option (short version)" )
	public void testCmdlnOptionVmSerial0Short() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_SHORT + CmdLnOption.VM_SERIAL0.getShortOption(),
				CMDLN_TEST_SERPORT
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_SERPORT, cmdLn.getVmDeviceSerial0() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM first serial interface command line option (long version)" )
	public void testCmdlnOptionVmSerial0Long() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_SERIAL0.getLongOption(),
				CMDLN_TEST_SERPORT
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_SERPORT, cmdLn.getVmDeviceSerial0() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM first network interface MAC command line option (short version)" )
	public void testCmdlnOptionVmMac0Short() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_SHORT + CmdLnOption.VM_MAC0.getShortOption(),
				CMDLN_TEST_MAC
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_MAC, cmdLn.getVmMacAddress0() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM first network interface MAC command line option (long version)" )
	public void testCmdlnOptionVmMac0Long() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_MAC0.getLongOption(),
				CMDLN_TEST_MAC
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_MAC, cmdLn.getVmMacAddress0() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM first file system source command line option (short version)" )
	public void testCmdlnOptionVmFsSrc0Short() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_SHORT + CmdLnOption.VM_FSSRC0.getShortOption(),
				CMDLN_TEST_FILENAME
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_FILENAME, cmdLn.getVmFsSrc0() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM first file system source command line option (long version)" )
	public void testCmdlnOptionVmFsSrc0Long() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_FSSRC0.getLongOption(),
				CMDLN_TEST_FILENAME
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_FILENAME, cmdLn.getVmFsSrc0() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM first file system target command line option (short version)" )
	public void testCmdlnOptionVmFsTgt0Short() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_SHORT + CmdLnOption.VM_FSTGT0.getShortOption(),
				CMDLN_TEST_NAME
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_NAME, cmdLn.getVmFsTgt0() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM first file system target command line option (long version)" )
	public void testCmdlnOptionVmFsTgt0Long() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_FSTGT0.getLongOption(),
				CMDLN_TEST_NAME
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_NAME, cmdLn.getVmFsTgt0() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM second file system source command line option (short version)" )
	public void testCmdlnOptionVmFsSrc1Short() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_SHORT + CmdLnOption.VM_FSSRC1.getShortOption(),
				CMDLN_TEST_FILENAME
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_FILENAME, cmdLn.getVmFsSrc1() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM second file system source command line option (long version)" )
	public void testCmdlnOptionVmFsSrc1Long() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_FSSRC1.getLongOption(),
				CMDLN_TEST_FILENAME
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_FILENAME, cmdLn.getVmFsSrc1() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM second file system target command line option (short version)" )
	public void testCmdlnOptionVmFsTgt1Short() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_SHORT + CmdLnOption.VM_FSTGT1.getShortOption(),
				CMDLN_TEST_NAME
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_NAME, cmdLn.getVmFsTgt1() );
	}

	@Test
	@DisplayName( "Test the parsing of the VM second file system target command line option (long version)" )
	public void testCmdlnOptionVmFsTgt1Long() throws CommandLineArgsException
	{
		final String[] args = {
				CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_FSTGT1.getLongOption(),
				CMDLN_TEST_NAME
		};

		CommandLineArgs cmdLn = new CommandLineArgs( args );

		assertEquals( CMDLN_TEST_NAME, cmdLn.getVmFsTgt1() );
	}
}
