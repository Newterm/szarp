#ifndef __LOCATIONS_LOCATIONS_LIST_H__
#define __LOCATIONS_LOCATIONS_LIST_H__

#include <unordered_map>
#include <functional>

#include "location.h"
#include "protocol.h"

#include "utils/signals.h"
#include "utils/iterator.h"

class LocationsList {
	typedef std::unordered_map<std::string,std::function<Location::ptr ()>> GenetratorsMap;

public:
	LocationsList() {}
	virtual ~LocationsList() {}

	class iterator : public key_iterator<GenetratorsMap> {
		friend class LocationsList;
	public:
		iterator( const iterator& itr ) : key_iterator(itr.itr) {}
		iterator( const typename GenetratorsMap::const_iterator& itr ) : key_iterator(itr) {}
	};

	iterator begin() const { return iterator(locations_generator.begin()); }
	iterator end  () const { return iterator(locations_generator.end  ()); }

	template<class T,class... Args> void register_location( const std::string& tag , Args... args )
	{
		std::function<Location::ptr ()> f = 
			std::bind( &LocationsList::create_location<T,Args&...> , this , tag , std::forward<Args>(args)... );
		locations_generator[ tag ] = f;

		emit_location_added( tag );
	}

	void remove_location( const std::string& tag )
	{
		if( !locations_generator.count(tag) )
			return;

		locations_generator.erase( tag );

		emit_location_removed( tag );
	}

	iterator remove_location( const iterator& itr )
	{
		auto tag = *itr;
		auto i = locations_generator.erase( itr.itr );

		emit_location_removed( tag );

		return iterator(i);
	}

	Location::ptr make_location( const std::string& tag )
	{
		auto itr = locations_generator.find(tag);
		return itr != locations_generator.end() ?
			itr->second() :
			Location::ptr();
	}

	slot_connection on_location_added  ( const sig_string_slot& slot )
	{	return emit_location_added.connect( slot ); }
	slot_connection on_location_removed( const sig_string_slot& slot )
	{	return emit_location_removed.connect( slot ); }

private:
	/* Can be specialized to other types if needed (17/05/2014 08:55, jkotur) */
	template<class T,class... Args> Location::ptr create_location( const std::string& tag , Args... args )
	{
		return std::make_shared<T>( args... );
	}

	GenetratorsMap locations_generator;

	sig_string emit_location_added;
	sig_string emit_location_removed;

};

#endif /* end of include guard: __LOCATIONS_LOCATIONS_LIST_H__ */

