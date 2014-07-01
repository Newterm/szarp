#ifndef __PROXY_LOCATION_H__
#define __PROXY_LOCATION_H__

#include <memory>

#include "net/tcp_client.h"

#include "locations/location.h"

class ProxyLoc : public Location {
public:
	ProxyLoc( const std::string& name , const std::string& address , unsigned port );
	virtual ~ProxyLoc();

protected:
	void fwd_connect();
	void get_response( const std::string& line );

	void connect( const std::string& address , unsigned port );
	virtual void parse_line( const std::string& line );

private:
	std::string address;
	unsigned port;

	std::shared_ptr<TcpClient> remote;

	boost::signals2::scoped_connection conn_line;
};

#endif /* end of include guard: __WELCOME_PROTOCOL_H__ */


