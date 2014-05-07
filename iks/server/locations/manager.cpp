#include "manager.h"

#include "welcome/welcome.h"
#include "szbase/szbase.h"

LocationsMgr::LocationsMgr( const std::string& base )
	: base(base)
{
}

LocationsMgr::~LocationsMgr()
{
	for( auto itr=locations.begin() ; itr!=locations.end() ; ++itr )
		delete itr->second;
}

void LocationsMgr::on_new_connection( Connection* con )
{
	locations[ con ] = new SzbaseLoc( "/opt/szarp/" + base , con );
}

void LocationsMgr::on_disconnected( Connection* con )
{
	auto itr = locations.find( con );
	if( itr == locations.end() )
		return;

	delete itr->second;
	locations.erase( itr );
}

