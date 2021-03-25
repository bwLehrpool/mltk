package org.openslx.runvirt.plugin.qemu.configuration;

import java.util.ArrayList;

import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.device.Parallel;
import org.openslx.libvirt.domain.device.Parallel.Type;
import org.openslx.runvirt.configuration.FilterException;
import org.openslx.runvirt.configuration.FilterGeneric;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.vm.QemuMetaDataUtils;

public class FilterGenericParallelDevices extends FilterGeneric<Domain, CommandLineArgs>
{
	private static final String FILTER_NAME = "Parallel devices";

	public FilterGenericParallelDevices()
	{
		super( FilterGenericParallelDevices.FILTER_NAME );
	}

	private void filterParallelDevice( Domain config, String fileName, int index ) throws FilterException
	{
		final ArrayList<Parallel> devices = config.getParallelDevices();
		final Parallel device = QemuMetaDataUtils.getArrayIndex( devices, index );

		if ( device == null ) {
			// check if device file name is specified
			if ( fileName != null ) {
				// parallel port device does not exist, so create new parallel port device
				final Parallel newDevice = config.addParallelDevice();
				newDevice.setType( Type.DEV );
				newDevice.setSource( fileName );
			}
		} else {
			if ( fileName == null ) {
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
	public void filter( Domain config, CommandLineArgs args ) throws FilterException
	{
		this.filterParallelDevice( config, args.getVmDeviceSerial0(), 0 );

		// remove all additional parallel devices
		final ArrayList<Parallel> devices = config.getParallelDevices();
		for ( int i = 1; i < devices.size(); i++ ) {
			devices.get( i ).remove();
		}
	}
}
