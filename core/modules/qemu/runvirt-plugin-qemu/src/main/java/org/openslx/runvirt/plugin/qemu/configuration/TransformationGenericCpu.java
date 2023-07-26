package org.openslx.runvirt.plugin.qemu.configuration;

import java.util.List;

import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.Domain.CpuCheck;
import org.openslx.libvirt.domain.Domain.CpuMode;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.virtualization.configuration.transformation.TransformationException;
import org.openslx.virtualization.configuration.transformation.TransformationGeneric;

/**
 * Generic CPU transformation for Libvirt/QEMU virtualization configurations.
 * 
 * @author Manuel Bentele
 * @version 1.0
 */
public class TransformationGenericCpu extends TransformationGeneric<Domain, CommandLineArgs>
{
	/**
	 * Name of the configuration transformation.
	 */
	private static final String NAME = "CPU [number of cores, mode, ...]";

	/**
	 * Number of CPU dies per virtual machine.
	 */
	public static final int CPU_NUM_DIES = 1;

	/**
	 * Number of CPU sockets per virtual machine.
	 */
	public static final int CPU_NUM_SOCKETS = 1;

	/**
	 * Creates a new generic CPU transformation for Libvirt/QEMU virtualization configurations.
	 */
	public TransformationGenericCpu()
	{
		super( TransformationGenericCpu.NAME );
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
		} else if ( args.getVmNumCpus() < 1 && args.getCpuTopology() == null ) {
			throw new TransformationException( "Invalid number of virtual CPUs or CPU topology specified! Expected a number n > 0!" );
		}
	}

	@Override
	public void transform( Domain config, CommandLineArgs args ) throws TransformationException
	{
		// validate configuration and input arguments
		this.validateInputs( config, args );

		// set general CPU modes
		config.setCpuMode( CpuMode.HOST_PASSTHROUGH );
		config.setCpuCheck( CpuCheck.NONE );

		List<List<Integer>> topo = args.getCpuTopology();
		boolean onlyOneThread = false;
		int numCores;

		if ( topo == null ) {
			numCores = args.getVmNumCpus();
		} else {
			int last = -1;
			for ( List<Integer> group : topo ) {
				if ( last != -1 && last != group.size() ) {
					onlyOneThread = true;
					break;
				}
				last = group.size();
			}
			numCores = topo.size();
		}

		// set detailed CPU topology
		config.setCpuDies( TransformationGenericCpu.CPU_NUM_DIES );
		config.setCpuSockets( TransformationGenericCpu.CPU_NUM_SOCKETS );
		config.setCpuCores( numCores );
		config.setCpuThreads( ( topo == null || onlyOneThread ) ? 1 : topo.get( 0 ).size() );

		// set maximum allocated CPUs for the VM
		final int maxCpus = TransformationGenericCpu.CPU_NUM_DIES * TransformationGenericCpu.CPU_NUM_SOCKETS
				* numCores * config.getCpuThreads();
		config.setVCpu( maxCpus );
		config.setCpuMigratable( false );
		
		// Set CPU pinning if known
		config.resetCpuPin();
		if ( topo != null ) {
			int guestCore = 0;
			for ( List<Integer> group : topo ) {
				for ( int hostCore : group ) {
					config.addCpuPin( guestCore++, hostCore );
					if ( onlyOneThread )
						break;
				}
			}
		}
	}
}
