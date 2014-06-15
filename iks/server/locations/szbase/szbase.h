#ifndef __SZBASE_PROTOCOL_H__
#define __SZBASE_PROTOCOL_H__

#include "locations/protocol.h"
#include "locations/protocol_location.h"

#include "data/vars.h"

class SzbaseProt : public Protocol {
public:
	SzbaseProt( Vars& vars );
	virtual ~SzbaseProt();

	virtual Command* cmd_from_tag( const std::string& tag );
	virtual std::string tag_from_cmd( const Command* cmd );

	void set_current_set(
			Set::const_ptr s = Set::const_ptr() ,
			ProbeType pt = ProbeType() );

private:
	void on_param_value_changed( Param::const_ptr p , double value , ProbeType pt );

	Vars& vars;

	ProbeType current_pt;
	Set::const_ptr current_set;

	boost::signals2::scoped_connection conn_param;

	ParamsUpdater::Subscription sub_params;

};

class SzbaseLocation : public ProtocolLocation {
public:
	SzbaseLocation(
			const std::string& name ,
			Vars* vars ,
			Connection* conn = NULL )
		: ProtocolLocation(
				name ,
				std::make_shared<SzbaseProt>(*vars) ,
				conn )
	{}

	virtual ~SzbaseLocation()
	{}

};

#endif /* end of include guard: __SZBASE_PROTOCOL_H__ */

