#ifndef __PROTOCOL_LOCATION_H__
#define __PROTOCOL_LOCATION_H__

#include <memory>
#include <unordered_map>

#include "location.h"
#include "command.h"
#include "protocol.h"
#include "error_codes.h"

#include "net/connection.h"

class ProtocolLocation : public Location {
public:
	ProtocolLocation( Protocol::ptr protocol = Protocol::ptr() , Connection* connection = NULL );
	virtual ~ProtocolLocation();

	void set_protocol( Protocol::ptr protocol );

	virtual void request_location( Location::ptr loc );

private:
	void new_cmd(
			Command* cmd ,
			const std::string& tag , id_t id ,
			const Command::to_send& in_data = Command::to_send() );
	void erase_cmd( Command* cmd );
	void erase_cmd( id_t id );

	id_t generate_id();

	void send_cmd( Command* cmd );

	void send_response(
		Command::ResponseType type ,
		const std::string& data ,
		Command* cmd );

	void send_fail( ErrorCodes code , const std::string& msg = "" );

	virtual void parse_line( const std::string& line );

	Protocol::ptr protocol;

	std::unordered_map<id_t,Command*> commands;

#if GCC_VERSION >= 40600
	std::default_random_engine rnd;
#endif

	boost::signals2::scoped_connection conn_send_cmd;
	boost::signals2::scoped_connection conn_location_request;
};

#endif /* end of include guard: __PROTOCOL_LOCATION_H__ */

