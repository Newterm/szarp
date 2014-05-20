#include "remotes_updater.h"

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/algorithm/string.hpp>

#include "cmd_list_remotes.h"
#include "cmd_update_remotes.h"
#include "cmd_connect_remote.h"

#include "global_service.h"

using boost::asio::ip::tcp;

namespace p = std::placeholders;

RemotesUpdater::RemotesUpdater(
		const std::string& name ,
		const std::string& address , unsigned port ,
		LocationsList& locs )
	: locs(locs) , name(name) , address(address) , port(port) ,
	  timeout(GlobalService::get_service())
{
}

RemotesUpdater::~RemotesUpdater()
{
}

void RemotesUpdater::connect()
{
	auto& service = GlobalService::get_service();

	tcp::resolver resolver(service);
	tcp::resolver::query query(address,boost::lexical_cast<std::string>(port));
	tcp::resolver::iterator endpoint = resolver.resolve(query);

	remote = std::make_shared<TcpClient>( service , endpoint );

	remote->on_connected ( std::bind(&RemotesUpdater::get_remotes,this) );
	remote->on_disconnect( std::bind(&RemotesUpdater::rm_remotes ,this) );

	protocol_mgr = std::make_shared<ProtocolLocation>( shared_from_this() , remote.get() );
}

void RemotesUpdater::get_remotes()
{
	send_cmd( new CmdListRemotesSnd( locs , name , address , port ) );
}

void RemotesUpdater::rm_remotes()
{
	auto tag = name + ":";
	for( auto itr=locs.begin() ; itr!=locs.end() ; )
		if( boost::starts_with( *itr , tag ) )
			itr = locs.remove_location( itr );
		else
			++itr;

	timeout.expires_from_now(boost::posix_time::seconds(5));
	timeout.async_wait( std::bind(&RemotesUpdater::reconnect,this,p::_1) );
}

void RemotesUpdater::reconnect( const boost::system::error_code& e )
{
	if( e ) return;

	connect();
}

Command* RemotesUpdater::cmd_from_tag( const std::string& tag )
{
	if( tag == "remote_added"   )
		return new CmdAddRemotesRcv   ( locs , name , address , port );
	if( tag == "remote_removed" )
		return new CmdRemoveRemotesRcv( locs , name );
	return NULL;
}

std::string RemotesUpdater::tag_from_cmd( const Command* cmd )
{
	if( typeid(CmdListRemotesSnd) == typeid(*cmd) )
		return "list_remotes";
	if( typeid(CmdConnectRemoteSnd) == typeid(*cmd) )
		return "connect";
	return "";
}

