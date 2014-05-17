#ifndef __LOCATIONS_LOCATIONS_LIST_H__
#define __LOCATIONS_LOCATIONS_LIST_H__

#include <unordered_map>
#include <functional>

#include "location.h"
#include "protocol.h"

class LocationsList {
public:
	LocationsList() {}
	virtual ~LocationsList() {}

	template<class T,class... Args> void register_location( const std::string& tag , Args... args )
	{
		std::function<Location::ptr ()> f = 
			std::bind( &LocationsList::create_location<T,Args&...> , this , tag , std::forward<Args>(args)... );
		locations_generator[ tag ] = f;
	}

	void remove_location( const std::string& tag )
	{
		locations_generator.erase( tag );
	}

	Location::ptr make_location( const std::string& tag )
	{
		return locations_generator[tag]();
	}

private:
	/* Can be specialized to other types if needed (17/05/2014 08:55, jkotur) */
	template<class T,class... Args> Location::ptr create_location( const std::string& tag , Args... args )
	{
		return std::make_shared<T>( args... );
	}

	std::unordered_map<std::string,std::function<Location::ptr ()>> locations_generator;

};

#endif /* end of include guard: __LOCATIONS_LOCATIONS_LIST_H__ */

