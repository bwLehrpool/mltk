package org.openslx.runvirt.plugin.qemu;

import java.io.IOException;
import java.lang.ProcessBuilder.Redirect;
import java.util.concurrent.TimeUnit;

/**
 * Small wrapper around Process that will try to launch any sensible GUI
 * text editor on the given file.
 */
public class EditorRunner
{

	private static final String[] EDITORS = {
			"mousepad",
			"leafpad",
			"gedit",
			"gvim",
			"medit",
			"kate",
			"featherpad",
	};

	public static void open( String file )
	{
		for ( String editor : EDITORS ) {
			if ( open( editor, file ) )
				return;
		}
		return;
	}

	private static boolean open( String editor, String file )
	{
		ProcessBuilder pr = new ProcessBuilder( editor, file );
		pr.redirectError( Redirect.DISCARD );
		pr.redirectOutput( Redirect.DISCARD );
		try {
			final Process p = pr.start();
			p.waitFor( 1, TimeUnit.SECONDS );
			if ( !p.isAlive() )
				return false;
			p.waitFor();
		} catch ( IOException | InterruptedException e ) {
			return false;
		}
		return true;
	}

}
