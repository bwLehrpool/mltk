package org.openslx.runvirt.plugin.qemu.configuration;

import org.openslx.libvirt.domain.Domain;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.runvirt.plugin.qemu.virtualization.LibvirtHypervisorQemu;
import org.openslx.virtualization.configuration.transformation.TransformationException;
import org.openslx.virtualization.configuration.transformation.TransformationSpecific;

public class TransformationSpecificQemuNvidiaGpuPassthrough extends TransformationSpecific<Domain, CommandLineArgs, LibvirtHypervisorQemu>
{
	private static final String FILTER_NAME = "QEMU GPU passthrough [Nvidia]";

	public TransformationSpecificQemuNvidiaGpuPassthrough( LibvirtHypervisorQemu hypervisor )
	{
		super( TransformationSpecificQemuNvidiaGpuPassthrough.FILTER_NAME, hypervisor );
	}

	@Override
	public void transform( Domain config, CommandLineArgs args ) throws TransformationException
	{
		// check if IOMMU support is available on the host
		
		// TODO: implement Nvidia hypervisor shadowing
		// call this filter at the end, since -> override of software graphics to 'none' necessary
	}
}
