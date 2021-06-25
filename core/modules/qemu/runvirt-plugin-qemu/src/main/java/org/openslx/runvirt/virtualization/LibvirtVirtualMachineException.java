package org.openslx.runvirt.virtualization;

/**
 * An exception of a Libvirt virtual machine error during controlling the virtual machine.
 * 
 * @author Manuel Bentele
 * @version 1.0
 */
public class LibvirtVirtualMachineException extends Exception
{
	/**
	 * Version for serialization.
	 */
	private static final long serialVersionUID = -5371327391243047616L;

	/**
	 * Creates a Libvirt virtual machine exception including an error message.
	 * 
	 * @param errorMsg message to describe a specific Libvirt virtual machine error.
	 */
	public LibvirtVirtualMachineException( String errorMsg )
	{
		super( errorMsg );
	}
}
