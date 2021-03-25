package org.openslx.runvirt.plugin.qemu.configuration;

import java.util.ArrayList;
import java.util.List;

import org.openslx.libvirt.capabilities.Capabilities;
import org.openslx.libvirt.capabilities.guest.Guest;
import org.openslx.libvirt.capabilities.guest.Machine;
import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.Domain.OsType;
import org.openslx.libvirt.domain.Domain.Type;
import org.openslx.runvirt.configuration.FilterException;
import org.openslx.runvirt.configuration.FilterSpecific;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.runvirt.plugin.qemu.virtualization.LibvirtHypervisorQemu;
import org.openslx.runvirt.virtualization.LibvirtHypervisorException;

public class FilterSpecificQemuArchitecture extends FilterSpecific<Domain, CommandLineArgs, LibvirtHypervisorQemu>
{
	private static final String FILTER_NAME = "QEMU Architecture [CPU architecture, machine type, ...]";

	// used as instance of an singelton, always use getCapabilities to retrieve caps instance
	private Capabilities capabilities = null;

	public FilterSpecificQemuArchitecture( LibvirtHypervisorQemu hypervisor )
	{
		super( FilterSpecificQemuArchitecture.FILTER_NAME, hypervisor );
	}

	private Capabilities getCapabilities() throws FilterException
	{
		// retrieve capabilities from QEMU hypervisor only once
		if ( this.capabilities == null ) {
			try {
				this.capabilities = this.getHypervisor().getCapabilites();
			} catch ( LibvirtHypervisorException e ) {
				final String errorMsg = new String(
						"Failed to get host capabilities from QEMU virtualizer: " + e.getLocalizedMessage() );
				throw new FilterException( errorMsg );
			}
		}

		return this.capabilities;
	}

	private Guest getTargetGuestFromArchName( String architectureName ) throws FilterException
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

	private Machine getTargetMachineFromGuest( Guest guest, String machineName ) throws FilterException
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

	private List<String> getCanonicalNamesFromTargetMachines( Guest guest ) throws FilterException
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
	public void filter( Domain config, CommandLineArgs args ) throws FilterException
	{
		// get source architecture, machine- and OS type
		final String sourceArchitectureName = config.getOsArch();
		final String sourceMachine = config.getOsMachine();
		final OsType sourceOsType = config.getOsType();
		final Type sourceDomainType = config.getType();

		// check if source architecture is supported by one of the hypervisor's guests
		Guest targetGuest = null;
		if ( sourceArchitectureName == null ) {
			final String errorMsg = new String( "Source architecture is not specified!" );
			throw new FilterException( errorMsg );
		} else {
			targetGuest = this.getTargetGuestFromArchName( sourceArchitectureName );
			if ( targetGuest == null ) {
				final String errorMsg = new String( "Source architecture is not supported by the virtualizer!" );
				throw new FilterException( errorMsg );
			}
		}

		// check if source machine is supported by the hypervisor
		Machine targetMachine = null;
		if ( sourceMachine == null ) {
			final String errorMsg = new String( "Source machine type is not specified!" );
			throw new FilterException( errorMsg );
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
					throw new FilterException( errorMsg );
				}
			}
		}

		// check if source OS type is supported by the hypervisor's architecture
		if ( sourceOsType == null ) {
			final String errorMsg = new String( "OS type is not specified!" );
			throw new FilterException( errorMsg );
		} else {
			if ( !sourceOsType.toString().equals( targetGuest.getOsType().toString() ) ) {
				final String errorMsg = new String( "OS type is not supported by the virtualizer!" );
				throw new FilterException( errorMsg );
			}
		}

		// check if source domain type is supported by the hypervisor's architecture
		Type targetDomainType = null;
		if ( sourceDomainType == null ) {
			final String errorMsg = new String( "Source domain type is not specified!" );
			throw new FilterException( errorMsg );
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
				throw new FilterException( errorMsg );
			}
		}

		// patch path of QEMU emulator binary
		final String archEmulator = targetGuest.getArchEmulator();
		if ( archEmulator == null ) {
			final String errorMsg = new String( "Emulation of source architecture is not supported by the virtualizer!" );
			throw new FilterException( errorMsg );
		} else {
			config.setDevicesEmulator( targetGuest.getArchEmulator() );
		}
	}
}
