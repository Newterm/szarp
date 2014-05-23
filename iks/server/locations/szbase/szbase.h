#ifndef __SZBASE_PROTOCOL_H__
#define __SZBASE_PROTOCOL_H__

#include "locations/protocol.h"
#include "locations/protocol_location.h"

#include "data/vars.h"

class SzbaseProt : public Protocol {
public:
	SzbaseProt(
			const std::string& szarp_base ,
			const std::string& prober_address = "127.0.0.1" ,
			unsigned prober_port = 8090 );

	virtual Command* cmd_from_tag( const std::string& tag );
	virtual std::string tag_from_cmd( const Command* cmd );

	void set_current_set( Set::const_ptr s = Set::const_ptr() );

private:
	void on_param_value_changed( Param::const_ptr p );

	Vars vars;

	Set::const_ptr current_set;

	boost::signals2::scoped_connection conn_param;
};

class SzbaseLocation : public ProtocolLocation {
public:
	SzbaseLocation(
			const std::string& name ,
			const std::string& szarp_base ,
			const std::string& prober_address = "127.0.0.1" ,
			unsigned prober_port = 8090 ,
			Connection* conn = NULL )
		: ProtocolLocation(
				name ,
				std::make_shared<SzbaseProt>(
					szarp_base,prober_address,prober_port) ,
				conn )
	{}

	virtual ~SzbaseLocation()
	{}

};

#endif /* end of include guard: __SZBASE_PROTOCOL_H__ */

