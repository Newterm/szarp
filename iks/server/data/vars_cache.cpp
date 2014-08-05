#include "vars_cache.h"

Vars& VarsCache::get_szarp( const std::string& base )
{
	if( !szarp_vars.count(base) ) {
		szarp_vars[ base ].reset( new Vars() );
		szarp_vars[ base ]->from_szarp( base );
	}

	return *szarp_vars[base];
}

