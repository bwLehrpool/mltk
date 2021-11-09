package org.openslx.runvirt.plugin.qemu.configuration;

import java.io.File;
import java.net.URL;

public class TransformationTestResources
{
	private static final String QEMU_PREFIX_PATH = File.separator + "qemu";
	private static final String QEMU_PREFIX_PATH_FW = QEMU_PREFIX_PATH + File.separator + "firmware";

	public static String getQemuFirmwareSpecPath()
	{
		final URL qemuFwSpecPath = TransformationTestResources.class.getResource( QEMU_PREFIX_PATH_FW );
		return qemuFwSpecPath.getFile();
	}
}
