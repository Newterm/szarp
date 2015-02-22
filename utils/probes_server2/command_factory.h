#ifndef __SERVER_COMMAND_FACTORY_H__
#define __SERVER_COMMAND_FACTORY_H__

/**
 * File with mapping tag names to CommandHandler classes
 */

#include "cmd_get.h"
#include "cmd_search.h"
#include "cmd_range.h"

#define MAP_CMD_TAG( _tag , cmd ) \
	if( _tag == tag ) return new cmd(ch);

class ClientHandler;

class CmdFactory {
public:
	static CommandHandler* cmd_from_tag( const std::string& tag , ClientHandler& ch )
	{
		MAP_CMD_TAG( "GET"    , GetRcv    );
		MAP_CMD_TAG( "SEARCH" , SearchRcv );
		MAP_CMD_TAG( "RANGE"  , RangeRcv  );
		return NULL;
	}

};

#undef MAP_CMD_TAG

#endif /* __SERVER_COMMAND_FACTORY_H__ */

