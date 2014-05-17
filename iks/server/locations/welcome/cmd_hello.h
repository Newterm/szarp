#ifndef __WELCOME_CMD_HELLO_H__
#define __WELCOME_CMD_HELLO_H__

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include "global_service.h"
#include "locations/command.h"

#include "locations/locations_list.h"

namespace balgo = boost::algorithm;

class CmdHelloRcv : public Command {
public:
	CmdHelloRcv( Protocol& prot , LocationsList& locs )
		: prot(prot) , locs(locs)
	{
		set_next( std::bind(&CmdHelloRcv::parse_command,this,std::placeholders::_1) );
	}

	virtual ~CmdHelloRcv()
	{
	}

	void parse_command( const std::string& line )
	{
		try {
			auto loc = locs.make_location( line );

			GlobalService::get_service().post(
				std::bind(&Protocol::request_location,&prot,loc) );

			apply();
		} catch( ... ) {
			/* TODO: Better error handling (17/05/2014 09:36, jkotur) */
			fail( ErrorCodes::internal_error );
		}
	}

protected:
	Protocol& prot;
	LocationsList& locs;

};

#endif /* end of include guard: __WELCOME_CMD_HELLO_H__ */

