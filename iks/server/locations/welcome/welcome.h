#ifndef __WELCOME_CONNECTION_H__
#define __WELCOME_CONNECTION_H__

#include "locations/location.h"

class WelcomeLoc : public Location {
public:
	WelcomeLoc( Connection* con ) : Location(con) {}

	virtual void parse_line( const std::string& line );
};

#endif /* end of include guard: __WELCOME_CONNECTION_H__ */

