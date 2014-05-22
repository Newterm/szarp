#include "szbase.h"

#include <typeinfo>

#include "cmd_set.h"
#include "cmd_value.h"
#include "cmd_list_sets.h"
#include "cmd_list_params.h"
#include "cmd_get_params.h"
#include "cmd_set_update.h"
#include "cmd_set_subscribe.h"
#include "cmd_get_config.h"
#include "cmd_get_history.h"

#define MAP_CMD_TAG( _tag , cmd ) \
	if( _tag == tag ) return new cmd(vars);

#define MAP_TAG_CMD( type , tag ) \
	if( typeid(type) == typeid(cmd) ) return tag;

SzbaseProt::SzbaseProt(
		const std::string& szarp_base ,
		const std::string& prober_address ,
		unsigned prober_port )
{
	vars.from_szarp( szarp_base );
	vars.set_szarp_prober_server( prober_address , prober_port );
}

Command* SzbaseProt::cmd_from_tag( const std::string& tag )
{
	MAP_CMD_TAG( "s"             , SetRcv          );
	MAP_CMD_TAG( "set"           , SetRcv          );
	MAP_CMD_TAG( "list_sets"     , ListSetsRcv     );
	MAP_CMD_TAG( "list_params"   , ListParamsRcv   );
	MAP_CMD_TAG( "get_params"    , GetParamsRcv    );
	MAP_CMD_TAG( "get_set"       , GetSetRcv       );
	MAP_CMD_TAG( "set_update"    , SetUpdateRcv    );
	MAP_CMD_TAG( "set_subscribe" , SetSubscribeRcv );
	MAP_CMD_TAG( "get_options"   , GetConfigRcv    );
	MAP_CMD_TAG( "get_history"   , GetHistoryRcv   );
	return NULL;
}

std::string SzbaseProt::tag_from_cmd( const Command* cmd )
{
	MAP_TAG_CMD( ValueSnd        , "v"             );
	MAP_TAG_CMD( SetUpdateSnd    , "set_update"    );
	MAP_TAG_CMD( ConfigUpdateSnd , "new_options"   );
	return "";
}

