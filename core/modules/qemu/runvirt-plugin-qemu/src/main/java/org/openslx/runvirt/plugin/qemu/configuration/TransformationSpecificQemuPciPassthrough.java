package org.openslx.runvirt.plugin.qemu.configuration;

import java.math.BigInteger;
import java.util.ArrayList;
import java.util.List;

import org.openslx.libvirt.capabilities.Capabilities;
import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.device.Device;
import org.openslx.libvirt.domain.device.Graphics.ListenType;
import org.openslx.libvirt.domain.device.GraphicsSpice;
import org.openslx.libvirt.domain.device.HostdevPci;
import org.openslx.libvirt.domain.device.HostdevPciDeviceAddress;
import org.openslx.libvirt.domain.device.HostdevPciDeviceDescription;
import org.openslx.libvirt.domain.device.Shmem;
import org.openslx.libvirt.domain.device.Video;
import org.openslx.runvirt.plugin.qemu.Util;
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
public class TransformationSpecificQemuPciPassthrough
		extends TransformationSpecific<Domain, CommandLineArgs, LibvirtHypervisorQemu>
{
	/**
	 * Name of the configuration transformation.
	 */
	private static final String NAME = "QEMU GPU passthrough [Nvidia]";

	/**
	 * Vendor identifier of PCI devices from Nvidia.
	 */
	public static final int NVIDIA_PCI_VENDOR_ID = 0x10de;

	/**
	 * Switch to turn patch for Nvidia GPU-Passthrough (enables Hyper-V enlightening) on or off to
	 * avoid driver error code 43 in guest system.
	 */
	public static final boolean NVIDIA_PATCH = true;

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
	
	private final boolean withLookingGlass;

	/**
	 * Creates a new Nvidia GPU passthrough transformation for Libvirt/QEMU virtualization
	 * configurations.
	 * 
	 * @param hypervisor Libvirt/QEMU hypervisor.
	 */
	public TransformationSpecificQemuPciPassthrough( LibvirtHypervisorQemu hypervisor, boolean withLookingGlass )
	{
		super( TransformationSpecificQemuPciPassthrough.NAME, hypervisor );
		this.withLookingGlass = withLookingGlass;
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
						"Arguments of PCI IDs do not follow the pattern for a GPU passthrough!" );
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

		TransformationSpecificQemuPciPassthrough.validateParseNvidiaPciIds( args.getVmNvGpuIds0() );
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

	/**
	 * Calculates the framebuffer memory size for the Looking Glass shared memory device.
	 * 
	 * @return framebuffer memory size in bytes for the Looking Glass shared memory device.
	 */
	private static long calculateFramebufferSize()
	{
		final long totalBytesFramebuffer = MAX_DISPLAY_WIDTH * MAX_DISPLAY_HEIGHT * 4 * 2;
		final long totalBytesReserved = RESERVED_MEMORY_FRAMEBUFFER * 1048576;

		// round sum of total memory in bytes to nearest power of two
		return Util.roundToNearestPowerOf2( totalBytesFramebuffer + totalBytesReserved );
	}

	@Override
	public void transform( Domain config, CommandLineArgs args ) throws TransformationException
	{
		// validate configuration and input arguments
		this.validateInputs( config, args );

		// validate submitted PCI IDs
		final List<HostdevPciDeviceAddress> pciDeviceAddresses = TransformationSpecificQemuPciPassthrough
				.validateParseNvidiaPciIds( args.getVmNvGpuIds0() );

		if ( pciDeviceAddresses.isEmpty() )
			return; // Nothing to do

		// check if IOMMU support is available on the host
		if ( !this.getCapabilities().hasHostIommuSupport() ) {
			final String errorMsg = "IOMMU support is not available on the hypervisor but required for GPU passthrough!";
			throw new TransformationException( errorMsg );
		}
		// Check config for PCI addresses already in use
		int inUse[] = new int[ 64 ];
		inUse[0] = Integer.MAX_VALUE;
		inUse[1] = Integer.MAX_VALUE;
		for ( Device dev : config.getDevices() ) {
			HostdevPciDeviceAddress target = dev.getPciTarget();
			if ( target == null )
				continue;
			if ( target.getPciDomain() != 0 || target.getPciBus() != 0 )
				continue; // Ignore non-primary bus
			if ( target.getPciDevice() >= inUse.length )
				continue;
			inUse[target.getPciDevice()] = Integer.MAX_VALUE;
		}

		// passthrough PCI devices of the GPU
		for ( final HostdevPciDeviceAddress pciDeviceAddress : pciDeviceAddresses ) {
			final HostdevPci pciDevice = config.addHostdevPciDevice();
			pciDevice.setManaged( true );
			pciDevice.setSource( pciDeviceAddress );
			if ( pciDeviceAddress.getPciFunction() == 0 && pciDeviceAddresses.size() > 1 ) {
				pciDevice.setMultifunction( true );
			}
			int devAddr = getFreeAddr( inUse, pciDeviceAddress );
			pciDevice.setPciTarget( new HostdevPciDeviceAddress( 0, devAddr, pciDeviceAddress.getPciFunction() ) );
		}

		// check if passthrough of Nvidia GPU takes place
		if ( this.withLookingGlass ) {
			// add shared memory device for Looking Glass
			final Shmem shmemDevice = config.addShmemDevice();
			shmemDevice.setName( "looking-glass" );
			shmemDevice.setModel( Shmem.Model.IVSHMEM_PLAIN );
			shmemDevice.setSize( BigInteger.valueOf( TransformationSpecificQemuPciPassthrough.calculateFramebufferSize() ) );

			// disable all software video devices if device passthrough debug mode is not enabled
			if ( !args.isDebugDevicePassthroughEnabled() ) {
				for ( Video videoDevice : config.getVideoDevices() ) {
					videoDevice.disable();
				}
			}

			// force SPICE graphics to listen on local address for looking-glass-client
			for ( int i = 0; i < config.getGraphicSpiceDevices().size(); i++ ) {
				final GraphicsSpice graphicsSpiceDevice = config.getGraphicSpiceDevices().get( i );
				graphicsSpiceDevice.setListenType( ListenType.ADDRESS );
				graphicsSpiceDevice.setListenAddress( GraphicsSpice.DEFAULT_ADDRESS );
				graphicsSpiceDevice.setListenPort( GraphicsSpice.DEFAULT_PORT + i );
			}
		}
		if ( args.isNvidiaGpuPassthroughEnabled() ) {
			// enable hypervisor shadowing to avoid error code 43 of Nvidia drivers in virtual machines
			if ( TransformationSpecificQemuPciPassthrough.NVIDIA_PATCH ) {
				config.setFeatureHypervVendorIdValue( TransformationSpecificQemuPciPassthrough.HYPERV_VENDOR_ID );
				config.setFeatureHypervVendorIdState( true );
				config.setFeatureKvmHiddenState( true );
			}
		}
	}

	private int getFreeAddr( int[] inUse, HostdevPciDeviceAddress pciDeviceAddress )
	{
		// Use first free one. Usually 00:02:00 is primary VGA
		int devAddr;
		int firstFree = -1;
		int lookup = (pciDeviceAddress.getPciDomain() << 16)
				| (pciDeviceAddress.getPciBus() << 8)
				| (pciDeviceAddress.getPciDevice());
		for ( devAddr = 0; devAddr < inUse.length; ++devAddr ) {
			if ( firstFree == -1 && inUse[devAddr] == 0 ) {
				firstFree = devAddr;
			} else if ( inUse[devAddr] == lookup ) {
				return devAddr;
			}
		}
		inUse[firstFree] = lookup;
		return firstFree;
	}
}
