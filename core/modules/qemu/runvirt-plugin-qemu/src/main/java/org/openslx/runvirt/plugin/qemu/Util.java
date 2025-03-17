package org.openslx.runvirt.plugin.qemu;

public class Util
{

	/**
	 * Round up to nearest power of two
	 */
	public static long roundToNearestPowerOf2( long value )
	{
		long k = 1;
	
		while ( k < value ) {
			k *= 2;
		}
	
		return k;
	}

}
