package org.openslx.runvirt.plugin.qemu.virtualization;

import org.openslx.runvirt.virtualization.LibvirtHypervisor;
import org.openslx.runvirt.virtualization.LibvirtHypervisorException;

public class LibvirtHypervisorQemu extends LibvirtHypervisor
{
	public LibvirtHypervisorQemu( QemuSessionType type ) throws LibvirtHypervisorException
	{
		super( type.getConnectionUri() );
	}

	public enum QemuSessionType
	{
		// @formatter:off
		LOCAL_SYSTEM_SESSION( "qemu:///system" ),
		LOCAL_USER_SESSION  ( "qemu:///session" );
		// @formatter:on

		private final String connectionUri;

		QemuSessionType( String connectionUri )
		{
			this.connectionUri = connectionUri;
		}

		public String getConnectionUri()
		{
			return this.connectionUri;
		}
		
		// TODO:
		// Implement capabilities -> get host architecture => decision whether to emulate or use KVM? -> change domain of XML
		// fill in given HDD file, CDROM, ...
		// GPU-Passthrough: patch XML with hypervisor disable bit, ..., to get Nvidia driver working
		// Add hostdev für GPU passthrough -> add PCI ID arguments to cmdln parser
		// 
	}
}
