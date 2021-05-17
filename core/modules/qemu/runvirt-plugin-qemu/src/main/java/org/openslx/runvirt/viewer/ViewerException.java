package org.openslx.runvirt.viewer;

/**
 * An exception of a viewer error during displaying all displays of a virtual machine.
 * 
 * @author Manuel Bentele
 * @version 1.0
 */
public class ViewerException extends Exception
{
	/**
	 * Version for serialization.
	 */
	private static final long serialVersionUID = 161091514643380414L;

	/**
	 * Creates a new viewer exception including an error message.
	 *
	 * @param errorMsg message to describe a specific viewer error.
	 */
	public ViewerException( String errorMsg )
	{
		super( errorMsg );
	}

	/**
	 * Creates a new viewer exception by a copy of an existing viewer exception.
	 * 
	 * @param e existing viewer exception.
	 */
	public ViewerException( ViewerException e )
	{
		super( e );
	}
}
