package org.openslx.runvirt.viewer;

import org.openslx.runvirt.virtualization.LibvirtHypervisor;
import org.openslx.runvirt.virtualization.LibvirtHypervisorException;
import org.openslx.runvirt.virtualization.LibvirtVirtualMachine;
import org.openslx.virtualization.Version;

/**
 * Virtual Machine Manager (virt-manager) to control and view a display of a virtual machine.
 * 
 * @author Manuel Bentele
 * @version 1.0
 */
public class ViewerVirtManager extends Viewer
{
	/**
	 * Name of the Virtual Machine Manager program.
	 */
	private final static String NAME = "virt-manager";

	/**
	 * Maximum number of supported displays by the Virtual Machine Manager.
	 */
	private final static int NUM_SUPPORTED_DISPLAYS = 1;

	/**
	 * Creates a new Virtual Machine Manager for a virtual machine running on a hypervisor.
	 * 
	 * @param machine virtual machine to display.
	 * @param hypervisor remote (hypervisor) endpoint for the viewer to connect to.
	 */
	public ViewerVirtManager( LibvirtVirtualMachine machine, LibvirtHypervisor hypervisor )
	{
		super( ViewerVirtManager.NAME, ViewerVirtManager.NUM_SUPPORTED_DISPLAYS, machine, hypervisor );
	}

	@Override
	public Version getVersion() throws ViewerException
	{
		// execute viewer process with arguments: 
		// "virt-manager --version"
		final String versionOutput = ViewerUtils.executeViewer( ViewerVirtManager.NAME,
				new String[] { "--version" } );

		return Version.valueOf( versionOutput );
	}

	@Override
	public void render() throws ViewerException
	{
		String connectionUri = null;
		String machineUuid = null;

		// get URI of the hypervisor connection and UUID of the machine
		try {
			connectionUri = this.getHypervisor().getConnectionUri();
			machineUuid = this.getMachine().getConfiguration().getUuid();
		} catch ( LibvirtHypervisorException e ) {
			throw new ViewerException(
					"Failed to retrieve the URI of the hypervisor backend or the UUID of the machine to display: "
							+ e.getLocalizedMessage() );
		}

		// check if URI of the hypervisor connection and UUID of the machine is specified, otherwise abort
		if ( connectionUri == null || connectionUri.isEmpty() || machineUuid == null || machineUuid.isEmpty() ) {
			throw new ViewerException(
					"The URI of the hypervisor backend or the UUID of the machine to display is missing!" );
		}

		// execute viewer process with arguments:
		// "virt-viewer --connect=<URI> --show-domain-console <DOMAIN-UUID>"
		ViewerUtils.executeViewer( ViewerVirtManager.NAME,
				new String[] { "--connect=" + connectionUri, "--show-domain-console", machineUuid } );
	}
}
