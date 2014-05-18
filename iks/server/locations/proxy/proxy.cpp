#include "proxy.h"

#include <functional>

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>

#include "global_service.h"
#include "net/tcp_client.h"

using boost::asio::ip::tcp;

namespace p = std::placeholders;

ProxyLoc::ProxyLoc( const std::string& address , unsigned port )
	: address(address) , port(port)
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

	remote->on_line_received( std::bind(&ProxyLoc::write_line,this,p::_1) );
}

void ProxyLoc::parse_line( const std::string& line )
{
	remote->write_line( line );
}

