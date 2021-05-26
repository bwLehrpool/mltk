package org.openslx.runvirt.plugin.qemu.configuration;

import java.util.ArrayList;

import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.device.Parallel;
import org.openslx.libvirt.domain.device.Parallel.Type;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.virtualization.configuration.VirtualizationConfigurationQemuUtils;
import org.openslx.virtualization.configuration.transformation.TransformationException;
import org.openslx.virtualization.configuration.transformation.TransformationGeneric;

/**
 * Generic parallel device transformation for Libvirt/QEMU virtualization configurations.
 * 
 * @author Manuel Bentele
 * @version 1.0
 */
public class TransformationGenericParallelDevices extends TransformationGeneric<Domain, CommandLineArgs>
{
	/**
	 * Name of the configuration transformation.
	 */
	private static final String NAME = "Parallel devices";

	/**
	 * Creates a new parallel device transformation for Libvirt/QEMU virtualization configurations.
	 */
	public TransformationGenericParallelDevices()
	{
		super( TransformationGenericParallelDevices.NAME );
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
	 * Transforms a parallel device in a virtualization configuration selected by its {@code index}.
	 * 
	 * @param config virtualization configuration for the transformation.
	 * @param fileName path to the parallel device file on the host system.
	 * @param index number of the parallel device in the virtualization configuration that is
	 *           selected.
	 * @throws TransformationException transformation has failed.
	 */
	private void transformParallelDevice( Domain config, String fileName, int index ) throws TransformationException
	{
		final ArrayList<Parallel> devices = config.getParallelDevices();
		final Parallel device = VirtualizationConfigurationQemuUtils.getArrayIndex( devices, index );

		if ( device == null ) {
			// check if device file name is specified
			if ( fileName != null ) {
				// parallel port device does not exist, so create new parallel port device
				final Parallel newDevice = config.addParallelDevice();
				newDevice.setType( Type.DEV );
				newDevice.setSource( fileName );
			}
		} else {
			if ( fileName == null || fileName.isEmpty() ) {
				// remove device since device file is not specified
				device.remove();
			} else {
				// change type and source of existing parallel port device
				device.setType( Type.DEV );
				device.setSource( fileName );
			}
		}
	}

	@Override
	public void transform( Domain config, CommandLineArgs args ) throws TransformationException
	{
		// validate configuration and input arguments
		this.validateInputs( config, args );

		// alter parallel device
		this.transformParallelDevice( config, args.getVmDeviceParallel0(), 0 );

		// remove all additional parallel devices
		final ArrayList<Parallel> devices = config.getParallelDevices();
		for ( int i = 1; i < devices.size(); i++ ) {
			devices.get( i ).remove();
		}
	}
}
