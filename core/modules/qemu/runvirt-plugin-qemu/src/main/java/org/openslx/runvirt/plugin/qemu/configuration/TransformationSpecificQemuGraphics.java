package org.openslx.runvirt.plugin.qemu.configuration;

import java.util.List;

import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.DomainUtils;
import org.openslx.libvirt.domain.device.Graphics.ListenType;
import org.openslx.libvirt.domain.device.GraphicsSpice;
import org.openslx.libvirt.domain.device.GraphicsSpice.ImageCompression;
import org.openslx.libvirt.domain.device.GraphicsSpice.StreamingMode;
import org.openslx.libvirt.domain.device.GraphicsVnc;
import org.openslx.libvirt.domain.device.HostdevPciDeviceAddress;
import org.openslx.libvirt.domain.device.Video;
import org.openslx.libvirt.domain.device.Video.Model;
import org.openslx.runvirt.plugin.qemu.Util;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.runvirt.plugin.qemu.virtualization.LibvirtHypervisorQemu;
import org.openslx.virtualization.configuration.transformation.TransformationException;
import org.openslx.virtualization.configuration.transformation.TransformationSpecific;

/**
 * Specific graphics transformation for Libvirt/QEMU virtualization configurations.
 * 
 * @author Manuel Bentele
 * @version 1.0
 */
public class TransformationSpecificQemuGraphics
		extends TransformationSpecific<Domain, CommandLineArgs, LibvirtHypervisorQemu>
{

	// TODO: Configurable, calculate in run-virt, maybe support multiple heads
	public static final int MIN_VGA_MEM = 48 * 1024;

	/**
	 * Name of the configuration transformation.
	 */
	private static final String NAME = "QEMU Graphics [GPU emulation, framebuffer, ...]";

	/**
	 * Creates a new graphics transformation for Libvirt/QEMU virtualization configurations.
	 * 
	 * @param hypervisor Libvirt/QEMU hypervisor.
	 */
	public TransformationSpecificQemuGraphics( LibvirtHypervisorQemu virtualizer )
	{
		super( TransformationSpecificQemuGraphics.NAME, virtualizer );
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
		if ( config == null ) {
			throw new TransformationException( "Virtualization configuration is missing!" );
		}
	}

	/**
	 * Add a SPICE graphics device configured for local access through a Unix domain socket.
	 * 
	 * @param config virtualization configuration into that the SPICE graphics device is added.
	 * @param openGlEnabled state whether OpenGL is enabled or not.
	 * 
	 * @return created SPICE graphics device instance.
	 */
	private GraphicsSpice addLocalSpiceGraphics( Domain config, boolean openGlEnabled )
	{
		final GraphicsSpice newGraphicsSpiceDevice = config.addGraphicsSpiceDevice();

		// select local Unix domain socket access
		newGraphicsSpiceDevice.setListenType( ListenType.NONE );

		// disable optimizations (compressions) for local usage
		newGraphicsSpiceDevice.setImageCompression( ImageCompression.OFF );
		newGraphicsSpiceDevice.setPlaybackCompression( false );
		newGraphicsSpiceDevice.setStreamingMode( StreamingMode.OFF );

		// enable OpenGL acceleration if necessary
		newGraphicsSpiceDevice.setOpenGl( openGlEnabled );

		return newGraphicsSpiceDevice;
	}

	/**
	 * Adjust display devices to match requested count and ensure proper memory settings.
	 * - If displayCount > 0: add or remove QXL devices to match exactly
	 * - If displayCount == 0: keep existing devices, just adjust memory settings
	 *
	 * @param config domain configuration to adjust
	 * @param displayCount requested number of display adapters (0 = auto/keep existing)
	 */
	private void adjustDisplayDevices( Domain config, int displayCount )
	{
		// adjust vgamem, so higher resolutions work properly
		List<Video> list = config.getVideoDevices();
		for ( Video dev : list ) {
			if ( dev.getModel() == Model.QXL || dev.getVgaMem() > 0 ) {
				// See https://lists.gnu.org/archive/html/qemu-devel/2012-06/msg01898.html
				if ( dev.getVgaMem() < MIN_VGA_MEM ) {
					dev.setVgaMem( Util.roundToNearestPowerOf2( MIN_VGA_MEM ) );
				}
				// * 4 is recommended on newer linux (KMS) according to https://www.ovirt.org/develop/internal/video-ram.html
				if ( dev.getRam() < dev.getVgaMem() * 4 ) {
					dev.setRam( Util.roundToNearestPowerOf2( dev.getVgaMem() * 4 ) );
				}
				// Windows can't really make good use of it. 2025-03-17, re-check every now and then...
				String os = config.getLibOsInfoOsId();
				if ( os != null && os.contains( "microsoft.com/" ) ) {
					dev.setVRam( 8192 );
				} else {
					dev.setVRam( Util.roundToNearestPowerOf2( dev.getVgaMem() * 2 ) );
				}
			}
		}

		// Adjust number of QXL devices if requested
		if ( displayCount > 0 ) {
			// Count existing QXL devices
			int qxlCount = 0;
			for ( Video dev : list ) {
				if ( dev.getModel() == Model.QXL ) {
					qxlCount++;
				}
			}

			// Build PCI slot usage map for collision-free assignment
			int[] inUse = DomainUtils.buildPciSlotUsageMap( config );

			// Add more QXL devices if needed
			int deviceCounter = 2; // Start from 2 to avoid lookup=0 which conflicts with "free slot" marker
			while ( qxlCount < displayCount ) {
				Video qxl = config.addVideoDevice();
				qxl.setModel( Model.QXL );
				// Set minimum VGA memory for proper resolution support
				qxl.setVgaMem( Util.roundToNearestPowerOf2( MIN_VGA_MEM ) );
				qxl.setRam( Util.roundToNearestPowerOf2( MIN_VGA_MEM * 4 ) );
				qxl.setVRam( Util.roundToNearestPowerOf2( MIN_VGA_MEM * 2 ) );

				// Assign a collision-free PCI slot using unique dummy addresses
				// Each device gets a unique device number to ensure different lookup values
				HostdevPciDeviceAddress dummyAddr = new HostdevPciDeviceAddress( 0, deviceCounter++, 0 );
				int slot = DomainUtils.findFreePciDeviceSlot( inUse, dummyAddr );
				qxl.setPciTarget( new HostdevPciDeviceAddress( 0, slot, 0 ) );

				qxlCount++;
			}

			// Remove excess QXL devices if we have too many
			// Remove from the end to avoid index shifting issues
			while ( qxlCount > displayCount ) {
				for ( int i = list.size() - 1; i >= 0; i-- ) {
					Video dev = list.get( i );
					if ( dev.getModel() == Model.QXL ) {
						dev.remove();
						qxlCount--;
						break;
					}
				}
			}
		}
	}

	@Override
	public void transform( Domain config, CommandLineArgs args ) throws TransformationException
	{
		// validate configuration and input arguments
		this.validateInputs( config, args );

		// Remove all existing graphics devices
		for ( final GraphicsVnc graphicsVncDevice : config.getGraphicVncDevices() ) {
			// remove VNC graphics device
			graphicsVncDevice.remove();
		}

		// Remove all existing SPICE devices
		boolean isOGL = false;
		for ( final GraphicsSpice graphicsSpiceDevice : config.getGraphicSpiceDevices() ) {
			// save state of configured OpenGL option
			isOGL = graphicsSpiceDevice.isOpenGlEnabled() || isOGL;
			// remove VNC graphics device
			graphicsSpiceDevice.remove();
		}

		// Adjust display device config
		this.adjustDisplayDevices( config, args.getDisplayCount() );
		
		// finally, add one SPICE graphics device with local Unix domain socket access
		this.addLocalSpiceGraphics( config, isOGL );
	}
}
