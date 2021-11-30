package org.openslx.runvirt.plugin.qemu.configuration;

import org.openslx.firmware.FirmwareException;
import org.openslx.firmware.QemuFirmwareUtil;
import org.openslx.libvirt.domain.Domain;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.runvirt.plugin.qemu.virtualization.LibvirtHypervisorQemu;
import org.openslx.virtualization.configuration.transformation.TransformationException;
import org.openslx.virtualization.configuration.transformation.TransformationSpecific;

/**
 * Specific firmware transformation for Libvirt/QEMU virtualization configurations.
 * 
 * @author Manuel Bentele
 * @version 1.0
 */
public class TransformationSpecificQemuFirmware
		extends TransformationSpecific<Domain, CommandLineArgs, LibvirtHypervisorQemu>
{
	/**
	 * Name of the configuration transformation.
	 */
	private static final String NAME = "QEMU Firmware [Seabios, UEFI (OVMF)]";

	/**
	 * Creates a new firmware transformation for Libvirt/QEMU virtualization configurations.
	 * 
	 * @param hypervisor Libvirt/QEMU hypervisor.
	 */
	public TransformationSpecificQemuFirmware( LibvirtHypervisorQemu virtualizer )
	{
		super( TransformationSpecificQemuFirmware.NAME, virtualizer );
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
		} else if ( args.getFirmware() == null || args.getFirmware().isEmpty() ) {
			throw new TransformationException( "Path to QEMU firmware specifications directory is not specified!" );
		}
	}

	@Override
	public void transform( Domain config, CommandLineArgs args ) throws TransformationException
	{
		// validate configuration and input arguments
		this.validateInputs( config, args );

		// get OS loader from VM
		final String sourceOsLoader = config.getOsLoader();

		// get OS architecture from VM
		final String sourceOsArch = config.getOsArch();

		// get OS machine type from VM
		final String sourceOsMachine = config.getOsMachine();

		// check if OS loader is specified
		if ( sourceOsLoader != null && !sourceOsLoader.isEmpty() ) {
			// OS loader is specified so transform path to specified firmware path
			// First, lookup QEMU firmware loader for target
			String targetOsLoader = null;
			try {
				targetOsLoader = QemuFirmwareUtil.lookupTargetOsLoader( args.getFirmware(), sourceOsLoader, sourceOsArch,
						sourceOsMachine );
			} catch ( FirmwareException e ) {
				throw new TransformationException( e.getLocalizedMessage() );
			}

			// Second, set target QEMU firmware loader if specified
			if ( targetOsLoader != null && !targetOsLoader.isEmpty() ) {
				config.setOsLoader( targetOsLoader );
			}
		}
	}
}
