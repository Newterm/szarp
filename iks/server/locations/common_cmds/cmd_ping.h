#ifndef __CMD_PING_H__
#define __CMD_PING_H__

#include "locations/command.h"

class Protocol;
class LocationsList;
class Vars;
class SzbaseProt;

class CmdPingRcv : public Command {
public:
	CmdPingRcv(Vars&, SzbaseProt&) {
		set_next([this](const std::string& data){ parse_command(data); });
	}

	CmdPingRcv(Protocol&, LocationsList&) {
		set_next([this](const std::string& data){ parse_command(data); });
	}

	void parse_command(const std::string& data) {
		try {
			apply(data);
		} catch( const std::exception& e ) {
			fail( ErrorCodes::internal_error , e.what() );
		}
	}
};

#endif

