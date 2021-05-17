package org.openslx.runvirt.viewer;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.openslx.runvirt.virtualization.LibvirtHypervisor;
import org.openslx.runvirt.virtualization.LibvirtHypervisorException;
import org.openslx.runvirt.virtualization.LibvirtVirtualMachine;
import org.openslx.virtualization.Version;

/**
 * Virtual Viewer (virt-viewer) to view one or several displays of a virtual machine.
 * 
 * @author Manuel Bentele
 * @version 1.0
 */
public class ViewerVirtViewer extends Viewer
{
	/**
	 * Name of the Virtual Machine Manager program.
	 */
	private final static String NAME = "virt-viewer";

	/**
	 * Maximum number of supported displays by the Virtual Viewer.
	 */
	private final static int NUM_SUPPORTED_DISPLAYS = Integer.MAX_VALUE;

	/**
	 * Creates a new Virtual Viewer for a Libvirt virtual machine running on a Libvirt hypervisor.
	 * 
	 * @param machine virtual machine to display.
	 * @param hypervisor remote (hypervisor) endpoint for the viewer to connect to.
	 */
	public ViewerVirtViewer( LibvirtVirtualMachine machine, LibvirtHypervisor hypervisor )
	{
		super( ViewerVirtViewer.NAME, ViewerVirtViewer.NUM_SUPPORTED_DISPLAYS, machine, hypervisor );
	}

	@Override
	public Version getVersion() throws ViewerException
	{
		final Version version;

		// execute viewer process with arguments: 
		// "virt-viewer --version"
		final String versionOutput = ViewerUtils.executeViewer( ViewerVirtViewer.NAME,
				new String[] { "--version" } );

		if ( versionOutput == null ) {
			version = null;
		} else {
			// parse version from the viewer's process output
			final Pattern viewerVersionPattern = Pattern.compile( "(\\d+).(\\d+)" );
			final Matcher viewerVersionMatcher = viewerVersionPattern.matcher( versionOutput );

			// check if version pattern was found
			if ( viewerVersionMatcher.find() ) {
				final short major = Short.valueOf( viewerVersionMatcher.group( 1 ) );
				final short minor = Short.valueOf( viewerVersionMatcher.group( 2 ) );
				version = new Version( major, minor );
			} else {
				version = null;
			}
		}

		return version;
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
		// "virt-viewer --full-screen --reconnect --wait --attach --connect=<URI> --domain-name -- <DOMAIN-UUID>"
		ViewerUtils.executeViewer( ViewerVirtViewer.NAME, new String[] { "--full-screen", "--reconnect", "--wait",
				"--attach", "--connect=" + connectionUri, "--uuid", "--", machineUuid } );
	}
}
