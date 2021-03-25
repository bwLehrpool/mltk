package org.openslx.runvirt.configuration;

public abstract class FilterSpecific<T, R, H> extends Filter<T, R>
{
	private final H hypervisor;

	public FilterSpecific( String name, H hypervisor )
	{
		super( name );

		this.hypervisor = hypervisor;
	}

	public H getHypervisor()
	{
		return this.hypervisor;
	}
}
