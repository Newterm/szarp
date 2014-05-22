#include "manager.h"

#include <functional>

#include "locations/welcome/welcome.h"
#include "locations/proxy/proxy.h"
#include "locations/szbase/szbase.h"
#include "locations/protocol_location.h"

LocationsMgr::LocationsMgr()
{
}

LocationsMgr::~LocationsMgr()
{
}

void LocationsMgr::add_locations( const CfgSections& cfg )
{
	for( auto itr=cfg.begin() ; itr!=cfg.end() ; ++itr )
		add_location( itr->first , itr->second );
}

void LocationsMgr::add_location( const std::string& name , const CfgPairs& cfg )
{
	try {
			 if( cfg.at("type") == "szbase" )
			add_szbase( name , cfg );
		else if( cfg.at("type") == "proxy" )
			add_proxy( name , cfg );
		else
			throw invalid_value("Invalid 'type' in section " + name );
	} catch( std::out_of_range& e ) {
		throw missing_option("Missing option in section " + name );
	}
}

void LocationsMgr::add_szbase( const std::string& name , const CfgPairs& cfg )
{
	loc_factory.register_location
		<SzbaseLocation>( name , cfg.at("base") );
}

void LocationsMgr::add_proxy( const std::string& name , const CfgPairs& cfg )
{
	unsigned port;
	try {
		port = boost::lexical_cast<unsigned>(cfg.at("port"));
	} catch( boost::bad_lexical_cast& e ) {
		throw invalid_value("Invalid port number in section " + name );
	} catch( std::out_of_range& e ) {
		port = 9002;
	}

	auto updater = std::make_shared
		<RemotesUpdater>( name , cfg.at("address") , port , loc_factory );

	updater->connect();

	updaters[ name ] = updater;
}

void LocationsMgr::on_new_connection( Connection* con )
{
	new_location( std::make_shared<ProtocolLocation>( "welcome" , std::make_shared<WelcomeProt>(loc_factory) , con ) );
}

void LocationsMgr::on_disconnected( Connection* con )
{
	auto itr = locations.find( con );
	if( itr != locations.end() )
		locations.erase( itr );
}

void LocationsMgr::new_location( Location::ptr nloc , std::weak_ptr<Location> oloc_w )
{
	auto oloc = oloc_w.lock();

	if( oloc )
		nloc->swap_connection( *oloc );

	locations[ nloc->connection ] = nloc;

	nloc->on_location_request(
		std::bind(
			&LocationsMgr::new_location, this,
			std::placeholders::_1 , std::weak_ptr<Location>(nloc) ) );
}

