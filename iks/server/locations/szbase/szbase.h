#ifndef __SZBASE_PROTOCOL_H__
#define __SZBASE_PROTOCOL_H__

#include "locations/protocol.h"

#include "data/vars.h"

class SzbaseProt : public Protocol {
public:
	SzbaseProt( const std::string& szarp_base );

	virtual Command* cmd_from_tag( const std::string& tag );
	virtual std::string tag_from_cmd( const Command* cmd );

private:
	Vars vars;
};

#endif /* end of include guard: __SZBASE_PROTOCOL_H__ */

