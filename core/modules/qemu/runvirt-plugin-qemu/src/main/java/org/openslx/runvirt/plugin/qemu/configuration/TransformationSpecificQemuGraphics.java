package org.openslx.runvirt.plugin.qemu.configuration;

import java.util.List;

import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.device.Graphics.ListenType;
import org.openslx.libvirt.domain.device.GraphicsSpice;
import org.openslx.libvirt.domain.device.GraphicsSpice.ImageCompression;
import org.openslx.libvirt.domain.device.GraphicsSpice.StreamingMode;
import org.openslx.libvirt.domain.device.GraphicsVnc;
import org.openslx.libvirt.domain.device.Video;
import org.openslx.libvirt.domain.device.Video.Model;
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

	private static final int MIN_VGA_MEM = 48 * 1024;
	private static final int MIN_RAM = 16 * 1024;

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

	private void adjustVideoMemory( Domain config )
	{
		// adjust vgamem, so higher resolutions work properly
		List<Video> list = config.getVideoDevices();
		for ( Video dev : list ) {
			if ( dev.getModel() == Model.QXL || dev.getVgaMem() > 0 ) {
				// See https://lists.gnu.org/archive/html/qemu-devel/2012-06/msg01898.html
				if ( dev.getVgaMem() < MIN_VGA_MEM ) {
					dev.setVgaMem( MIN_VGA_MEM );
				}
				if ( dev.getRam() < dev.getVgaMem() + MIN_RAM ) {
					dev.setRam( dev.getVgaMem() + MIN_RAM );
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

		// convert all SPICE graphics devices to local SPICE graphics devices
		boolean isOGL = false;
		for ( final GraphicsSpice graphicsSpiceDevice : config.getGraphicSpiceDevices() ) {
			// save state of configured OpenGL option
			isOGL = graphicsSpiceDevice.isOpenGlEnabled() || isOGL;
			// remove VNC graphics device
			graphicsSpiceDevice.remove();
		}
		// in case vmem is low, adjust to support large resolutions (4k+)
		this.adjustVideoMemory( config );
		
		// finally, add one SPICE graphics device with local Unix domain socket access
		this.addLocalSpiceGraphics( config, isOGL );
	}
}
