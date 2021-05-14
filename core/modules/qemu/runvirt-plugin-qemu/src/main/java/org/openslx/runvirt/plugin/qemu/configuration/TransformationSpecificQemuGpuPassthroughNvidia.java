package org.openslx.runvirt.plugin.qemu.configuration;

import org.openslx.libvirt.domain.Domain;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.runvirt.plugin.qemu.virtualization.LibvirtHypervisorQemu;
import org.openslx.virtualization.configuration.transformation.TransformationException;
import org.openslx.virtualization.configuration.transformation.TransformationSpecific;

/**
 * Specific Nvidia GPU passthrough transformation for Libvirt/QEMU virtualization configurations.
 * 
 * @author Manuel Bentele
 * @version 1.0
 */
public class TransformationSpecificQemuGpuPassthroughNvidia
		extends TransformationSpecific<Domain, CommandLineArgs, LibvirtHypervisorQemu>
{
	/**
	 * Name of the configuration transformation.
	 */
	private static final String NAME = "QEMU GPU passthrough [Nvidia]";

	/**
	 * Creates a new Nvidia GPU passthrough transformation for Libvirt/QEMU virtualization
	 * configurations.
	 * 
	 * @param hypervisor Libvirt/QEMU hypervisor.
	 */
	public TransformationSpecificQemuGpuPassthroughNvidia( LibvirtHypervisorQemu hypervisor )
	{
		super( TransformationSpecificQemuGpuPassthroughNvidia.NAME, hypervisor );
	}

	/**
	 * Validates a virtualization configuration and input arguments for this transformation.
	 * 
	 * @param config virtualization configuration for the validation.
	 * @param args input arguments for the validation.
	 * @throws TransformationException validation has failed.
	 */
	private void validateInputs( Domain config, CommandLineArgs args ) throws TransformationException
	{
		if ( config == null || args == null ) {
			throw new TransformationException( "Virtualization configuration or input arguments are missing!" );
		}
	}

	@Override
	public void transform( Domain config, CommandLineArgs args ) throws TransformationException
	{
		// validate configuration and input arguments
		this.validateInputs( config, args );

		// check if IOMMU support is available on the host

		// TODO: implement Nvidia hypervisor shadowing
		// call this filter at the end, since -> override of software graphics to 'none' necessary
	}
}
