#ifndef __WELCOME_PROTOCOL_H__
#define __WELCOME_PROTOCOL_H__

#include "locations/protocol.h"
#include "locations/locations_list.h"
#include "locations/config_container.h"

class WelcomeProt : public Protocol, public ConfigContainer {
public:
	WelcomeProt( LocationsList& locs, Config& config );
	virtual ~WelcomeProt();

	virtual Command* cmd_from_tag( const std::string& tag );
	virtual std::string tag_from_cmd( const Command* cmd );

protected:
	void on_remote_added  ( const std::string& tag );
	void on_remote_removed( const std::string& tag );

	LocationsList& locs;

	boost::signals2::scoped_connection conn_add;
	boost::signals2::scoped_connection conn_rm ;

};

#endif /* end of include guard: __WELCOME_PROTOCOL_H__ */

