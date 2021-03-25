package org.openslx.runvirt.plugin.qemu.configuration;

import java.util.ArrayList;

import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.device.Interface;
import org.openslx.runvirt.configuration.FilterException;
import org.openslx.runvirt.configuration.FilterGeneric;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.vm.QemuMetaDataUtils;

public class FilterGenericInterfaceDevices extends FilterGeneric<Domain, CommandLineArgs>
{
	private static final String FILTER_NAME = "Network interface devices";

	public FilterGenericInterfaceDevices()
	{
		super( FilterGenericInterfaceDevices.FILTER_NAME );
	}

	private void filterInterfaceDevice( Domain config, String macAddress, int index ) throws FilterException
	{
		final ArrayList<Interface> devices = config.getInterfaceDevices();
		final Interface device = QemuMetaDataUtils.getArrayIndex( devices, index );

		if ( device != null ) {
			if ( macAddress == null ) {
				// remove network interface device if MAC address is not set
				device.remove();
			} else {
				// set MAC address of network interface device if network interface device is available
				device.setMacAddress( macAddress );
			}
		}
	}

	@Override
	public void filter( Domain config, CommandLineArgs args ) throws FilterException
	{
		this.filterInterfaceDevice( config, args.getVmMacAddress0(), 0 );

		// remove all additional disk storage devices
		final ArrayList<Interface> devices = config.getInterfaceDevices();
		for ( int i = 1; i < devices.size(); i++ ) {
			devices.get( i ).remove();
		}
	}
}
