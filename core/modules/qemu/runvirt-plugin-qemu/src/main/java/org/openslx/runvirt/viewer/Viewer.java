package org.openslx.runvirt.viewer;

import org.openslx.runvirt.virtualization.LibvirtHypervisor;
import org.openslx.runvirt.virtualization.LibvirtVirtualMachine;
import org.openslx.virtualization.Version;

/**
 * Representation of an viewer for virtual machines running on a host system.
 * 
 * @author Manuel Bentele
 * @version 1.0
 */
public abstract class Viewer
{
	/**
	 * Name of the viewer.
	 */
	private final String name;

	/**
	 * Number of supported displays by the viewer.
	 */
	private final int numSupportedDisplays;

	/**
	 * The virtual machine to display.
	 */
	private final LibvirtVirtualMachine machine;

	/**
	 * Remote (hypervisor) endpoint for the viewer to connect to.
	 */
	private final LibvirtHypervisor hypervisor;

	/**
	 * Creates a new viewer for a Libvirt virtual machine running on a Libvirt hypervisor.
	 * 
	 * @param name textual name of the viewer.
	 * @param numSupportedDisplays number of supported displays by the viewer.
	 * @param machine virtual machine to display.
	 * @param hypervisor remote (hypervisor) endpoint for the viewer to connect to.
	 */
	public Viewer( String name, int numSupportedDisplays, LibvirtVirtualMachine machine, LibvirtHypervisor hypervisor )
	{
		this.name = name;
		this.numSupportedDisplays = numSupportedDisplays;
		this.machine = machine;
		this.hypervisor = hypervisor;
	}

	/**
	 * Returns the name of the viewer.
	 * 
	 * @return name of the viewer.
	 */
	public String getName()
	{
		return this.name;
	}

	/**
	 * Returns the number of supported displays by the viewer.
	 * 
	 * @return number of supported displays by the viewer.
	 */
	public int getNumberOfSupportedDisplays()
	{
		return this.numSupportedDisplays;
	}

	/**
	 * Returns the virtual machine to display.
	 * 
	 * @return virtual machine to display.
	 */
	public LibvirtVirtualMachine getMachine()
	{
		return this.machine;
	}

	/**
	 * Returns the remote (hypervisor) endpoint for the viewer to connect to.
	 * 
	 * @return remote (hypervisor) endpoint for the viewer to connect to.
	 */
	public LibvirtHypervisor getHypervisor()
	{
		return this.hypervisor;
	}

	/**
	 * Displays all virtual machine's displays.
	 * 
	 * @throws ViewerException failed to display all displays of a virtual machine.
	 * 
	 * @apiNote A call to this method blocks until the implemented {@link #render()} process
	 *          terminates.
	 */
	public void display() throws ViewerException
	{
		this.render();
	}

	/**
	 * Returns the version of the viewer.
	 * 
	 * @return version of the viewer.
	 * @throws ViewerException failed to get version of the viewer.
	 */
	public abstract Version getVersion() throws ViewerException;

	/**
	 * Renders the content of all displays from the virtual machine.
	 * 
	 * @throws ViewerException failed to render all displays of a virtual machine.
	 */
	protected abstract void render() throws ViewerException;
}
