package org.openslx.runvirt.plugin.qemu.configuration;

import java.util.ArrayList;
import java.util.function.Predicate;
import java.util.stream.Collectors;

import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.device.Serial.Type;
import org.openslx.libvirt.domain.device.Serial;
import org.openslx.runvirt.configuration.FilterException;
import org.openslx.runvirt.configuration.FilterSpecific;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.runvirt.plugin.qemu.virtualization.LibvirtHypervisorQemu;
import org.openslx.vm.QemuMetaDataUtils;

public class FilterSpecificQemuSerialDevices extends FilterSpecific<Domain, CommandLineArgs, LibvirtHypervisorQemu>
{
	private static final String FILTER_NAME = "Serial devices";

	public FilterSpecificQemuSerialDevices( LibvirtHypervisorQemu hypervisor )
	{
		super( FilterSpecificQemuSerialDevices.FILTER_NAME, hypervisor );
	}

	private ArrayList<Serial> getSerialDevDevices( Domain config )
	{
		final ArrayList<Serial> devices = config.getSerialDevices();
		final Predicate<Serial> byDeviceTypeDev = device -> device.getType() == Type.DEV;

		return devices.stream().filter( byDeviceTypeDev ).collect( Collectors.toCollection( ArrayList::new ) );
	}

	private void filterSerialDevice( Domain config, String fileName, int index ) throws FilterException
	{
		final ArrayList<Serial> devices = this.getSerialDevDevices( config );
		final Serial device = QemuMetaDataUtils.getArrayIndex( devices, index );

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
	public void filter( Domain config, CommandLineArgs args ) throws FilterException
	{
		this.filterSerialDevice( config, args.getVmDeviceSerial0(), 0 );

		// remove all additional serial devices
		final ArrayList<Serial> devices = this.getSerialDevDevices( config );
		for ( int i = 1; i < devices.size(); i++ ) {
			devices.get( i ).remove();
		}
	}
}
