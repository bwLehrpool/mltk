package org.openslx.runvirt.plugin.qemu.configuration;

import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.device.Graphics.ListenType;
import org.openslx.libvirt.domain.device.GraphicsSpice;
import org.openslx.libvirt.domain.device.GraphicsSpice.ImageCompression;
import org.openslx.libvirt.domain.device.GraphicsSpice.StreamingMode;
import org.openslx.libvirt.domain.device.GraphicsVnc;
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
	public GraphicsSpice addLocalSpiceGraphics( Domain config, boolean openGlEnabled )
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

	@Override
	public void transform( Domain config, CommandLineArgs args ) throws TransformationException
	{
		// validate configuration and input arguments
		this.validateInputs( config, args );

		// convert all VNC graphics devices to local SPICE graphics devices
		for ( final GraphicsVnc graphicsVncDevice : config.getGraphicVncDevices() ) {

			// remove VNC graphics device
			graphicsVncDevice.remove();

			// add SPICE graphics device with local Unix domain socket access
			this.addLocalSpiceGraphics( config, false );
		}

		// convert all SPICE graphics devices to local SPICE graphics devices
		for ( final GraphicsSpice graphicsSpiceDevice : config.getGraphicSpiceDevices() ) {

			if ( graphicsSpiceDevice.getListenType() != ListenType.NONE ) {

				// save state of configured OpenGL option
				final boolean openGlEnabled = graphicsSpiceDevice.isOpenGlEnabled();

				// remove VNC graphics device
				graphicsSpiceDevice.remove();

				// add SPICE graphics device with local Unix domain socket access
				this.addLocalSpiceGraphics( config, openGlEnabled );
			}
		}
	}
}
