#include "welcome.h"

#include <typeinfo>

#include "cmd_hello.h"

#define MAP_CMD_TAG( _tag , cmd ) \
	if( _tag == tag ) return new cmd();

#define MAP_TAG_CMD( type , tag ) \
	if( typeid(type) == typeid(cmd) ) return tag;


WelcomeLoc::WelcomeLoc( Connection* con )
	: ProtocolLocation(con,this)
{
}

Command* WelcomeLoc::cmd_from_tag( const std::string& tag )
{
	MAP_CMD_TAG( "hello"          , CmdHelloRcv );
	return NULL;
}

std::string WelcomeLoc::tag_from_cmd( const Command* cmd )
{
	return "";
}

