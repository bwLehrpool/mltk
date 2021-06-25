package org.openslx.runvirt.virtualization;

import org.libvirt.Domain;
import org.libvirt.LibvirtException;

/**
 * Representation of a Libvirt virtual machine.
 * 
 * @author Manuel Bentele
 * @version 1.0
 */
public class LibvirtVirtualMachine
{
	/**
	 * Internal Libvirt virtualization configuration of the virtual machine.
	 */
	private Domain domain;

	/**
	 * Libvirt virtualization configuration of the virtual machine.
	 */
	private org.openslx.libvirt.domain.Domain configuration;

	/**
	 * Creates a new Libvirt virtual machine specified by a virtualization configuration.
	 * 
	 * @param internalConfiguration internal Libvirt virtualization configuration to specify the
	 *           Libvirt virtual machine.
	 * @param configuration Libvirt virtualization configuration to specify the Libvirt virtual
	 *           machine.
	 */
	LibvirtVirtualMachine( Domain internalConfiguration, org.openslx.libvirt.domain.Domain configuration )
	{
		this.domain = internalConfiguration;
		this.configuration = configuration;
	}

	/**
	 * Returns the internal Libvirt virtualization configuration of the Libvirt virtual machine.
	 * 
	 * @return internal Libvirt virtualization configuration of the Libvirt virtual machine.
	 */
	Domain getLibvirtDomain()
	{
		return this.domain;
	}

	/**
	 * Returns the Libvirt virtualization configuration of the Libvirt virtual machine.
	 * 
	 * @return Libvirt virtualization configuration of the Libvirt virtual machine.
	 */
	public org.openslx.libvirt.domain.Domain getConfiguration()
	{
		return this.configuration;
	}

	/**
	 * Checks if the Libvirt virtual machine is running.
	 * 
	 * @return state of the Libvirt virtual machine whether it is running or not.
	 * @throws LibvirtVirtualMachineException failed to check if Libvirt machine is running or not.
	 */
	public boolean isRunning() throws LibvirtVirtualMachineException
	{
		int state = 0;

		try {
			state = this.domain.isActive();
		} catch ( LibvirtException e ) {
			throw new LibvirtVirtualMachineException( e.getLocalizedMessage() );
		}

		return ( state == 0 ) ? false : true;
	}

	/**
	 * Starts the Libvirt virtual machine.
	 * 
	 * @throws LibvirtVirtualMachineException failed to start the Libvirt virtual machine.
	 */
	public void start() throws LibvirtVirtualMachineException
	{
		if ( !this.isRunning() ) {
			try {
				this.domain.create();
			} catch ( LibvirtException e ) {
				throw new LibvirtVirtualMachineException( e.getLocalizedMessage() );
			}
		}
	}

	/**
	 * Stops the Libvirt virtual machine.
	 * 
	 * @throws LibvirtVirtualMachineException failed to stop the Libvirt virtual machine.
	 */
	public void stop() throws LibvirtVirtualMachineException
	{
		if ( this.isRunning() ) {
			try {
				this.domain.shutdown();
			} catch ( LibvirtException e ) {
				throw new LibvirtVirtualMachineException( e.getLocalizedMessage() );
			}
		}
	}

	/**
	 * Suspends the Libvirt virtual machine.
	 * 
	 * @throws LibvirtVirtualMachineException failed to suspend the Libvirt virtual machine.
	 */
	public void suspend() throws LibvirtVirtualMachineException
	{
		try {
			this.domain.suspend();
		} catch ( LibvirtException e ) {
			throw new LibvirtVirtualMachineException( e.getLocalizedMessage() );
		}
	}

	/**
	 * Resumes the Libvirt virtual machine.
	 * 
	 * @throws LibvirtVirtualMachineException faild to resume the Libvirt virtual machine.
	 */
	public void resume() throws LibvirtVirtualMachineException
	{
		try {
			this.domain.resume();
		} catch ( LibvirtException e ) {
			throw new LibvirtVirtualMachineException( e.getLocalizedMessage() );
		}
	}

	/**
	 * Reboot the Libvirt virtual machine.
	 * 
	 * @throws LibvirtVirtualMachineException failed to reboot the Libvirt virtual machine.
	 */
	public void reboot() throws LibvirtVirtualMachineException
	{
		try {
			this.domain.reboot( 0 );
		} catch ( LibvirtException e ) {
			throw new LibvirtVirtualMachineException( e.getLocalizedMessage() );
		}
	}
}
