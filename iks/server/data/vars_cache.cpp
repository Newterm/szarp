#include "vars_cache.h"

VarsCache::VarsCache()
{
}

VarsCache::~VarsCache()
{
}

Vars& VarsCache::get_szarp( const std::string& base )
{
	if( szarp_vars.count(base) )
		return szarp_vars.find(base)->second;
	
	auto p = szarp_vars.emplace();
	p.first->second.from_szarp( base );

	return p.first->second;
}

