package org.openslx.runvirt.plugin.qemu;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.io.ByteArrayOutputStream;
import java.io.PrintStream;

import org.apache.log4j.Level;
import org.apache.log4j.LogManager;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Nested;
import org.junit.jupiter.api.Test;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs.CmdLnOption;

import com.ginsberg.junit.exit.ExpectSystemExit;
import com.ginsberg.junit.exit.ExpectSystemExitWithStatus;

public class AppTest
{
	@BeforeAll
	private static void setUp()
	{
		// disable logging with log4j
		LogManager.getRootLogger().setLevel( Level.OFF );
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
		@DisplayName( "Test ouput of correct 'help' command line option (short version)" )
		@ExpectSystemExit
		public void testCmdLnOptionHelpShortCorrect()
		{
			String[] argsShortHelpOptionCorrect = { "-" + CmdLnOption.HELP.getShortOption() };

			this.setUp();

			// test correct usage of the short help option
			try {
				App.main( argsShortHelpOptionCorrect );
			} catch ( Exception e ) {
				// do nothing and check output afterwards
			}

			final String shortHelpOptionCorrectOutput = new String( this.out.toString() );
			final String shortHelpOptionCorrectErrOutput = new String( this.err.toString() );
			assertTrue( shortHelpOptionCorrectOutput.contains( "usage" ) );
			assertTrue( shortHelpOptionCorrectOutput.contains( App.APP_NAME ) );
			assertTrue( shortHelpOptionCorrectOutput.contains( App.APP_INFO ) );
			assertTrue( shortHelpOptionCorrectOutput.contains( App.APP_DESC ) );

			// test that no error was logged and output is available
			assertEquals( 2503, shortHelpOptionCorrectOutput.length() );
			assertEquals( 0, shortHelpOptionCorrectErrOutput.length() );
		}

		@Test
		@DisplayName( "Test ouput of correct 'help' command line option (long version)" )
		@ExpectSystemExit
		public void testCmdLnOptionHelpLongCorrect()
		{
			String[] argsLongHelpOptionCorrect = { "--" + CmdLnOption.HELP.getLongOption() };

			this.setUp();

			// test correct usage of the long help option
			try {
				App.main( argsLongHelpOptionCorrect );
			} catch ( Exception e ) {
				// do nothing and check output afterwards
			}

			final String longHelpOptionCorrectOutput = this.out.toString();
			final String longHelpOptionCorrectErrOutput = this.err.toString();
			assertTrue( longHelpOptionCorrectOutput.contains( "usage" ) );
			assertTrue( longHelpOptionCorrectOutput.contains( App.APP_NAME ) );
			assertTrue( longHelpOptionCorrectOutput.contains( App.APP_INFO ) );
			assertTrue( longHelpOptionCorrectOutput.contains( App.APP_DESC ) );

			// test that no error was logged and output is available
			assertEquals( 2503, longHelpOptionCorrectOutput.length() );
			assertEquals( 0, longHelpOptionCorrectErrOutput.length() );
		}

		@Test
		@DisplayName( "Test ouput of incorrect 'help' command line option (short version)" )
		@ExpectSystemExit
		public void testCmdLnOptionHelpShortIncorrect()
		{
			String[] argsShortHelpOptionIncorrect = { "---" + CmdLnOption.HELP.getShortOption() };

			this.setUp();

			// test incorrect usage of the short help option
			try {
				App.main( argsShortHelpOptionIncorrect );
			} catch ( Exception e ) {
				// do nothing and check output afterwards
			}

			final String shortHelpOptionIncorrectOutput = this.out.toString();
			final String shortHelpOptionIncorrectErrOutput = this.err.toString();
			assertTrue( shortHelpOptionIncorrectOutput.contains( "usage" ) );
			assertTrue( shortHelpOptionIncorrectOutput.contains( App.APP_NAME ) );
			assertTrue( shortHelpOptionIncorrectOutput.contains( App.APP_INFO ) );
			assertTrue( shortHelpOptionIncorrectOutput.contains( App.APP_DESC ) );

			// test that error was logged and output is available
			assertEquals( 2503, shortHelpOptionIncorrectOutput.length() );
			assertEquals( 0, shortHelpOptionIncorrectErrOutput.length() );
		}

		@Test
		@DisplayName( "Test ouput of incorrect 'help' command line option (long version)" )
		@ExpectSystemExit
		public void testCmdLnOptionHelpLongIncorrect()
		{
			String[] argsLongHelpOptionIncorrect = { "---" + CmdLnOption.HELP.getLongOption() };

			this.setUp();

			// test incorrect usage of the long help option
			try {
				App.main( argsLongHelpOptionIncorrect );
			} catch ( Exception e ) {
				// do nothing and check output afterwards
			}

			final String longHelpOptionIncorrectOutput = this.out.toString();
			final String longHelpOptionIncorrectErrOutput = this.err.toString();
			assertTrue( longHelpOptionIncorrectOutput.contains( "usage" ) );
			assertTrue( longHelpOptionIncorrectOutput.contains( App.APP_NAME ) );
			assertTrue( longHelpOptionIncorrectOutput.contains( App.APP_INFO ) );
			assertTrue( longHelpOptionIncorrectOutput.contains( App.APP_DESC ) );

			// test that error was logged and output is available
			assertEquals( 2503, longHelpOptionIncorrectOutput.length() );
			assertEquals( 0, longHelpOptionIncorrectErrOutput.length() );
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
