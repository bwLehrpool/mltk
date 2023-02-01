package org.openslx.runvirt.plugin.qemu.configuration;

import org.openslx.libvirt.capabilities.Capabilities;
import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.device.GraphicsSpice;
import org.openslx.libvirt.domain.device.Hostdev;
import org.openslx.libvirt.domain.device.HostdevMdev;
import org.openslx.libvirt.domain.device.HostdevMdev.Model;
import org.openslx.libvirt.domain.device.HostdevMdevDeviceAddress;
import org.openslx.libvirt.domain.device.Video;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.runvirt.plugin.qemu.virtualization.LibvirtHypervisorQemu;
import org.openslx.runvirt.virtualization.LibvirtHypervisorException;
import org.openslx.virtualization.configuration.transformation.TransformationException;
import org.openslx.virtualization.configuration.transformation.TransformationSpecific;

/**
 * Specific Intel mediated device (Intel GVT-g) passthrough transformation for Libvirt/QEMU
 * virtualization configurations.
 * 
 * @author Manuel Bentele
 * @version 1.0
 */
public class TransformationSpecificQemuMdevPassthroughIntel
		extends TransformationSpecific<Domain, CommandLineArgs, LibvirtHypervisorQemu>
{
	/**
	 * Name of the configuration transformation.
	 */
	private static final String NAME = "QEMU mediated device passthrough [Intel]";

	/**
	 * Path to the UEFI rom file for Intel GVT-g instances.
	 */
	private static final String INTEL_GVT_G_UEFI_ROMFILE = "/usr/share/qemu/vbios_gvt_uefi.rom";

	/**
	 * Creates a new Intel mediated device passthrough transformation for Libvirt/QEMU virtualization
	 * configurations.
	 * 
	 * @param hypervisor Libvirt/QEMU hypervisor.
	 */
	public TransformationSpecificQemuMdevPassthroughIntel( LibvirtHypervisorQemu hypervisor )
	{
		super( TransformationSpecificQemuMdevPassthroughIntel.NAME, hypervisor );
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

	/**
	 * Queries and returns the capabilities of the Libvirt/QEMU hypervisor.
	 * 
	 * @return capabilities of the Libvirt/QEMU hypervisor.
	 * @throws TransformationException failed to query and return the capabilities of the
	 *            Libvirt/QEMU hypervisor.
	 */
	protected Capabilities getCapabilities() throws TransformationException
	{
		Capabilities capabilities = null;

		try {
			capabilities = this.getVirtualizer().getCapabilites();
		} catch ( LibvirtHypervisorException e ) {
			final String errorMsg = new String(
					"Failed to retrieve host capabilities from QEMU virtualizer: " + e.getLocalizedMessage() );
			throw new TransformationException( errorMsg );
		}

		return capabilities;
	}

	@Override
	public void transform( Domain config, CommandLineArgs args ) throws TransformationException
	{
		// validate configuration and input arguments
		this.validateInputs( config, args );

		// check if passthrough of an Intel mediated device (virtual GPU) takes place
		if ( args.isIntelMdevPassthroughEnabled() ) {
			// validate submitted mediated device UUID
			final HostdevMdevDeviceAddress mdevDeviceAddress = HostdevMdevDeviceAddress.valueOf( args.getVmIlMdevId0() );
			if ( mdevDeviceAddress == null ) {
				final String errorMsg = "UUID of the Intel mediated device (virtual GPU) address is invalid!";
				throw new TransformationException( errorMsg );
			}

			// check if IOMMU support is available on the host
			if ( !this.getCapabilities().hasHostIommuSupport() ) {
				final String errorMsg = "IOMMU support is not available on the hypervisor but required for Intel mediated device (virtual GPU) passthrough!";
				throw new TransformationException( errorMsg );
			}

			// remove all existing hostdev devices
			// otherwise the Intel specific QEMU options with index 0 do not work
			for ( final Hostdev hostdevDevice : config.getHostdevDevices() ) {
				hostdevDevice.remove();
			}

			// passthrough Intel mediated device (virtual GPU)
			final HostdevMdev mdevDevice = config.addHostdevMdevDevice();
			mdevDevice.setManaged( false );
			mdevDevice.setModel( Model.VFIO_PCI );
			mdevDevice.setDisplayOn( true );
			mdevDevice.setMemoryFramebufferOn( true );
			mdevDevice.setSource( mdevDeviceAddress );

			if ( config.getOsLoader() != null && !config.getOsLoader().isEmpty() ) {
				// set Intel specific rom file for GVT-g if UEFI loader is used
				config.addGvtg( INTEL_GVT_G_UEFI_ROMFILE );
			} else {
				config.addGvtg( null );
			}

			// disable all software video devices if device passthrough debug mode is not enabled
			if ( !args.isDebugDevicePassthroughEnabled() ) {
				for ( Video videoDevice : config.getVideoDevices() ) {
					videoDevice.disable();
				}
			}

			// enable OpenGL on all SPICE graphics devices
			for ( GraphicsSpice graphicsSpiceDevice : config.getGraphicSpiceDevices() ) {
				graphicsSpiceDevice.setOpenGl( true );
			}
		}
	}
}
