package org.openslx.runvirt.configuration;

@FunctionalInterface
public interface FilterFunction<T, R>
{
	public void filter( T config, R args ) throws FilterException;

	public default void apply( T config, R args ) throws FilterException
	{
		this.filter( config, args );
	}
}
