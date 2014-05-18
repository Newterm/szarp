#include "manager.h"

#include <functional>

#include "locations/welcome/welcome.h"
#include "locations/protocol_location.h"

LocationsMgr::LocationsMgr()
{
}

LocationsMgr::~LocationsMgr()
{
}

void LocationsMgr::on_new_connection( Connection* con )
{
	new_location( std::make_shared<ProtocolLocation>( con , std::make_shared<WelcomeProt>() ) );
}

void LocationsMgr::on_disconnected( Connection* con )
{
	auto itr = locations.find( con );
	if( itr == locations.end() )
		return;

	locations.erase( itr );
}

void LocationsMgr::new_location( Location::ptr nloc , Location::ptr oloc )
{
	if( oloc )
		nloc->swap_connection( *oloc );

	locations[ nloc->connection ] = nloc;

	nloc->on_location_request(
		std::bind(
			&LocationsMgr::new_location, this,
			std::placeholders::_1 , nloc ) );
}

