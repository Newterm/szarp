#ifndef __WELCOME_PROTOCOL_H__
#define __WELCOME_PROTOCOL_H__

#include "locations/protocol.h"
#include "locations/locations_list.h"

class WelcomeProt : public Protocol {
public:
	WelcomeProt( LocationsList& locs ) : locs(locs) {}
	virtual ~WelcomeProt() {}

	virtual Command* cmd_from_tag( const std::string& tag );
	virtual std::string tag_from_cmd( const Command* cmd );

protected:
	LocationsList& locs;

};

#endif /* end of include guard: __WELCOME_PROTOCOL_H__ */

