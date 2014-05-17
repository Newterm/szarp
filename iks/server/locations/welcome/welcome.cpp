#include "welcome.h"

#include <typeinfo>

#include "cmd_hello.h"

#define MAP_CMD_TAG( _tag , cmd ) \
	if( _tag == tag ) return new cmd(*this,locs);

#define MAP_TAG_CMD( type , tag ) \
	if( typeid(type) == typeid(cmd) ) return tag;


Command* WelcomeProt::cmd_from_tag( const std::string& tag )
{
	MAP_CMD_TAG( "hello"          , CmdHelloRcv );
	return NULL;
}

std::string WelcomeProt::tag_from_cmd( const Command* cmd )
{
	return "";
}

