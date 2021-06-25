package org.openslx.runvirt.plugin.qemu.configuration;

import java.math.BigInteger;
import java.util.ArrayList;
import java.util.List;

import org.openslx.libvirt.capabilities.Capabilities;
import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.device.HostdevPci;
import org.openslx.libvirt.domain.device.HostdevPciDeviceAddress;
import org.openslx.libvirt.domain.device.HostdevPciDeviceDescription;
import org.openslx.libvirt.domain.device.Shmem;
import org.openslx.libvirt.domain.device.Video;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.runvirt.plugin.qemu.virtualization.LibvirtHypervisorQemu;
import org.openslx.runvirt.virtualization.LibvirtHypervisorException;
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
	 * Vendor identifier of PCI devices from Nvidia.
	 */
	private static final int NVIDIA_PCI_VENDOR_ID = 0x10de;

	/**
	 * Vendor identifier of the Hyper-V enlightenment for hypervisor shadowing.
	 */
	public static final String HYPERV_VENDOR_ID = "62776c706277";

	/**
	 * Maximum width in pixel of the GPU passthrough rendered display.
	 */
	private static final long MAX_DISPLAY_WIDTH = 2560;

	/**
	 * Maximum height in pixel of the GPU passthrough rendered display.
	 */
	private static final long MAX_DISPLAY_HEIGHT = 1440;

	/**
	 * Reserved memory for framebuffer meta data of the Looking Glass shared memory device in MiB.
	 */
	private static final long RESERVED_MEMORY_FRAMEBUFFER = 10;

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
	 * Validates a PCI device description and address of a PCI device from a Nvidia GPU and parses
	 * the validated PCI device addresses.
	 * 
	 * @param pciIds textual PCI device description and address to be validated.
	 * 
	 * @return list of validated and parsed PCI device addresses for a NVIDIA GPU passthrough.
	 * 
	 * @throws TransformationException validation of PCI device description and address failed.
	 */
	private static List<HostdevPciDeviceAddress> validateParseNvidiaPciIds( List<String> pciIds )
			throws TransformationException
	{
		final List<HostdevPciDeviceAddress> parsedPciAddresses = new ArrayList<HostdevPciDeviceAddress>();

		if ( pciIds != null && pciIds.size() > 0 ) {
			// abort if arguments do not follow the pattern:
			//
			//   [0]: <VENDOR ID 0>:<DEVICE ID 0>
			//   [1]: <PCI DOMAIN 0>:<PCI BUS 0>:<PCI DEVICE 0>.<PCI FUNCTION 0>
			//   [2]: <VENDOR ID 1>:<DEVICE ID 1>
			//   [3]: <PCI DOMAIN 1>:<PCI BUS 1>:<PCI DEVICE 1>.<PCI FUNCTION 1>
			//        ...
			//
			if ( pciIds.size() % 2 != 0 ) {
				throw new TransformationException(
						"Arguments of PCI IDs are not follow the pattern for a GPU passthrough!" );
			}

			// parse PCI device description and PCI device address
			for ( int i = 0; i < pciIds.size(); i += 2 ) {
				// parse vendor and device ID
				HostdevPciDeviceDescription deviceDescription = null;
				try {
					deviceDescription = HostdevPciDeviceDescription.valueOf( pciIds.get( i ) );
				} catch ( IllegalArgumentException e ) {
					throw new TransformationException( "Invalid vendor or device ID of the PCI device description!" );
				}

				// validate vendor ID
				final int vendorId = deviceDescription.getVendorId();
				if ( TransformationSpecificQemuGpuPassthroughNvidia.NVIDIA_PCI_VENDOR_ID != vendorId ) {
					final String errorMsg = "Vendor ID '" + vendorId + "' of the PCI device is not from Nvidia!";
					throw new TransformationException( errorMsg );
				}

				// parse PCI domain, PCI bus, PCI device and PCI function
				final HostdevPciDeviceAddress parsedPciAddress = HostdevPciDeviceAddress.valueOf( pciIds.get( i + 1 ) );
				if ( parsedPciAddress != null ) {
					parsedPciAddresses.add( parsedPciAddress );
				}
			}
		}

		return parsedPciAddresses;
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

		TransformationSpecificQemuGpuPassthroughNvidia.validateParseNvidiaPciIds( args.getVmNvGpuIds0() );
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

	private static BigInteger roundToNearestPowerOf2( BigInteger value )
	{
		BigInteger k = BigInteger.valueOf( 1 );

		while ( k.compareTo( value ) == -1 ) {
			k = k.multiply( BigInteger.valueOf( 2 ) );
		}

		return k;
	}

	/**
	 * Calculates the framebuffer memory size for the Looking Glass shared memory device.
	 * 
	 * @return framebuffer memory size in bytes for the Looking Glass shared memory device.
	 */
	private static BigInteger calculateFramebufferSize()
	{
		final long totalBytesFramebuffer = MAX_DISPLAY_WIDTH * MAX_DISPLAY_HEIGHT * 4 * 2;
		final long totalBytesReserved = RESERVED_MEMORY_FRAMEBUFFER * 1048576;

		// round sum of total memory in bytes to nearest power of two
		return roundToNearestPowerOf2( BigInteger.valueOf( totalBytesFramebuffer + totalBytesReserved ) );
	}

	@Override
	public void transform( Domain config, CommandLineArgs args ) throws TransformationException
	{
		// validate configuration and input arguments
		this.validateInputs( config, args );

		// check if passthrough of Nvidia GPU takes place
		if ( args.isNvidiaGpuPassthroughEnabled() ) {
			// validate submitted PCI IDs
			final List<HostdevPciDeviceAddress> pciDeviceAddresses = TransformationSpecificQemuGpuPassthroughNvidia
					.validateParseNvidiaPciIds( args.getVmNvGpuIds0() );

			// check if IOMMU support is available on the host
			if ( !this.getCapabilities().hasHostIommuSupport() ) {
				final String errorMsg = "IOMMU support is not available on the hypervisor but required for GPU passthrough!";
				throw new TransformationException( errorMsg );
			}

			// passthrough PCI devices of the GPU
			for ( final HostdevPciDeviceAddress pciDeviceAddress : pciDeviceAddresses ) {
				final HostdevPci pciDevice = config.addHostdevPciDevice();
				pciDevice.setManaged( true );
				pciDevice.setSource( pciDeviceAddress );
			}

			// add shared memory device for Looking Glass
			final Shmem shmemDevice = config.addShmemDevice();
			shmemDevice.setName( "looking-glass" );
			shmemDevice.setModel( Shmem.Model.IVSHMEM_PLAIN );
			shmemDevice.setSize( TransformationSpecificQemuGpuPassthroughNvidia.calculateFramebufferSize() );

			// enable hypervisor shadowing to avoid error code 43 of Nvidia drivers in virtual machines
			config.setFeatureHypervVendorIdValue( TransformationSpecificQemuGpuPassthroughNvidia.HYPERV_VENDOR_ID );
			config.setFeatureHypervVendorIdState( true );
			config.setFeatureKvmHiddenState( true );

			// disable all software video devices by disable them
			for ( Video videoDevice : config.getVideoDevices() ) {
				videoDevice.disable();
			}
		}
	}
}
