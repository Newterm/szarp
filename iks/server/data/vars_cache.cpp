#include "vars_cache.h"

VarsCache::VarsCache()
{
}

VarsCache::~VarsCache()
{
	for( auto itr=szarp_vars.begin() ; itr!=szarp_vars.end() ; ++itr )
		delete itr->second;
}

Vars& VarsCache::get_szarp( const std::string& base )
{
	if( szarp_vars.count(base) )
		return *szarp_vars[base];

	szarp_vars[ base ] = new Vars();
	szarp_vars[ base ]->from_szarp( base );

	return *szarp_vars[ base ];
}

