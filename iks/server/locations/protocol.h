#ifndef __LOCATIONS_PROTOCOL_H__
#define __LOCATIONS_PROTOCOL_H__

#include "command.h"

class Protocol {
public:
	virtual ~Protocol() {}

	virtual Command* cmd_from_tag( const std::string& tag ) =0;
	virtual std::string tag_from_cmd( const Command* cmd ) =0;
};

#endif /* end of include guard: __LOCATIONS_PROTOCOL_H__ */

