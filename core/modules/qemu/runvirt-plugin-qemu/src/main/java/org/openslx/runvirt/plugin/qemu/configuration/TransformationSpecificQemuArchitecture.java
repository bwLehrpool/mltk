package org.openslx.runvirt.plugin.qemu.configuration;

import java.util.ArrayList;
import java.util.List;

import org.openslx.libvirt.capabilities.Capabilities;
import org.openslx.libvirt.capabilities.guest.Guest;
import org.openslx.libvirt.capabilities.guest.Machine;
import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.Domain.OsType;
import org.openslx.libvirt.domain.Domain.Type;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.runvirt.plugin.qemu.virtualization.LibvirtHypervisorQemu;
import org.openslx.runvirt.virtualization.LibvirtHypervisorException;
import org.openslx.virtualization.configuration.transformation.TransformationException;
import org.openslx.virtualization.configuration.transformation.TransformationSpecific;

/**
 * Specific architecture transformation for Libvirt/QEMU virtualization configurations.
 * 
 * @author Manuel Bentele
 * @version 1.0
 */
public class TransformationSpecificQemuArchitecture
		extends TransformationSpecific<Domain, CommandLineArgs, LibvirtHypervisorQemu>
{
	/**
	 * Name of the configuration transformation.
	 */
	private static final String NAME = "QEMU Architecture [CPU architecture, machine type, ...]";

	/**
	 * Creates a new architecture transformation for Libvirt/QEMU virtualization configurations.
	 * 
	 * @param hypervisor Libvirt/QEMU hypervisor.
	 */
	public TransformationSpecificQemuArchitecture( LibvirtHypervisorQemu virtualizer )
	{
		super( TransformationSpecificQemuArchitecture.NAME, virtualizer );
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
	 * Queries and returns the capabilities of the Libvirt/QEMU hypervisor.
	 * 
	 * @return capabilities of the Libvirt/QEMU hypervisor.
	 * @throws TransformationException failed to query and return the capabilities of the
	 *            Libvirt/QEMU hypervisor.
	 */
	protected Capabilities getCapabilities() throws TransformationException
	{
		final Capabilities capabilities;

		try {
			capabilities = this.getVirtualizer().getCapabilites();
		} catch ( LibvirtHypervisorException e ) {
			final String errorMsg = new String(
					"Failed to retrieve host capabilities from QEMU virtualizer: " + e.getLocalizedMessage() );
			throw new TransformationException( errorMsg );
		}

		return capabilities;
	}

	/**
	 * Returns a guest capability of the hypervisor's host system based on a given target
	 * architecture name.
	 * 
	 * @param architectureName target architecture of the guest that is returned
	 * @return guest capability of the hypervisor's host system with target architecture name.
	 * @throws TransformationException failed to return guest capability of the hypervisor's host.
	 */
	private Guest getTargetGuestFromArchName( String architectureName ) throws TransformationException
	{
		final List<Guest> guests = this.getCapabilities().getGuests();
		Guest targetGuest = null;

		if ( architectureName == null ) {
			return targetGuest;
		}

		for ( Guest guest : guests ) {
			final String guestArchitectureName = guest.getArchName();
			if ( architectureName.equals( guestArchitectureName ) ) {
				targetGuest = guest;
				break;
			}
		}

		return targetGuest;
	}

	/**
	 * Returns the target machine description of a host system's guest capability based on a given
	 * target machine name.
	 * 
	 * @param guest guest capability of a host system.
	 * @param machineName name of the machine description.
	 * @return target machine description of a host system's guest capability.
	 * @throws TransformationException failed to return the target machine description of a host
	 *            system's guest capabilities.
	 */
	private Machine getTargetMachineFromGuest( Guest guest, String machineName ) throws TransformationException
	{
		final List<Machine> machines = guest.getArchMachines();
		Machine targetMachine = null;

		if ( machineName == null ) {
			return targetMachine;
		}

		for ( Machine machine : machines ) {
			if ( machineName.equals( machine.getName() ) ) {
				targetMachine = machine;
				break;
			}
		}

		return targetMachine;
	}

	/**
	 * Returns the canonical names of a target machine description of a host system's guest
	 * capability.
	 * 
	 * @param guest guest capability of a host system.
	 * @return canonical names of a target machine description of a host system's guest capability.
	 * @throws TransformationException failed to return the canonical names of a target machine
	 *            description of a host system's guest capability
	 */
	private List<String> getCanonicalNamesFromTargetMachines( Guest guest ) throws TransformationException
	{
		final List<Machine> machines = guest.getArchMachines();
		final List<String> canonicalNames = new ArrayList<String>();

		for ( Machine machine : machines ) {
			final String canonicalName = machine.getCanonicalMachine();
			if ( canonicalName != null ) {
				canonicalNames.add( canonicalName );
			}
		}

		return canonicalNames;
	}

	@Override
	public void transform( Domain config, CommandLineArgs args ) throws TransformationException
	{
		// validate configuration and input arguments
		this.validateInputs( config, args );

		// get source architecture, machine- and OS type
		final String sourceArchitectureName = config.getOsArch();
		final String sourceMachine = config.getOsMachine();
		final OsType sourceOsType = config.getOsType();
		final Type sourceDomainType = config.getType();

		// check if source architecture is supported by one of the hypervisor's guests
		Guest targetGuest = null;
		if ( sourceArchitectureName == null ) {
			final String errorMsg = new String( "Source architecture is not specified!" );
			throw new TransformationException( errorMsg );
		} else {
			targetGuest = this.getTargetGuestFromArchName( sourceArchitectureName );
			if ( targetGuest == null ) {
				final String errorMsg = new String( "Source architecture is not supported by the virtualizer!" );
				throw new TransformationException( errorMsg );
			}
		}

		// check if source machine is supported by the hypervisor
		Machine targetMachine = null;
		if ( sourceMachine == null ) {
			final String errorMsg = new String( "Source machine type is not specified!" );
			throw new TransformationException( errorMsg );
		} else {
			// get all possible machine type for supported source architecture
			targetMachine = this.getTargetMachineFromGuest( targetGuest, sourceMachine );

			if ( targetMachine == null ) {
				// source machine is not directly supported by the hypervisor
				// check if up- or downgraded version of the chipset is supported by the hypervisor
				List<String> targetMachineCanonicalNames = this.getCanonicalNamesFromTargetMachines( targetGuest );

				// retrieve overwrite chipset name from canonical machine names
				String sourceMachineOverwrite = null;
				for ( String targetMachineCanonicalName : targetMachineCanonicalNames ) {
					if ( sourceMachine.contains( targetMachineCanonicalName ) ) {
						sourceMachineOverwrite = targetMachineCanonicalName;
						break;
					}
				}

				// if overwrite available, patch the machine type
				if ( sourceMachineOverwrite != null ) {
					config.setOsMachine( sourceMachineOverwrite );
				} else {
					final String errorMsg = new String( "Source machine type is not supported by the virtualizer!" );
					throw new TransformationException( errorMsg );
				}
			}
		}

		// check if source OS type is supported by the hypervisor's architecture
		if ( sourceOsType == null ) {
			final String errorMsg = new String( "OS type is not specified!" );
			throw new TransformationException( errorMsg );
		} else {
			if ( !sourceOsType.toString().equals( targetGuest.getOsType().toString() ) ) {
				final String errorMsg = new String( "OS type is not supported by the virtualizer!" );
				throw new TransformationException( errorMsg );
			}
		}

		// check if source domain type is supported by the hypervisor's architecture
		Type targetDomainType = null;
		if ( sourceDomainType == null ) {
			final String errorMsg = new String( "Source domain type is not specified!" );
			throw new TransformationException( errorMsg );
		} else {
			final List<org.openslx.libvirt.capabilities.guest.Domain> targetDomains = targetGuest.getArchDomains();

			// retrieve supported domain type
			for ( org.openslx.libvirt.capabilities.guest.Domain domain : targetDomains ) {
				final Type domainType = domain.getType();
				if ( domainType == sourceDomainType ) {
					targetDomainType = domainType;
					break;
				}
			}

			// check supported domain type
			if ( targetDomainType == null ) {
				final String errorMsg = new String( "Source domain type is not supported by the virtualizer!" );
				throw new TransformationException( errorMsg );
			}
		}

		// patch path of QEMU emulator binary
		final String archEmulator = targetGuest.getArchEmulator();
		if ( archEmulator == null ) {
			final String errorMsg = new String( "Emulation of source architecture is not supported by the virtualizer!" );
			throw new TransformationException( errorMsg );
		} else {
			config.setDevicesEmulator( targetGuest.getArchEmulator() );
		}
	}
}
