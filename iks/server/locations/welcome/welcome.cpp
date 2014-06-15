#include "welcome.h"

#include <typeinfo>

#include "cmd_list_remotes.h"
#include "cmd_update_remotes.h"
#include "cmd_connect_remote.h"

#define MAP_CMD_TAG( _tag , cmd ) \
	if( _tag == tag ) return new cmd(*this,locs);

#define MAP_TAG_CMD( type , tag ) \
	if( typeid(type) == typeid(*cmd) ) return tag;

namespace p = std::placeholders;

WelcomeProt::WelcomeProt( LocationsList& locs )
	: locs(locs)
{
	conn_add = locs.on_location_added(
		std::bind( &WelcomeProt::on_remote_added   , this , p::_1 ) );
	conn_rm  = locs.on_location_removed(
		std::bind( &WelcomeProt::on_remote_removed , this , p::_1 ) );
}

WelcomeProt::~WelcomeProt()
{
}

Command* WelcomeProt::cmd_from_tag( const std::string& tag )
{
	MAP_CMD_TAG( "connect"            , CmdConnectRemoteRcv );
	MAP_CMD_TAG( "list_remotes"       , CmdListRemotesRcv   );
	return NULL;
}

std::string WelcomeProt::tag_from_cmd( const Command* cmd )
{
	MAP_TAG_CMD( CmdAddRemoteSnd      , "remote_added"      );
	MAP_TAG_CMD( CmdRemoveRemoteSnd   , "remote_removed"    );
	return "";
}

void WelcomeProt::on_remote_added( const std::string& tag )
{
	send_cmd( new CmdAddRemoteSnd( tag ) );
}

void WelcomeProt::on_remote_removed( const std::string& tag )
{
	send_cmd( new CmdRemoveRemoteSnd( tag ) );
}

