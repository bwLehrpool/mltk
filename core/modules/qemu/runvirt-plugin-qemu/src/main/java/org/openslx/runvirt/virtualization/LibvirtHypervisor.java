package org.openslx.runvirt.virtualization;

import java.io.Closeable;

import org.libvirt.Connect;
import org.libvirt.LibvirtException;
import org.openslx.libvirt.capabilities.Capabilities;
import org.openslx.libvirt.xml.LibvirtXmlDocumentException;
import org.openslx.libvirt.xml.LibvirtXmlSerializationException;
import org.openslx.libvirt.xml.LibvirtXmlValidationException;
import org.openslx.virtualization.Version;

/**
 * Representation of a Libvirt hypervisor backend (e.g. QEMU or VMware).
 * <p>
 * The representation allows to connect to a running Libvirt service and query the host system's
 * capabilities or manage virtual machines.
 * 
 * @implNote This class is the abstract representation to implement various Libvirt hypervisor
 *           backends using inheritance.
 * 
 * @author Manuel Bentele
 * @version 1.0
 */
public abstract class LibvirtHypervisor implements Closeable
{
	/**
	 * Connection to a Libvirt hypervisor backend.
	 */
	protected Connect hypervisor = null;

	/**
	 * Creates a new Libvirt hypervisor backend specified by an URI and connects to the specified
	 * backend.
	 * 
	 * @param connectionUri URI of a specific Libvirt hypervisor backend.
	 * @throws LibvirtHypervisorException failed to connect to the specified Libvirt hypervisor
	 *            backend.
	 */
	public LibvirtHypervisor( String connectionUri ) throws LibvirtHypervisorException
	{
		this.connect( connectionUri );
	}

	/**
	 * Connects to the Libvirt hypervisor backend specified by an URI.
	 * 
	 * @param connectionUri URI of a specific Libvirt hypervisor backend.
	 * @throws LibvirtHypervisorException failed to connect to the specified Libvirt hypervisor
	 *            backend.
	 */
	protected void connect( String connectionUri ) throws LibvirtHypervisorException
	{
		try {
			this.hypervisor = new Connect( connectionUri );
		} catch ( LibvirtException e ) {
			throw new LibvirtHypervisorException( e.getLocalizedMessage() );
		}
	}

	/**
	 * Returns the URI of the connection to the Libvirt hypervisor backend.
	 * 
	 * @return URI of the connection to the hypervisor.
	 * @throws LibvirtHypervisorException failed to return the connection URI of the Libvirt
	 *            hypervisor backend.
	 */
	public String getConnectionUri() throws LibvirtHypervisorException
	{
		String connectionUri = null;

		try {
			connectionUri = this.hypervisor.getURI();
		} catch ( LibvirtException e ) {
			throw new LibvirtHypervisorException( e.getLocalizedMessage() );
		}

		return connectionUri;
	}

	/**
	 * Returns the queried Libvirt hypervisor's host system capabilities.
	 * 
	 * @return queried Libvirt hypervisor's host system capabilities.
	 * @throws LibvirtHypervisorException failed to query and return the Libvirt hypervisor's host
	 *            system capabilities.
	 */
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

	/**
	 * Returns the version of the Libvirt hypervisor backend.
	 * 
	 * @return version of the Libvirt hypervisor backend.
	 * @throws LibvirtHypervisorException failed to get the version of the Libvirt hypervisor
	 *            backend.
	 */
	public Version getVersion() throws LibvirtHypervisorException
	{
		long hypervisorVersionRaw = 0;
		Version hypervisorVersion = null;

		try {
			hypervisorVersionRaw = this.hypervisor.getVersion();
		} catch ( LibvirtException e ) {
			throw new LibvirtHypervisorException( e.getLocalizedMessage() );
		}

		if ( hypervisorVersionRaw > 0 ) {
			final short major = Long.valueOf( hypervisorVersionRaw / Long.valueOf( 1000000 ) ).shortValue();
			hypervisorVersionRaw %= Long.valueOf( 1000000 );
			final short minor = Long.valueOf( hypervisorVersionRaw / Long.valueOf( 1000 ) ).shortValue();
			hypervisorVersion = new Version( major, minor );
		}

		return hypervisorVersion;
	}

	/**
	 * Register a virtual machine by the Libvirt hypervisor based on a virtualization configuration.
	 * 
	 * @param vmConfiguration virtualization configuration for the virtual machine.
	 * @return instance of the registered and defined virtual machine.
	 * @throws LibvirtHypervisorException failed to register and define virtual machine.
	 */
	public LibvirtVirtualMachine registerVm( org.openslx.libvirt.domain.Domain vmConfiguration )
			throws LibvirtHypervisorException
	{
		final String xmlVmConfiguration = vmConfiguration.toString();
		org.libvirt.Domain internalConfiguration = null;

		try {
			internalConfiguration = this.hypervisor.domainDefineXML( xmlVmConfiguration );
		} catch ( LibvirtException e ) {
			throw new LibvirtHypervisorException( e.getLocalizedMessage() );
		}

		return new LibvirtVirtualMachine( internalConfiguration, vmConfiguration );
	}

	/**
	 * Deregisters an already registered virtual machine by the Libvirt hypervisor.
	 * 
	 * @param vm virtual machine that should be deregistered
	 * @throws LibvirtHypervisorException failed to deregister virtual machine by the Libvirt
	 *            hypervisor.
	 * @throws LibvirtVirtualMachineException failed to check and stop the virtual machine.
	 */
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
