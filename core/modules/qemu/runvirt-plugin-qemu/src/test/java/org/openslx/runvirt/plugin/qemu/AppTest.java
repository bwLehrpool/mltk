package org.openslx.runvirt.plugin.qemu;

import java.io.ByteArrayOutputStream;
import java.io.PrintStream;

import org.apache.logging.log4j.Level;
import org.apache.logging.log4j.core.config.Configurator;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Nested;
import org.junit.jupiter.api.Test;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs.CmdLnOption;

import com.ginsberg.junit.exit.ExpectSystemExitWithStatus;

public class AppTest
{
	
	@BeforeAll
	public static void setUp()
	{
		// disable logging with log4j
		Configurator.setRootLevel( Level.OFF );
	}

	@Nested
	public class CmdLnTest
	{
		private final ByteArrayOutputStream out = new ByteArrayOutputStream();
		private final ByteArrayOutputStream err = new ByteArrayOutputStream();

		private void setUp()
		{
			// redirect output and error stream content
			System.setOut( new PrintStream( this.out ) );
			System.setErr( new PrintStream( this.err ) );
		}

		@Test
		@DisplayName( "Test exit status of application invoked with correct 'help' command line option (short version)" )
		@ExpectSystemExitWithStatus( 0 )
		public void testCmdLnOptionHelpShortCorrectExit()
		{
			String[] argsShortHelpOptionCorrect = { "-" + CmdLnOption.HELP.getShortOption() };

			this.setUp();

			App.main( argsShortHelpOptionCorrect );
		}

		@Test
		@DisplayName( "Test exit status of application invoked with correct 'help' command line option (long version)" )
		@ExpectSystemExitWithStatus( 0 )
		public void testCmdLnOptionHelpLongCorrectExit()
		{
			String[] argsLongHelpOptionCorrect = { "--" + CmdLnOption.HELP.getLongOption() };

			this.setUp();

			App.main( argsLongHelpOptionCorrect );
		}

		@Test
		@DisplayName( "Test exit status of application invoked with incorrect 'help' command line option (short version)" )
		@ExpectSystemExitWithStatus( 1 )
		public void testCmdLnOptionHelpShortIncorrectExit()
		{
			String[] argsShortHelpOptionCorrect = { "---" + CmdLnOption.HELP.getShortOption() };

			this.setUp();

			App.main( argsShortHelpOptionCorrect );
		}

		@Test
		@DisplayName( "Test exit status of application invoked with incorrect 'help' command line option (long version)" )
		@ExpectSystemExitWithStatus( 1 )
		public void testCmdLnOptionHelpLongIncorrectExit()
		{
			String[] argsLongHelpOptionCorrect = { "---" + CmdLnOption.HELP.getLongOption() };

			this.setUp();

			App.main( argsLongHelpOptionCorrect );
		}
	}
}
