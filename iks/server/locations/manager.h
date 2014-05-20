#ifndef __LOCATIONS_MANAGER_H__
#define __LOCATIONS_MANAGER_H__

#include <unordered_map>

#include "location.h"

#include "net/connection.h"

#include "data/vars.h"

#include "locations/locations_list.h"

#include "locations/remotes_updater/remotes_updater.h"

class LocationsMgr {
public:
	LocationsMgr();
	virtual ~LocationsMgr();

	void on_new_connection( Connection* conn );
	void on_disconnected  ( Connection* conn );

private:
	void new_location( Location::ptr nloc , std::weak_ptr<Location> oloc = std::weak_ptr<Location>() );

	LocationsList loc_factory;

	std::unordered_map<Connection*,Location::ptr> locations;

	std::unordered_map<std::string,RemotesUpdater::ptr> updaters;

	std::string base;
};

#endif /* end of include guard: __LOCATIONS_MANAGER_H__ */

