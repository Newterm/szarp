#include "szbase.h"

#include <typeinfo>

#include <liblog.h>

#include "cmd_set.h"
#include "cmd_value.h"
#include "cmd_notify.h"
#include "cmd_list_sets.h"
#include "cmd_list_params.h"
#include "cmd_get_params.h"
#include "cmd_get_key_params.h"
#include "cmd_set_update.h"
#include "cmd_set_subscribe.h"
#include "cmd_get_config.h"
#include "cmd_get_history.h"
#include "cmd_get_latest.h"
#include "cmd_get_latest_set.h"
#include "cmd_get_summary.h"
#include "cmd_search_data.h"
#include "cmd_get_data.h"
#include "cmd_param_subscribe.h"
#include "cmd_param_unsubscribe.h"

namespace p = std::placeholders;

#define MAP_CMD_TAG( _tag , cmd ) \
	if( _tag == tag ) return new cmd(vars,*this);

#define MAP_TAG_CMD( type , tag ) \
	if( typeid(type) == typeid(*cmd) ) return tag;

SzbaseProt::SzbaseProt( Vars& vars )
	: vars(vars)
{
	conn_param_value = vars.get_params().on_param_value_changed(
		std::bind(
			&SzbaseProt::on_param_value_changed,
			this,p::_1,p::_2,p::_3) );

	conn_param = vars.get_params().on_param_changed(
		std::bind(
			&SzbaseProt::on_param_changed,
			this,p::_1) );
}

SzbaseProt::~SzbaseProt()
{
}

Command* SzbaseProt::cmd_from_tag( const std::string& tag )
{
	MAP_CMD_TAG( "s"                 , SetRcv              );
	MAP_CMD_TAG( "set"               , SetRcv              );
	MAP_CMD_TAG( "list_sets"         , ListSetsRcv         );
	MAP_CMD_TAG( "list_params"       , ListParamsRcv       );
	MAP_CMD_TAG( "get_params"        , GetParamsRcv        );
	MAP_CMD_TAG( "get_key_params"    , GetParamsForKeyRcv  );
	MAP_CMD_TAG( "get_set"           , GetSetRcv           );
	MAP_CMD_TAG( "set_update"        , SetUpdateRcv        );
	MAP_CMD_TAG( "set_subscribe"     , SetSubscribeRcv     );
	MAP_CMD_TAG( "get_options"       , GetConfigRcv        );
	MAP_CMD_TAG( "get_history"       , GetHistoryRcv       );
	MAP_CMD_TAG( "get_latest"        , GetLatestRcv        );
	MAP_CMD_TAG( "get_latest_set"    , GetLatestFromSetRcv );
	MAP_CMD_TAG( "get_summary"       , GetSummaryRcv       );
	MAP_CMD_TAG( "search_data"       , SearchDataRcv       );
	MAP_CMD_TAG( "get_data"          , GetDataRcv          );
	MAP_CMD_TAG( "param_subscribe"   , ParamSubscribeRcv   );
	MAP_CMD_TAG( "param_unsubscribe" , ParamUnsubscribeRcv );
	return NULL;
}

std::string SzbaseProt::tag_from_cmd( const Command* cmd )
{
	MAP_TAG_CMD( ValueSnd        , "v"             );
	MAP_TAG_CMD( NotifySnd       , "n"             );
	MAP_TAG_CMD( SetUpdateSnd    , "set_update"    );
	MAP_TAG_CMD( ConfigUpdateSnd , "new_options"   );
	return "";
}

void SzbaseProt::on_param_changed( Param::const_ptr p )
{
    if( sub_params.count( p->get_name() ) )
		send_cmd( new NotifySnd(p) );
}

void SzbaseProt::on_param_value_changed( Param::const_ptr p , double value , ProbeType pt )
{
	(void)value;

	if( current_set
	 && current_set->has_param( p->get_name() ) 
	 && current_pt == pt )
		send_cmd( new ValueSnd(p,pt) );
}

void SzbaseProt::set_current_set( Set::const_ptr s , ProbeType pt )
{
	current_set = s;
	current_pt = pt;

	if( !current_set ) {
		sub_set.cancel();
		return;
	}

	/** Prevent from sending values double if they values changed on subscribe */
	boost::signals2::shared_connection_block block(conn_param_value);

	sub_set = vars.get_updater().subscribe_params( *s , pt );

	for( auto itr=current_set->begin() ; itr!=current_set->end() ; ++itr )
	{
		auto p = vars.get_params().get_param( *itr );
		if( p )
			send_cmd( new ValueSnd(p,pt) );
		else
			sz_log(0, "Unknown param (%s) in set (%s)",
					itr->c_str() , s->get_name().c_str() );
	}
}

void SzbaseProt::subscribe_param( Param::const_ptr p )
{
	sub_params.insert(
		std::make_pair( p->get_name() ,
						vars.get_updater().subscribe_param( p->get_name() ,
															boost::optional<ProbeType>() ,
															false ) ) );
}

void SzbaseProt::unsubscribe_param( Param::const_ptr p )
{
	auto it = sub_params.find( p->get_name() );
	if( it != sub_params.end() )
		sub_params.erase( it );
}
