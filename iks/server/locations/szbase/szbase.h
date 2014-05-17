#ifndef __SZBASE_PROTOCOL_H__
#define __SZBASE_PROTOCOL_H__

#include "locations/protocol.h"
#include "locations/protocol_location.h"

#include "data/vars.h"

class SzbaseProt : public Protocol {
public:
	SzbaseProt( const std::string& szarp_base );

	virtual Command* cmd_from_tag( const std::string& tag );
	virtual std::string tag_from_cmd( const Command* cmd );

private:
	Vars vars;
};

class SzbaseLocation : public ProtocolLocation {
public:
	SzbaseLocation( const std::string& szarp_base , Connection* conn = NULL )
		: ProtocolLocation( std::make_shared<SzbaseProt>(szarp_base) , conn )
	{}

	virtual ~SzbaseLocation()
	{}

};

#endif /* end of include guard: __SZBASE_PROTOCOL_H__ */

