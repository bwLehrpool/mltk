package org.openslx.runvirt.plugin.qemu.virtualization;

import org.openslx.runvirt.virtualization.LibvirtHypervisor;
import org.openslx.runvirt.virtualization.LibvirtHypervisorException;

/**
 * Representation of the Libvirt QEMU hypervisor backend.
 * 
 * @author Manuel Bentele
 * @version 1.0
 */
public class LibvirtHypervisorQemu extends LibvirtHypervisor
{
	/**
	 * Creates a new Libvirt QEMU hypervisor backend and connects to the specified backend.
	 * 
	 * @param type session type of the connection to the Libvirt QEMU hypervisor backend.
	 * @throws LibvirtHypervisorException failed to connect to the Libvirt QEMU hypervisor backend.
	 */
	public LibvirtHypervisorQemu( QemuSessionType type ) throws LibvirtHypervisorException
	{
		super( type.getConnectionUri() );
	}

	/**
	 * Type of Libvirt QEMU hypervisor backend session.
	 * 
	 * @author Manuel Bentele
	 * @version 1.0
	 */
	public enum QemuSessionType
	{
		// @formatter:off
		LOCAL_SYSTEM_SESSION( "qemu:///system" ),
		LOCAL_USER_SESSION  ( "qemu:///session" );
		// @formatter:on

		/**
		 * Connection URI of the QEMU session type.
		 */
		private final String connectionUri;

		/**
		 * Creates a new QEMU session type.
		 * 
		 * @param connectionUri URI for the connection of the session.
		 */
		QemuSessionType( String connectionUri )
		{
			this.connectionUri = connectionUri;
		}

		/**
		 * Returns the URI of the connection for the session.
		 * 
		 * @return URI of the connection for the session.
		 */
		public String getConnectionUri()
		{
			return this.connectionUri;
		}
	}
}
