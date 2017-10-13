#include "proxy.h"

#include <iostream>
#include <functional>

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>

#include <liblog.h>

#include "global_service.h"

using boost::asio::ip::tcp;

namespace p = std::placeholders;

ProxyLoc::ProxyLoc( const std::string& name , const std::string& address , unsigned port )
	: Location(name) , address(address) , port(port)
{
	connect( address , port );
}

ProxyLoc::~ProxyLoc()
{
}

void ProxyLoc::connect( const std::string& address , unsigned port )
{
	auto& service = GlobalService::get_service();

	tcp::resolver resolver(service);
	tcp::resolver::query query(address,boost::lexical_cast<std::string>(port));
	tcp::resolver::iterator endpoint = resolver.resolve(query);

	remote = std::make_shared<TcpClient>( service , endpoint );

	std::string next = get_name();
	next.erase( next.begin() , std::next(next.begin(),next.find(':')+1) );
	
	if( !next.empty() )
		remote->write_line( "connect 0 " + next );

	remote->on_connected ( std::bind(&ProxyLoc::fwd_connect,this) );
	remote->on_disconnect( std::bind(&ProxyLoc::die        ,this) );
}

void ProxyLoc::fwd_connect()
{
	conn_line = remote->on_line_received( std::bind(&ProxyLoc::get_response,this,p::_1) );
}

void ProxyLoc::get_response( const std::string& line )
{
	if( line.empty() || !(line[0] == 'k') )
		sz_log(1, "Error in forwarding connection, invalid line: '%s'", line.c_str());

	conn_line = remote->on_line_received( std::bind(&ProxyLoc::write_line,this,p::_1) );
}

void ProxyLoc::parse_line( const std::string& line )
{
	remote->write_line( line );
}

