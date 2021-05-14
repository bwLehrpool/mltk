package org.openslx.runvirt.plugin.qemu.configuration;

import java.util.ArrayList;
import java.util.function.Predicate;
import java.util.stream.Collectors;

import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.device.Serial.Type;
import org.openslx.libvirt.domain.device.Serial;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.runvirt.plugin.qemu.virtualization.LibvirtHypervisorQemu;
import org.openslx.virtualization.configuration.VirtualizationConfigurationQemuUtils;
import org.openslx.virtualization.configuration.transformation.TransformationException;
import org.openslx.virtualization.configuration.transformation.TransformationSpecific;

/**
 * Specific serial device transformation for Libvirt/QEMU virtualization configurations.
 * 
 * @author Manuel Bentele
 * @version 1.0
 */
public class TransformationSpecificQemuSerialDevices
		extends TransformationSpecific<Domain, CommandLineArgs, LibvirtHypervisorQemu>
{
	/**
	 * Name of the configuration transformation.
	 */
	private static final String NAME = "Serial devices";

	/**
	 * Creates a new serial device transformation for Libvirt/QEMU virtualization configurations.
	 * 
	 * @param hypervisor Libvirt/QEMU hypervisor.
	 */
	public TransformationSpecificQemuSerialDevices( LibvirtHypervisorQemu hypervisor )
	{
		super( TransformationSpecificQemuSerialDevices.NAME, hypervisor );
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
	 * Returns all serial devices from a virtualization configuration that link to a host system's
	 * serial device.
	 * 
	 * @param config virtualization configuration.
	 * @return all serial devices that link to a host system's serial device.
	 */
	private ArrayList<Serial> getSerialDevDevices( Domain config )
	{
		final ArrayList<Serial> devices = config.getSerialDevices();
		final Predicate<Serial> byDeviceTypeDev = device -> device.getType() == Type.DEV;

		return devices.stream().filter( byDeviceTypeDev ).collect( Collectors.toCollection( ArrayList::new ) );
	}

	/**
	 * Transforms a serial device in a virtualization configuration selected by its {@code index}.
	 * 
	 * @param config virtualization configuration for the transformation.
	 * @param fileName path to the serial device file on the host system.
	 * @param index number of the serial device in the virtualization configuration that is selected.
	 * @throws TransformationException transformation has failed.
	 */
	private void transformSerialDevice( Domain config, String fileName, int index ) throws TransformationException
	{
		final ArrayList<Serial> devices = this.getSerialDevDevices( config );
		final Serial device = VirtualizationConfigurationQemuUtils.getArrayIndex( devices, index );

		if ( device == null ) {
			// check if device file name is specified
			if ( fileName != null ) {
				// serial port device is not available, so create new serial port device
				final Serial newDevice = config.addSerialDevice();
				newDevice.setType( Type.DEV );
				newDevice.setSource( fileName );
			}
		} else {
			if ( fileName == null ) {
				// remove serial port device if device file name is not set
				device.remove();
			} else {
				// set type and source of existing serial port device
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

		// alter serial device
		this.transformSerialDevice( config, args.getVmDeviceSerial0(), 0 );

		// remove all additional serial devices
		final ArrayList<Serial> devices = this.getSerialDevDevices( config );
		for ( int i = 1; i < devices.size(); i++ ) {
			devices.get( i ).remove();
		}
	}
}
