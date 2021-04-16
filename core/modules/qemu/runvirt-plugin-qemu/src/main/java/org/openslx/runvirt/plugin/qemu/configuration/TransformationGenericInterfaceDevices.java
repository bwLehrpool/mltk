package org.openslx.runvirt.plugin.qemu.configuration;

import java.util.ArrayList;

import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.device.Interface;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.virtualization.configuration.VirtualizationConfigurationQemuUtils;
import org.openslx.virtualization.configuration.transformation.TransformationException;
import org.openslx.virtualization.configuration.transformation.TransformationGeneric;

public class TransformationGenericInterfaceDevices extends TransformationGeneric<Domain, CommandLineArgs>
{
	private static final String FILTER_NAME = "Network interface devices";

	public TransformationGenericInterfaceDevices()
	{
		super( TransformationGenericInterfaceDevices.FILTER_NAME );
	}

	private void filterInterfaceDevice( Domain config, String macAddress, int index ) throws TransformationException
	{
		final ArrayList<Interface> devices = config.getInterfaceDevices();
		final Interface device = VirtualizationConfigurationQemuUtils.getArrayIndex( devices, index );

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
	public void transform( Domain config, CommandLineArgs args ) throws TransformationException
	{
		this.filterInterfaceDevice( config, args.getVmMacAddress0(), 0 );

		// remove all additional disk storage devices
		final ArrayList<Interface> devices = config.getInterfaceDevices();
		for ( int i = 1; i < devices.size(); i++ ) {
			devices.get( i ).remove();
		}
	}
}
