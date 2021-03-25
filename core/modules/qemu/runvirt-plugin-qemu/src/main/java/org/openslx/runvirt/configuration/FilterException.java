package org.openslx.runvirt.configuration;

public class FilterException extends Exception
{
	/**
	 * Version for serialization.
	 */
	private static final long serialVersionUID = 7293420658901349154L;

	public FilterException( String errorMsg )
	{
		super( errorMsg );
	}
}
