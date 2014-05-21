#include "config.h"

void CfgSections::from_flat( const CfgPairs& flat )
{
	for( auto itr=flat.begin() ; itr!=flat.end() ; ++itr )
	{
		auto idot = itr->first.find('.');
		if( idot == std::string::npos )
			continue;

		std::string tag (itr->first.begin(),std::next(itr->first.begin(),idot));
		std::string name(std::next(itr->first.begin(),idot+1),itr->first.end());

		(*this)[tag][name] = itr->second;
	}
}

