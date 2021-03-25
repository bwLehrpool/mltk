package org.openslx.runvirt.virtualization;

import org.libvirt.Domain;
import org.libvirt.LibvirtException;

public class LibvirtVirtualMachine
{
	private Domain domain;

	LibvirtVirtualMachine( Domain vm )
	{
		this.domain = vm;
	}

	public Domain getLibvirtDomain()
	{
		return this.domain;
	}

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

	public void suspend() throws LibvirtVirtualMachineException
	{
		try {
			this.domain.suspend();
		} catch ( LibvirtException e ) {
			throw new LibvirtVirtualMachineException( e.getLocalizedMessage() );
		}
	}

	public void resume() throws LibvirtVirtualMachineException
	{
		try {
			this.domain.resume();
		} catch ( LibvirtException e ) {
			throw new LibvirtVirtualMachineException( e.getLocalizedMessage() );
		}
	}

	public void reboot() throws LibvirtVirtualMachineException
	{
		try {
			this.domain.reboot( 0 );
		} catch ( LibvirtException e ) {
			throw new LibvirtVirtualMachineException( e.getLocalizedMessage() );
		}
	}
}
