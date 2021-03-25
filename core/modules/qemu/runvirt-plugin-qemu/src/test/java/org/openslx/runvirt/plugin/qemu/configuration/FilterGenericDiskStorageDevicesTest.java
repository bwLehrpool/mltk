package org.openslx.runvirt.plugin.qemu.configuration;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotEquals;

import java.util.ArrayList;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.openslx.libvirt.domain.Domain;
import org.openslx.libvirt.domain.device.Disk.StorageType;
import org.openslx.libvirt.domain.device.DiskStorage;
import org.openslx.runvirt.configuration.FilterException;
import org.openslx.runvirt.plugin.qemu.cmdln.CommandLineArgs;

public class FilterGenericDiskStorageDevicesTest
{
	@Test
	@DisplayName( "Test filtering of VM disk storage devices configuration with specified input data" )
	public void testFilterGenericDiskStorageDevices() throws FilterException
	{
		final FilterGenericDiskStorageDevices filter = new FilterGenericDiskStorageDevices();
		final Domain config = FilterTestUtils.getDefaultDomain();
		final CommandLineArgs args = FilterTestUtils.getDefaultCmdLnArgs();

		final ArrayList<DiskStorage> devicesBeforeFiltering = config.getDiskStorageDevices();
		assertEquals( 1, devicesBeforeFiltering.size() );
		assertNotEquals( StorageType.FILE, devicesBeforeFiltering.get( 0 ).getStorageType() );
		assertNotEquals( FilterTestUtils.DEFAULT_VM_HDD0, devicesBeforeFiltering.get( 0 ).getStorageSource() );

		filter.filter( config, args );

		final ArrayList<DiskStorage> devicesAfterFiltering = config.getDiskStorageDevices();
		assertEquals( 1, devicesAfterFiltering.size() );
		assertEquals( StorageType.FILE, devicesAfterFiltering.get( 0 ).getStorageType() );
		assertEquals( FilterTestUtils.DEFAULT_VM_HDD0, devicesAfterFiltering.get( 0 ).getStorageSource() );
	}

	@Test
	@DisplayName( "Test filtering of VM disk storage devices configuration with unspecified input data" )
	public void testFilterGenericDiskStorageDevicesNoData() throws FilterException
	{
		final FilterGenericDiskStorageDevices filter = new FilterGenericDiskStorageDevices();
		final Domain config = FilterTestUtils.getDefaultDomain();
		final CommandLineArgs args = FilterTestUtils.getEmptyCmdLnArgs();

		final ArrayList<DiskStorage> devicesBeforeFiltering = config.getDiskStorageDevices();
		assertEquals( 1, devicesBeforeFiltering.size() );

		filter.filter( config, args );

		final ArrayList<DiskStorage> devicesAfterFiltering = config.getDiskStorageDevices();
		assertEquals( 0, devicesAfterFiltering.size() );
	}

	public static void main( String[] args ) throws FilterException
	{
		FilterGenericDiskStorageDevicesTest test = new FilterGenericDiskStorageDevicesTest();
		test.testFilterGenericDiskStorageDevicesNoData();
	}
}
