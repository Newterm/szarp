#ifndef __WELCOME_CONNECTION_H__
#define __WELCOME_CONNECTION_H__

#include "locations/protocol_location.h"

class WelcomeLoc : public ProtocolLocation , public Protocol {
public:
	WelcomeLoc( Connection* con );

	virtual Command* cmd_from_tag( const std::string& tag );
	virtual std::string tag_from_cmd( const Command* cmd );

};

#endif /* end of include guard: __WELCOME_CONNECTION_H__ */

