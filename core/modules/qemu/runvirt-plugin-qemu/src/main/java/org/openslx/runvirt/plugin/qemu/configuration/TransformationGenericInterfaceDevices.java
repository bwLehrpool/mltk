package org.openslx.runvirt.plugin.qemu.configuration;

import java.util.ArrayList;

import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.device.Interface;
import org.openslx.libvirt.domain.device.InterfaceBridge;
import org.openslx.libvirt.domain.device.Interface.Model;
import org.openslx.libvirt.domain.device.Interface.Type;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.virtualization.configuration.VirtualizationConfigurationQemu;
import org.openslx.virtualization.configuration.VirtualizationConfigurationQemuUtils;
import org.openslx.virtualization.configuration.transformation.TransformationException;
import org.openslx.virtualization.configuration.transformation.TransformationGeneric;

/**
 * Generic network interface transformation for Libvirt/QEMU virtualization configurations.
 * 
 * @author Manuel Bentele
 * @version 1.0
 */
public class TransformationGenericInterfaceDevices extends TransformationGeneric<Domain, CommandLineArgs>
{
	/**
	 * Name of the configuration transformation.
	 */
	private static final String NAME = "Network interface devices";

	/**
	 * Creates a new network interface transformation for Libvirt/QEMU virtualization configurations.
	 */
	public TransformationGenericInterfaceDevices()
	{
		super( TransformationGenericInterfaceDevices.NAME );
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
	 * Transforms a network interface in a virtualization configuration selected by its
	 * {@code index}.
	 * 
	 * @param config virtualization configuration for the transformation.
	 * @param macAddress MAC address for the network interface.
	 * @param index number of the network interface in the virtualization configuration that is
	 *           selected.
	 * @throws TransformationException transformation has failed.
	 */
	private void transformInterfaceDevice( Domain config, String macAddress, int index ) throws TransformationException
	{
		final ArrayList<Interface> devices = config.getInterfaceDevices();
		final Interface device = VirtualizationConfigurationQemuUtils.getArrayIndex( devices, index );

		if ( device == null ) {
			if ( macAddress != null && !macAddress.isEmpty() ) {
				// create network interface if it does not exists
				final InterfaceBridge newDevice = config.addInterfaceBridgeDevice();
				newDevice.setType( Type.BRIDGE );
				newDevice.setModel( Model.VIRTIO );
				newDevice.setMacAddress( macAddress );
				newDevice.setSource( VirtualizationConfigurationQemu.NETWORK_BRIDGE_NAT_DEFAULT );
			}
		} else {
			if ( macAddress == null || macAddress.isEmpty() ) {
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
		// validate configuration and input arguments
		this.validateInputs( config, args );

		// alter network interface
		this.transformInterfaceDevice( config, args.getVmMacAddress0(), 0 );

		// remove all additional disk storage devices
		final ArrayList<Interface> devices = config.getInterfaceDevices();
		for ( int i = 1; i < devices.size(); i++ ) {
			devices.get( i ).remove();
		}
	}
}
