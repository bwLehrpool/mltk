package org.openslx.runvirt.virtualization;

/**
 * An exception of a Libvirt hypervisor error during acquiring the hypervisor's functionality.
 * 
 * @author Manuel Bentele
 * @version 1.0
 */
public class LibvirtHypervisorException extends Exception
{
	/**
	 * Version for serialization.
	 */
	private static final long serialVersionUID = -3631452625806770209L;

	/**
	 * Creates a Libvirt hypervisor exception including an error message.
	 * 
	 * @param errorMsg message to describe a specific Libvirt hypervisor error.
	 */
	LibvirtHypervisorException( String errorMsg )
	{
		super( errorMsg );
	}
}
