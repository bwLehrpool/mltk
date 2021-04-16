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

public class TransformationSpecificQemuSerialDevices extends TransformationSpecific<Domain, CommandLineArgs, LibvirtHypervisorQemu>
{
	private static final String FILTER_NAME = "Serial devices";

	public TransformationSpecificQemuSerialDevices( LibvirtHypervisorQemu hypervisor )
	{
		super( TransformationSpecificQemuSerialDevices.FILTER_NAME, hypervisor );
	}

	private ArrayList<Serial> getSerialDevDevices( Domain config )
	{
		final ArrayList<Serial> devices = config.getSerialDevices();
		final Predicate<Serial> byDeviceTypeDev = device -> device.getType() == Type.DEV;

		return devices.stream().filter( byDeviceTypeDev ).collect( Collectors.toCollection( ArrayList::new ) );
	}

	private void filterSerialDevice( Domain config, String fileName, int index ) throws TransformationException
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
		this.filterSerialDevice( config, args.getVmDeviceSerial0(), 0 );

		// remove all additional serial devices
		final ArrayList<Serial> devices = this.getSerialDevDevices( config );
		for ( int i = 1; i < devices.size(); i++ ) {
			devices.get( i ).remove();
		}
	}
}
