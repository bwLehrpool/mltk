package org.openslx.runvirt.configuration;

public abstract class Filter<T, R> implements FilterFunction<T, R>
{
	private final String name;
	private boolean enabled;

	public Filter( String name )
	{
		this.name = name;
		this.setEnabled( true );
	}

	public String getName()
	{
		return this.name;
	}

	public boolean isEnabled()
	{
		return this.enabled;
	}

	public void setEnabled( boolean enabled )
	{
		this.enabled = enabled;
	}
}
