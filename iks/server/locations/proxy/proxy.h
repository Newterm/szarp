#ifndef __PROXY_LOCATION_H__
#define __PROXY_LOCATION_H__

#include <memory>

#include "locations/location.h"

class ProxyLoc : public Location {
public:
	ProxyLoc( const std::string& address , unsigned port );
	virtual ~ProxyLoc();

protected:
	void connect( const std::string& address , unsigned port );
	virtual void parse_line( const std::string& line );

private:
	std::string address;
	unsigned port;

	std::shared_ptr<Connection> remote;
};

#endif /* end of include guard: __WELCOME_PROTOCOL_H__ */


