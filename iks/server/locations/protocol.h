#ifndef __LOCATIONS_PROTOCOL_H__
#define __LOCATIONS_PROTOCOL_H__

#include <memory>

#include "command.h"

#include "utils/signals.h"
#include "locations/location.h"

class Protocol;

typedef boost::signals2::signal<void (std::shared_ptr<Protocol>)> sig_protocol;
typedef sig_protocol::slot_type sig_protocol_slot;

class Protocol {
public:
	typedef std::shared_ptr<Protocol> ptr;
	typedef std::shared_ptr<const Protocol> const_ptr;

	virtual ~Protocol() {}

	virtual Command* cmd_from_tag( const std::string& tag ) =0;
	virtual std::string tag_from_cmd( const Command* cmd ) =0;

	void request_location( Location::ptr loc )
	{	emit_request_location(loc); }
	slot_connection on_location_request( const sig_location_slot& slot ) const
	{	return emit_request_location.connect( slot ); }

protected:
	mutable sig_location emit_request_location;
};

#endif /* end of include guard: __LOCATIONS_PROTOCOL_H__ */

