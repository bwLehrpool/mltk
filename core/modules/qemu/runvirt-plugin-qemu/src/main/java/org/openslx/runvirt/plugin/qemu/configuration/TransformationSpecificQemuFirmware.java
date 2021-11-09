package org.openslx.runvirt.plugin.qemu.configuration;

import java.io.File;
import java.io.FileFilter;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.Reader;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.function.Predicate;

import org.openslx.libvirt.domain.Domain;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.runvirt.plugin.qemu.virtualization.LibvirtHypervisorQemu;
import org.openslx.util.LevenshteinDistance;
import org.openslx.virtualization.configuration.transformation.TransformationException;
import org.openslx.virtualization.configuration.transformation.TransformationSpecific;

import com.google.gson.Gson;
import com.google.gson.JsonIOException;
import com.google.gson.JsonSyntaxException;
import com.google.gson.annotations.SerializedName;

class QemuFirmware
{
	@SerializedName( "description" )
	private String description;
	@SerializedName( "interface-types" )
	private ArrayList<String> interfaceTypes;
	@SerializedName( "mapping" )
	private QemuFirmwareMapping mapping;
	@SerializedName( "targets" )
	private ArrayList<QemuFirmwareTarget> targets;
	@SerializedName( "features" )
	private ArrayList<String> features;
	@SerializedName( "tags" )
	private ArrayList<String> tags;

	public String getDescription()
	{
		return description;
	}

	public ArrayList<String> getInterfaceTypes()
	{
		return interfaceTypes;
	}

	public QemuFirmwareMapping getMapping()
	{
		return mapping;
	}

	public ArrayList<QemuFirmwareTarget> getTargets()
	{
		return targets;
	}

	public ArrayList<String> getFeatures()
	{
		return features;
	}

	public ArrayList<String> getTags()
	{
		return tags;
	}
}

class QemuFirmwareMapping
{
	@SerializedName( "device" )
	private String device;
	@SerializedName( "executable" )
	private QemuFirmwareMappingExecutable executable;
	@SerializedName( "nvram-template" )
	private QemuFirmwareMappingNvramTemplate nvramTemplate;

	public String getDevice()
	{
		return device;
	}

	public QemuFirmwareMappingExecutable getExecutable()
	{
		return executable;
	}

	public QemuFirmwareMappingNvramTemplate getNvramTemplate()
	{
		return nvramTemplate;
	}
}

class QemuFirmwareMappingExecutable
{
	@SerializedName( "filename" )
	private String fileName;
	@SerializedName( "format" )
	private String format;

	public String getFileName()
	{
		return fileName;
	}

	public String getFormat()
	{
		return format;
	}
}

class QemuFirmwareMappingNvramTemplate
{
	@SerializedName( "filename" )
	private String fileName;
	@SerializedName( "format" )
	private String format;

	public String getFileName()
	{
		return fileName;
	}

	public String getFormat()
	{
		return format;
	}
}

class QemuFirmwareTarget
{
	@SerializedName( "architecture" )
	private String architecture;
	@SerializedName( "machines" )
	private ArrayList<String> machines;

	public String getArchitecture()
	{
		return architecture;
	}

	public ArrayList<String> getMachines()
	{
		return machines;
	}
}

/**
 * Specific firmware transformation for Libvirt/QEMU virtualization configurations.
 * 
 * @author Manuel Bentele
 * @version 1.0
 */
public class TransformationSpecificQemuFirmware
		extends TransformationSpecific<Domain, CommandLineArgs, LibvirtHypervisorQemu>
{
	/**
	 * Name of the configuration transformation.
	 */
	private static final String NAME = "QEMU Firmware [Seabios, UEFI (OVMF)]";

	/**
	 * Creates a new firmware transformation for Libvirt/QEMU virtualization configurations.
	 * 
	 * @param hypervisor Libvirt/QEMU hypervisor.
	 */
	public TransformationSpecificQemuFirmware( LibvirtHypervisorQemu virtualizer )
	{
		super( TransformationSpecificQemuFirmware.NAME, virtualizer );
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
		} else if ( args.getFirmware() == null || args.getFirmware().isEmpty() ) {
			throw new TransformationException( "Path to QEMU firmware specifications directory is not specified!" );
		}
	}

	private String lookupTargetOsLoader( String firmwareSpecDirectory, String sourceOsLoader, String sourceOsArch,
			String sourceOsMachine )
			throws TransformationException
	{
		String lookupOsLoader = null;

		// parse and check firmware specification directory
		final File fwSpecDirectory = new File( firmwareSpecDirectory );
		if ( !fwSpecDirectory.exists() || !fwSpecDirectory.isDirectory() ) {
			throw new TransformationException( "Path to QEMU firmware specifications directory is invalid!" );
		}

		// get all firmware specification files
		final FileFilter fwSpecFilesFilter = file -> !file.isDirectory() && file.getName().endsWith( ".json" );
		final File[] fwSpecFiles = fwSpecDirectory.listFiles( fwSpecFilesFilter );

		// get paths to firmware files from firmware specification files
		if ( fwSpecFiles != null ) {
			final ArrayList<QemuFirmware> uefiFirmwares = new ArrayList<QemuFirmware>();
			final Gson gson = new Gson();
			for ( final File fwSpecFile : fwSpecFiles ) {
				QemuFirmware firmware = null;

				try {
					final Reader jsonContent = new FileReader( fwSpecFile );
					firmware = gson.fromJson( jsonContent, QemuFirmware.class );
				} catch ( FileNotFoundException | JsonSyntaxException | JsonIOException e ) {
					throw new TransformationException( e.getLocalizedMessage() );
				}

				final Predicate<String> byInterfaceType = s -> s.toLowerCase().equals( "uefi" );
				if ( firmware.getInterfaceTypes().stream().filter( byInterfaceType ).findAny().isPresent() ) {
					// found valid UEFI firmware
					// check if architecture and machine type of the VM is supported by the firmware
					final Predicate<QemuFirmwareTarget> byArchitecture = t -> sourceOsArch.equals( t.getArchitecture() );
					final Predicate<String> byMachineType = s -> sourceOsMachine.startsWith( s.replace( "*", "" ) );
					final Predicate<QemuFirmwareTarget> byMachines = t -> t.getMachines().stream().filter( byMachineType )
							.findAny().isPresent();

					if ( firmware.getTargets().stream().filter( byArchitecture ).filter( byMachines ).findAny()
							.isPresent() ) {
						// found UEFI firmware supporting suitable architecture and machine type from VM
						uefiFirmwares.add( firmware );
					}
				}
			}

			if ( uefiFirmwares.isEmpty() ) {
				throw new TransformationException( "There aren't any suitable UEFI firmwares locally available!" );
			} else {
				final LevenshteinDistance distance = new LevenshteinDistance( 1, 1, 1 );
				int minFileNameDistance = Integer.MAX_VALUE;
				Path suitablestUefiFirmwarePath = null;

				for ( final QemuFirmware uefiFirmware : uefiFirmwares ) {
					final Path uefiFirmwarePath = Paths.get( uefiFirmware.getMapping().getExecutable().getFileName() );
					final Path sourceOsLoaderPath = Paths.get( sourceOsLoader );
					final String uefiFirmwareFileName = uefiFirmwarePath.getFileName().toString().toLowerCase();
					final String sourceOsLoaderFileName = sourceOsLoaderPath.getFileName().toString().toLowerCase();

					final int fileNameDistance = distance.calculateDistance( uefiFirmwareFileName, sourceOsLoaderFileName );
					if ( fileNameDistance < minFileNameDistance ) {
						minFileNameDistance = fileNameDistance;
						suitablestUefiFirmwarePath = uefiFirmwarePath;
					}
				}

				lookupOsLoader = suitablestUefiFirmwarePath.toString();
			}
		}

		return lookupOsLoader;
	}

	@Override
	public void transform( Domain config, CommandLineArgs args ) throws TransformationException
	{
		// validate configuration and input arguments
		this.validateInputs( config, args );

		// get OS loader from VM
		final String sourceOsLoader = config.getOsLoader();

		// get OS architecture from VM
		final String sourceOsArch = config.getOsArch();

		// get OS machine type from VM
		final String sourceOsMachine = config.getOsMachine();

		// check if OS loader is specified
		if ( sourceOsLoader != null && !sourceOsLoader.isEmpty() ) {
			// OS loader is specified so transform path to specified firmware path
			final String targetOsLoader = this.lookupTargetOsLoader( args.getFirmware(), sourceOsLoader, sourceOsArch,
					sourceOsMachine );
			if ( targetOsLoader != null && !targetOsLoader.isEmpty() ) {
				config.setOsLoader( targetOsLoader );
			}
		}
	}
}
