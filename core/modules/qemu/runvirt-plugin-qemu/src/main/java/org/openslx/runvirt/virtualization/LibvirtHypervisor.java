package org.openslx.runvirt.virtualization;

import java.io.Closeable;

import org.libvirt.Connect;
import org.libvirt.LibvirtException;
import org.openslx.libvirt.capabilities.Capabilities;
import org.openslx.libvirt.xml.LibvirtXmlDocumentException;
import org.openslx.libvirt.xml.LibvirtXmlSerializationException;
import org.openslx.libvirt.xml.LibvirtXmlValidationException;

public abstract class LibvirtHypervisor implements Closeable
{
	protected Connect hypervisor = null;

	public LibvirtHypervisor( String connectionUri ) throws LibvirtHypervisorException
	{
		this.connect( connectionUri );
	}

	protected void connect( String connectionUri ) throws LibvirtHypervisorException
	{
		try {
			this.hypervisor = new Connect( connectionUri );
		} catch ( LibvirtException e ) {
			throw new LibvirtHypervisorException( e.getLocalizedMessage() );
		}
	}

	public Capabilities getCapabilites() throws LibvirtHypervisorException
	{
		Capabilities hypervisorCapabilities = null;

		try {
			final String hypervisorCapabilitiesString = this.hypervisor.getCapabilities();
			hypervisorCapabilities = new Capabilities( hypervisorCapabilitiesString );
		} catch ( LibvirtException | LibvirtXmlDocumentException | LibvirtXmlSerializationException
				| LibvirtXmlValidationException e ) {
			throw new LibvirtHypervisorException( e.getLocalizedMessage() );
		}

		return hypervisorCapabilities;
	}

	public int getVersion() throws LibvirtHypervisorException
	{
		int hypervisorVersion = 0;

		try {
			final long hypervisorVersionLong = this.hypervisor.getVersion();
			hypervisorVersion = Long.valueOf( hypervisorVersionLong ).intValue();
		} catch ( LibvirtException e ) {
			throw new LibvirtHypervisorException( e.getLocalizedMessage() );
		}

		return hypervisorVersion;
	}

	public LibvirtVirtualMachine registerVm( org.openslx.libvirt.domain.Domain vmConfiguration )
			throws LibvirtHypervisorException
	{
		final String xmlVmConfiguration = vmConfiguration.toString();
		org.libvirt.Domain libvirtDomain = null;

		try {
			libvirtDomain = this.hypervisor.domainDefineXML( xmlVmConfiguration );
		} catch ( LibvirtException e ) {
			throw new LibvirtHypervisorException( e.getLocalizedMessage() );
		}

		return new LibvirtVirtualMachine( libvirtDomain );
	}

	public void deregisterVm( LibvirtVirtualMachine vm )
			throws LibvirtHypervisorException, LibvirtVirtualMachineException
	{
		// stop virtual machine if machine is running
		if ( vm.isRunning() ) {
			vm.stop();
		}

		// deregister and remove virtual machine from hypervisor
		try {
			vm.getLibvirtDomain().undefine();
		} catch ( LibvirtException e ) {
			throw new LibvirtHypervisorException( e.getLocalizedMessage() );
		}
	}

	@Override
	public void close()
	{
		try {
			this.hypervisor.close();
		} catch ( LibvirtException e ) {
			e.printStackTrace();
		}
	}
}
