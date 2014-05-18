#ifndef __WELCOME_PROTOCOL_H__
#define __WELCOME_PROTOCOL_H__

#include "locations/protocol.h"

class WelcomeProt : public Protocol {
public:
	virtual Command* cmd_from_tag( const std::string& tag );
	virtual std::string tag_from_cmd( const Command* cmd );

};

#endif /* end of include guard: __WELCOME_PROTOCOL_H__ */

