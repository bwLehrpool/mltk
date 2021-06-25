package org.openslx.runvirt.plugin.qemu.configuration;

import java.io.File;
import java.net.URL;

public class TransformationTestResources
{
	private static final String LIBVIRT_PREFIX_PATH = File.separator + "libvirt";
	private static final String LIBVIRT_PREFIX_PATH_XML = LIBVIRT_PREFIX_PATH + File.separator + "xml";

	public static File getLibvirtXmlFile( String libvirtXmlFileName )
	{
		String libvirtXmlPath = TransformationTestResources.LIBVIRT_PREFIX_PATH_XML + File.separator + libvirtXmlFileName;
		URL libvirtXml = TransformationTestResources.class.getResource( libvirtXmlPath );
		return new File( libvirtXml.getFile() );
	}
}
