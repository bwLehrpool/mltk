package org.openslx.runvirt.virtualization;

public class LibvirtHypervisorException extends Exception
{
	/**
	 * Version for serialization.
	 */
	private static final long serialVersionUID = -3631452625806770209L;

	LibvirtHypervisorException( String errorMsg )
	{
		super( errorMsg );
	}
}
