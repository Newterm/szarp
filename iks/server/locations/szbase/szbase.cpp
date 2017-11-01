#include "szbase.h"

#include <typeinfo>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/random_generator.hpp>


#include <liblog.h>

#include "cmd_set.h"
#include "cmd_value.h"
#include "cmd_notify.h"
#include "cmd_list_sets.h"
#include "cmd_list_reports.h"
#include "cmd_list_params.h"
#include "cmd_get_params.h"
#include "cmd_get_key_params.h"
#include "cmd_set_update.h"
#include "cmd_set_subscribe.h"
#include "cmd_report_subscribe.h"
#include "cmd_get_config.h"
#include "cmd_get_report.h"
#include "cmd_get_history.h"
#include "cmd_get_latest.h"
#include "cmd_get_latest_set.h"
#include "cmd_get_summary.h"
#include "cmd_search_data.h"
#include "cmd_get_data.h"
#include "cmd_param_subscribe.h"
#include "cmd_param_unsubscribe.h"
#include "cmd_param_add.h"
#include "cmd_param_remove.h"
#include "cmd_custom_subscribe.h"

namespace p = std::placeholders;

#define MAP_CMD_TAG( _tag , cmd ) \
	if( _tag == tag ) return new cmd(vars,*this);

#define MAP_TAG_CMD( type , tag ) \
	if( typeid(type) == typeid(*cmd) ) return tag;

SzbaseProt::SzbaseProt( Vars& vars )
	: vars(vars)
	, def_param_uuid(boost::lexical_cast<std::string>(boost::uuids::random_generator()()))
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
	for ( auto i : user_params )
	{
		try{
			vars.get_updater().remove_param( i.first.first , i.second );
		} catch ( szbase_error& ) { } 
	}
}

Command* SzbaseProt::cmd_from_tag( const std::string& tag )
{
	MAP_CMD_TAG( "s"                 , SetRcv              );
	MAP_CMD_TAG( "set"               , SetRcv              );
	MAP_CMD_TAG( "list_sets"         , ListSetsRcv         );
	MAP_CMD_TAG( "list_reports"      , ListReportsRcv      );
	MAP_CMD_TAG( "list_params"       , ListParamsRcv       );
	MAP_CMD_TAG( "get_params"        , GetParamsRcv        );
	MAP_CMD_TAG( "get_key_params"    , GetParamsForKeyRcv  );
	MAP_CMD_TAG( "get_set"           , GetSetRcv           );
	MAP_CMD_TAG( "get_report"        , GetReportRcv        );
	MAP_CMD_TAG( "set_update"        , SetUpdateRcv        );
	MAP_CMD_TAG( "set_subscribe"     , SetSubscribeRcv     );
	MAP_CMD_TAG( "report_subscribe"  , ReportSubscribeRcv  );
	MAP_CMD_TAG( "get_options"       , GetConfigRcv        );
	MAP_CMD_TAG( "get_history"       , GetHistoryRcv       );
	MAP_CMD_TAG( "get_latest"        , GetLatestRcv        );
	MAP_CMD_TAG( "get_latest_set"    , GetLatestFromSetRcv );
	MAP_CMD_TAG( "get_summary"       , GetSummaryRcv       );
	MAP_CMD_TAG( "search_data"       , SearchDataRcv       );
	MAP_CMD_TAG( "get_data"          , GetDataRcv          );
	MAP_CMD_TAG( "param_subscribe"   , ParamSubscribeRcv   );
	MAP_CMD_TAG( "param_unsubscribe" , ParamUnsubscribeRcv );
	MAP_CMD_TAG( "add_param"         , ParamAddRcv         );
	MAP_CMD_TAG( "remove_param"      , ParamRemoveRcv      );
	MAP_CMD_TAG( "custom_subscribe"   , CustomSubscribeRcv   );
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

void SzbaseProt::on_param_changed( const std::string& pname )
{
    if( sub_params.count( pname ) )
		send_cmd( new NotifySnd( get_client_param_name (pname )) );
}

void SzbaseProt::on_param_value_changed( Param::const_ptr p , double value , ProbeType pt )
{
	(void)value;
	
	if( current_set
	 && current_set->has_param( p->get_name() ) 
	 && current_pt == pt )
		send_cmd( new ValueSnd(p,pt) );

	if( custom_params.find(p->get_name()) != custom_params.end() 
	 && custom_pt == pt )
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

	sub_set = vars.get_updater().subscribe_params( *(s) , pt );

	for( auto itr=current_set->begin() ; itr!=current_set->end() ; ++itr )
	{
		auto p = vars.get_params().get_param( *itr );
		if( p )
			send_cmd( new ValueSnd(p,pt) );
		else
			sz_log(1, "Unknown param (%s) in set (%s)",
					itr->c_str() , s->get_name().c_str() );
	}
}

void SzbaseProt::subscribe_custom( ProbeType pt, const std::vector<std::string>& params )
{
	
	custom_params.clear();
	/* Cancel custom subscribtion if no params given */
	if (params.empty()) {
		sub_custom.cancel();
		return;
	}
	for (auto cp = params.begin(); cp != params.end(); ++cp) 
		custom_params.insert(*cp);
	custom_pt = pt;

	/** Prevent from sending values double if they values changed on subscribe */
	boost::signals2::shared_connection_block block(conn_param_value);

	sub_custom = vars.get_updater().subscribe_params( params , pt );
	//sub_custom = vars.get_updater().subscribe_params( params , pt );
	for( auto itr=params.begin() ; itr!=params.end() ; ++itr ) {
	
		auto p = vars.get_params().get_param( *itr );
		if( p )
			send_cmd( new ValueSnd(p,pt) );
		else
			sz_log(1, "Unknown param (%s) in custom set", itr->c_str());
	}
}

void SzbaseProt::subscribe_param( const std::string& name) 
{
	const std::string& internal_name = get_mapped_param_name( name );
	sub_params.insert(
		std::make_pair( internal_name ,
						vars.get_updater().subscribe_param( internal_name ,
															boost::optional<ProbeType>() ,
															false ) ) );
}

void SzbaseProt::unsubscribe_param( const std::string& name )
{
	const std::string& internal_name = get_mapped_param_name( name );
	auto it = sub_params.find( internal_name );
	if( it != sub_params.end() )
		sub_params.erase( it );
}

void SzbaseProt::add_param( const std::string& param
						  , const std::string& base
						  , const std::string& formula
						  , const std::string& type
						  , int prec
						  , unsigned start_time)
{
	std::string internal_name = vars.get_updater().add_param( param , base , formula , def_param_uuid , type , prec , start_time );

	user_params.insert( std::make_pair( std::make_pair( base , param ) , internal_name ) );
	user_params_inverted.insert( std::make_pair( internal_name , param ) );
}

const std::string& SzbaseProt::get_client_param_name( const std::string& name ) const 
{
	auto i = user_params_inverted.find( name );

	return i == user_params_inverted.end() ? name : i->second;
}

const std::string& SzbaseProt::get_mapped_param_name( const std::string& name ) const 
{
	auto i = user_params.find( std::make_pair( vars.get_szbase()->get_base_name() , name ) );

	return i == user_params.end() ? name : i->second;
}

std::shared_ptr<const Param> SzbaseProt::get_param( const std::string& name ) const
{
	return get_param( get_mapped_param_name( name ) );
}

void SzbaseProt::remove_param(const std::string& base, const std::string& pname)
{
	auto i = user_params.find( std::make_pair( base , pname ) );
	if ( i == user_params.end() )
		throw szbase_param_not_found_error("param not found");

	vars.get_updater().remove_param( i->first.first, i->second );

	user_params_inverted.erase( i->second );
	user_params.erase( i );
}

