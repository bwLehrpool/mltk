package org.openslx.runvirt.viewer;

import org.openslx.libvirt.domain.device.GraphicsSpice;
import org.openslx.runvirt.virtualization.LibvirtHypervisor;
import org.openslx.runvirt.virtualization.LibvirtVirtualMachine;
import org.openslx.virtualization.Version;

/**
 * Looking Glass Client to view the exposed framebuffer (through a shared memory) of a virtual
 * machine running the Looking Glass Host application.
 * 
 * @author Manuel Bentele
 * @version 1.0
 */
public class ViewerLookingGlassClient extends Viewer
{
	/**
	 * Name of the Looking Glass Client program.
	 */
	private final static String NAME = "looking-glass-client";

	/**
	 * Maximum number of supported displays by the Looking Glass Client.
	 */
	private final static int NUM_SUPPORTED_DISPLAYS = 1;

	/**
	 * File name of the shared memory file to receive display content from the Looking Glass Host.
	 */
	private final static String SHARED_MEMORY_FILENAME = "/dev/shm/looking-glass";

	/**
	 * State whether showing debug information during virtual machine rendering or not.
	 */
	private final boolean debug;

	/**
	 * Creates a new Looking Glass Client for a Libvirt virtual machine running on a Libvirt
	 * hypervisor.
	 * 
	 * @param machine virtual machine to display.
	 * @param hypervisor remote (hypervisor) endpoint for the viewer to connect to.
	 */
	public ViewerLookingGlassClient( LibvirtVirtualMachine machine, LibvirtHypervisor hypervisor )
	{
		this( machine, hypervisor, false );
	}

	/**
	 * Creates a new Looking Glass Client for a Libvirt virtual machine running on a Libvirt
	 * hypervisor.
	 * 
	 * @param machine virtual machine to display.
	 * @param hypervisor remote (hypervisor) endpoint for the viewer to connect to.
	 * @param debug state whether showing debug information during virtual machine rendering or not.
	 */
	public ViewerLookingGlassClient( LibvirtVirtualMachine machine, LibvirtHypervisor hypervisor, boolean debug )
	{
		super( ViewerLookingGlassClient.NAME, ViewerLookingGlassClient.NUM_SUPPORTED_DISPLAYS, machine, hypervisor );

		this.debug = debug;
	}

	/**
	 * Returns the state whether showing debug information during virtual machine rendering or not.
	 * 
	 * @return state whether showing debug information during virtual machine rendering or not.
	 */
	public boolean isDebugEnabled()
	{
		return this.debug;
	}

	@Override
	public Version getVersion() throws ViewerException
	{
		return null;
	}

	@Override
	public void render() throws ViewerException
	{
		// execute viewer process with arguments:
		//   in non-debug mode:
		//     "looking-glass-client app:shmFile=<SHARED-MEM-FILE> win:fullScreen=yes spice:enable=yes win:alerts=no"
		//   in debug mode:
		//     "looking-glass-client app:shmFile=<SHARED-MEM-FILE> win:fullScreen=yes spice:enable=yes win:alerts=yes win:showFPS=yes"
		final String[] viewerParameters;
		if ( this.isDebugEnabled() ) {
			viewerParameters = new String[] {
					"app:shmFile=" + ViewerLookingGlassClient.SHARED_MEMORY_FILENAME,
					"win:fullScreen=yes",
					"spice:enable=yes",
					"spice:host=" + GraphicsSpice.DEFAULT_ADDRESS,
					"spice:port=" + GraphicsSpice.DEFAULT_PORT,
					"win:alerts=no" };
		} else {
			viewerParameters = new String[] {
					"app:shmFile=" + ViewerLookingGlassClient.SHARED_MEMORY_FILENAME,
					"win:fullScreen=yes",
					"spice:enable=yes",
					"spice:host=" + GraphicsSpice.DEFAULT_ADDRESS,
					"spice:port=" + GraphicsSpice.DEFAULT_PORT,
					"win:alerts=yes",
					"win:showFPS=yes" };
		}

		ViewerUtils.executeViewer( ViewerLookingGlassClient.NAME, viewerParameters );
	}
}
