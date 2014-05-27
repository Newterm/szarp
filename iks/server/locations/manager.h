#ifndef __LOCATIONS_MANAGER_H__
#define __LOCATIONS_MANAGER_H__

#include <unordered_map>

#include "location.h"

#include "net/connection.h"

#include "data/vars_cache.h"

#include "locations/locations_list.h"
#include "locations/remotes_updater/remotes_updater.h"

#include "utils/config.h"

class LocationsMgr {
public:
	LocationsMgr();
	virtual ~LocationsMgr();

	void add_locations( const CfgSections& cfg );
	void add_location( const std::string& name , const CfgPairs& cfg );

	void on_new_connection( Connection* conn );
	void on_disconnected  ( Connection* conn );

private:
	void new_location( Location::ptr nloc , std::weak_ptr<Location> oloc = std::weak_ptr<Location>() );

	void add_szbase( const std::string& name , const CfgPairs& cfg );
	void add_proxy ( const std::string& name , const CfgPairs& cfg );

	LocationsList loc_factory;
	VarsCache vars_cache;

	std::unordered_map<Connection*,Location::ptr> locations;
	std::unordered_map<std::string,RemotesUpdater::ptr> updaters;
};

#endif /* end of include guard: __LOCATIONS_MANAGER_H__ */

