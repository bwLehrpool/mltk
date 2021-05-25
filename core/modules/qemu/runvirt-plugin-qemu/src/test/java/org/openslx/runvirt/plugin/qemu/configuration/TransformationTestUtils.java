package org.openslx.runvirt.plugin.qemu.configuration;

import static org.junit.jupiter.api.Assertions.fail;

import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.xml.LibvirtXmlDocumentException;
import org.openslx.libvirt.xml.LibvirtXmlSerializationException;
import org.openslx.libvirt.xml.LibvirtXmlTestResources;
import org.openslx.libvirt.xml.LibvirtXmlValidationException;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs.CmdLnOption;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgsException;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgsTest;

public class TransformationTestUtils
{
	// @formatter:off
	public static final String DEFAULT_VM_NAME      = "archlinux";
	public static final String DEFAULT_VM_UUID      = "4ec504d5-5eac-482f-a344-dbf1dd4956c8";
	public static final String DEFAULT_VM_DSPLNAME  = "Archlinux";
	public static final String DEFAULT_VM_OS        = "Windows 10 (x64)";
	public static final String DEFAULT_VM_NCPUS     = "16";
	public static final String DEFAULT_VM_MEM       = "1024";
	public static final String DEFAULT_VM_HDD0      = "/mnt/vm/windows.qcow2";
	public static final String DEFAULT_VM_FLOPPY0   = "/mnt/vm/floppy0.qcow2";
	public static final String DEFAULT_VM_FLOPPY1   = "/mnt/vm/floppy1.qcow2";
	public static final String DEFAULT_VM_CDROM0    = "/dev/sr0";
	public static final String DEFAULT_VM_CDROM1    = "/mnt/vm/cdrom1.qcow2";
	public static final String DEFAULT_VM_PARALLEL0 = "/dev/parport0";
	public static final String DEFAULT_VM_SERIAL0   = "/dev/ttyS0";
	public static final String DEFAULT_VM_MAC0      = "ca:06:29:84:f0:6d";
	public static final String DEFAULT_VM_FSSRC0    = "/mnt/shared/folder0";
	public static final String DEFAULT_VM_FSTGT0    = "folder0";
	public static final String DEFAULT_VM_FSSRC1    = "/mnt/shared/folder1";
	public static final String DEFAULT_VM_FSTGT1    = "folder1";
	// @formatter:on

	private static final String[] DEFAULT_CMDLN_ARGS = {
			CommandLineArgsTest.CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_NAME.getLongOption(),
			TransformationTestUtils.DEFAULT_VM_NAME,
			CommandLineArgsTest.CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_UUID.getLongOption(),
			TransformationTestUtils.DEFAULT_VM_UUID,
			CommandLineArgsTest.CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_DSPLNAME.getLongOption(),
			TransformationTestUtils.DEFAULT_VM_DSPLNAME,
			CommandLineArgsTest.CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_OS.getLongOption(),
			TransformationTestUtils.DEFAULT_VM_OS,
			CommandLineArgsTest.CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_NCPUS.getLongOption(),
			TransformationTestUtils.DEFAULT_VM_NCPUS,
			CommandLineArgsTest.CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_MEM.getLongOption(),
			TransformationTestUtils.DEFAULT_VM_MEM,
			CommandLineArgsTest.CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_HDD0.getLongOption(),
			TransformationTestUtils.DEFAULT_VM_HDD0,
			CommandLineArgsTest.CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_FLOPPY0.getLongOption(),
			TransformationTestUtils.DEFAULT_VM_FLOPPY0,
			CommandLineArgsTest.CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_FLOPPY1.getLongOption(),
			TransformationTestUtils.DEFAULT_VM_FLOPPY1,
			CommandLineArgsTest.CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_CDROM0.getLongOption(),
			TransformationTestUtils.DEFAULT_VM_CDROM0,
			CommandLineArgsTest.CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_CDROM1.getLongOption(),
			TransformationTestUtils.DEFAULT_VM_CDROM1,
			CommandLineArgsTest.CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_PARALLEL0.getLongOption(),
			TransformationTestUtils.DEFAULT_VM_PARALLEL0,
			CommandLineArgsTest.CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_SERIAL0.getLongOption(),
			TransformationTestUtils.DEFAULT_VM_SERIAL0,
			CommandLineArgsTest.CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_MAC0.getLongOption(),
			TransformationTestUtils.DEFAULT_VM_MAC0,
			CommandLineArgsTest.CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_FSSRC0.getLongOption(),
			TransformationTestUtils.DEFAULT_VM_FSSRC0,
			CommandLineArgsTest.CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_FSTGT0.getLongOption(),
			TransformationTestUtils.DEFAULT_VM_FSTGT0,
			CommandLineArgsTest.CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_FSSRC1.getLongOption(),
			TransformationTestUtils.DEFAULT_VM_FSSRC1,
			CommandLineArgsTest.CMDLN_PREFIX_OPTION_LONG + CmdLnOption.VM_FSTGT1.getLongOption(),
			TransformationTestUtils.DEFAULT_VM_FSTGT1
	};

	private static CommandLineArgs getCmdLnArgs( String[] args )
	{
		final CommandLineArgs cmdLnArgs = new CommandLineArgs();

		try {
			cmdLnArgs.parseCmdLnArgs( args );
		} catch ( CommandLineArgsException e ) {
			fail( e.getLocalizedMessage() );
		}

		return cmdLnArgs;
	}

	public static CommandLineArgs getDefaultCmdLnArgs()
	{
		return TransformationTestUtils.getCmdLnArgs( TransformationTestUtils.DEFAULT_CMDLN_ARGS );
	}

	public static CommandLineArgs getEmptyCmdLnArgs()
	{
		return TransformationTestUtils.getCmdLnArgs( new String[] {} );
	}

	public static Domain getDefaultDomain()
	{
		Domain domain = null;

		try {
			domain = new Domain( LibvirtXmlTestResources
					.getLibvirtXmlStream( "qemu-kvm_default-ubuntu-20-04-vm_transform-non-persistent.xml" ) );
		} catch ( LibvirtXmlDocumentException | LibvirtXmlSerializationException | LibvirtXmlValidationException e ) {
			fail( "Cannot prepare requested Libvirt domain XML file from the resources folder: "
					+ e.getLocalizedMessage() );
		}

		return domain;
	}
}
