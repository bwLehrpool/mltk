package org.openslx.runvirt.viewer;

import java.io.IOException;
import java.nio.charset.StandardCharsets;

import org.apache.commons.exec.CommandLine;
import org.apache.commons.exec.DefaultExecutor;
import org.apache.commons.exec.PumpStreamHandler;
import org.apache.commons.io.FileUtils;
import org.apache.commons.io.IOUtils;
import org.apache.commons.io.output.ByteArrayOutputStream;

/**
 * Utils for viewing displays of virtual machines.
 * 
 * @author Manuel Bentele
 * @version 1.0
 */
public class ViewerUtils
{
	/**
	 * Synchronously executes a viewer program specified by a command line call.
	 * <p>
	 * The command line call of the viewer program consists of the program name and an optional list
	 * of submitted command line arguments for the viewer program. The result of the executed viewer
	 * program from the standard output is returned after the program has exited.
	 * 
	 * @param viewerProgram name of the viewer program.
	 * @param viewerArguments optional command line arguments for the viewer program.
	 * @return result of the executed viewer program from the standard output.
	 * @throws ViewerException failed to execute the viewer program.
	 */
	@SuppressWarnings( "deprecation" )
	public static String executeViewer( String viewerProgram, String[] viewerArguments ) throws ViewerException
	{
		final CommandLine viewerCommandLine = new CommandLine( viewerProgram );
		final DefaultExecutor viewerExecutor = new DefaultExecutor();

		// prepare viewer command to execute
		viewerCommandLine.addArguments( viewerArguments );

		// set up temporary working directory for the viewer process
		viewerExecutor.setWorkingDirectory( FileUtils.getTempDirectory() );

		// set expected exit value of the viewer process indicating a successful operation
		viewerExecutor.setExitValue( 0 );

		// set up output stream handler to retrieve the content from the viewer's standard output
		final ByteArrayOutputStream viewerOutputStream = new ByteArrayOutputStream();
		final PumpStreamHandler viewerOutputStreamHandler = new PumpStreamHandler( viewerOutputStream );
		viewerExecutor.setStreamHandler( viewerOutputStreamHandler );

		// execute the viewer command as blocking process
		try {
			viewerExecutor.execute( viewerCommandLine );
		} catch ( IOException e ) {
			throw new ViewerException( "Failed to execute '" + viewerProgram + "': " + e.getLocalizedMessage() );
		}

		final String viewerOuput = viewerOutputStream.toString( StandardCharsets.UTF_8 );
		IOUtils.closeQuietly( viewerOutputStream );

		return viewerOuput;
	}
}
