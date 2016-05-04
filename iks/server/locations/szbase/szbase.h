#ifndef __SZBASE_PROTOCOL_H__
#define __SZBASE_PROTOCOL_H__

#include "locations/protocol.h"
#include "locations/protocol_location.h"

#include "data/vars.h"

#include <unordered_set>

class SzbaseProt : public Protocol {
public:
	SzbaseProt( Vars& vars );
	virtual ~SzbaseProt();

	virtual Command* cmd_from_tag( const std::string& tag );
	virtual std::string tag_from_cmd( const Command* cmd );

	void set_current_set(
			Set::const_ptr s = Set::const_ptr() ,
			ProbeType pt = ProbeType() );


	void subscribe_custom( ProbeType pt, const std::vector<std::string>& params );

	void subscribe_param( const std::string& name );

	void unsubscribe_param( const std::string& name );

	void add_param( const std::string& param
				  , const std::string& base
				  , const std::string& formula
				  , const std::string& type
				  , int prec
				  , unsigned start_time);

	void remove_param( const std::string& base , const std::string& param );

	const std::string& get_client_param_name( const std::string& name ) const;
	const std::string& get_mapped_param_name( const std::string& name ) const;

	std::shared_ptr<const Param> get_param( const std::string& name ) const;
private:
	void on_param_changed( const std::string& pname );
	void on_param_value_changed( Param::const_ptr p , double value , ProbeType pt );

	Vars& vars;
	
	ProbeType custom_pt;
	ProbeType current_pt;
	Set::const_ptr current_set;

	boost::signals2::scoped_connection conn_param;
	boost::signals2::scoped_connection conn_param_value;

	ParamsUpdater::Subscription sub_set;
	ParamsUpdater::Subscription sub_custom;

	std::unordered_set<std::string> custom_params;
	std::multimap<std::string , ParamsUpdater::Subscription > sub_params;
	std::map< std::pair< std::string , std::string >, std::string> user_params;
	std::map< std::string , std::string> user_params_inverted;
	
	std::string def_param_uuid;
};

class SzbaseLocation : public ProtocolLocation {
public:
	SzbaseLocation(
			const std::string& name ,
			Vars& vars ,
			Connection* conn = NULL )
		: ProtocolLocation(
				name ,
				std::make_shared<SzbaseProt>(vars) ,
				conn )
	{}

	virtual ~SzbaseLocation()
	{}

};

#endif /* end of include guard: __SZBASE_PROTOCOL_H__ */

