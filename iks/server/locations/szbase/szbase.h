#ifndef __SZBASE_LOCATION_H__
#define __SZBASE_LOCATION_H__

#include "locations/protocol_location.h"

#include "data/vars.h"

class SzbaseLoc : public ProtocolLocation , public Protocol {
public:
	SzbaseLoc( Vars& vars , Connection* con );

	virtual Command* cmd_from_tag( const std::string& tag );
	virtual std::string tag_from_cmd( const Command* cmd );

private:
	Vars& vars;
};

#endif /* end of include guard: __SZBASE_LOCATION_H__ */

