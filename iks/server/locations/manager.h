#ifndef __LOCATIONS_MANAGER_H__
#define __LOCATIONS_MANAGER_H__

#include <unordered_map>

#include "location.h"

#include "net/connection.h"

#include "data/vars.h"

class LocationsMgr {
public:
	LocationsMgr();
	virtual ~LocationsMgr();

	void on_new_connection( Connection* conn );
	void on_disconnected  ( Connection* conn );

private:
	void new_location( Location::ptr nloc , Location::ptr oloc = Location::ptr() );

	std::unordered_map<Connection*,Location::ptr> locations;

	std::string base;
};

#endif /* end of include guard: __LOCATIONS_MANAGER_H__ */

