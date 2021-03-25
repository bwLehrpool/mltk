package org.openslx.runvirt.plugin.qemu.configuration;

import org.openslx.libvirt.domain.Domain;
import org.openslx.runvirt.configuration.FilterException;
import org.openslx.runvirt.configuration.FilterSpecific;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.runvirt.plugin.qemu.virtualization.LibvirtHypervisorQemu;

public class FilterSpecificQemuNvidiaGpuPassthrough extends FilterSpecific<Domain, CommandLineArgs, LibvirtHypervisorQemu>
{
	private static final String FILTER_NAME = "QEMU GPU passthrough [Nvidia]";

	public FilterSpecificQemuNvidiaGpuPassthrough( LibvirtHypervisorQemu hypervisor )
	{
		super( FilterSpecificQemuNvidiaGpuPassthrough.FILTER_NAME, hypervisor );
	}

	@Override
	public void filter( Domain config, CommandLineArgs args ) throws FilterException
	{
		// check if IOMMU support is available on the host
		
		// TODO: implement Nvidia hypervisor shadowing
		// call this filter at the end, since -> override of software graphics to 'none' necessary
	}
}
