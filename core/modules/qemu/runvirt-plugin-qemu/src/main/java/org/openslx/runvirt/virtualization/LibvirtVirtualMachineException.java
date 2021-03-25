package org.openslx.runvirt.virtualization;

public class LibvirtVirtualMachineException extends Exception
{
	/**
	 * Version for serialization.
	 */
	private static final long serialVersionUID = -5371327391243047616L;

	public LibvirtVirtualMachineException( String errorMsg )
	{
		super( errorMsg );
	}
}
