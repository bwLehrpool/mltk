package org.openslx.runvirt.configuration;

import java.util.ArrayList;

public final class FilterManager<T, R>
{
	private ArrayList<Filter<T, R>> filters;
	private T config;
	private R args;

	public FilterManager( T config, R args )
	{
		this.filters = new ArrayList<Filter<T, R>>();
		this.config = config;
		this.args = args;
	}

	public void register( Filter<T, R> filter )
	{
		this.register( filter, true );
	}

	public void register( Filter<T, R> filter, boolean enabled )
	{
		filter.setEnabled( enabled );
		this.filters.add( filter );
	}

	public void register( String name, FilterFunction<T, R> filterFunction )
	{
		this.register( name, filterFunction, true );
	}

	public void register( String name, FilterFunction<T, R> filterFunction, boolean enabled )
	{
		final Filter<T, R> filter = new Filter<T, R>( name ) {
			@Override
			public void filter( T document, R args ) throws FilterException
			{
				filterFunction.apply( document, args );
			}
		};

		filter.setEnabled( enabled );
		this.filters.add( filter );
	}

	public void filterAll() throws FilterException
	{
		for ( Filter<T, R> filter : this.filters ) {
			try {
				filter.apply( this.config, this.args );
			} catch ( FilterException e ) {
				final String errorMsg = new String(
						"Error in configuration filter '" + filter.getName() + "':" + e.getLocalizedMessage() );
				throw new FilterException( errorMsg );
			}
		}
	}

	private String showFilters()
	{
		String filterSummary = new String();
		final int maxFilterNumCharacters = ( this.filters.size() + 1 ) / 10;

		for ( int i = 0; i < this.filters.size(); i++ ) {
			final Filter<T, R> filter = this.filters.get( i );
			final String paddedNumber = String.format( "%-" + maxFilterNumCharacters + "s", i + 1 );
			filterSummary += paddedNumber + ": " + filter.getName() + System.lineSeparator();
		}

		return filterSummary;
	}

	@Override
	public String toString()
	{
		return this.showFilters();
	}
}
