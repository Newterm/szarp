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

			/* FIXME: Because request_location cat delete location
			 * apply should be called before this. This is pretty ugly
			 * and should be fixed so apply is safe in case of object
			 * deletion.
			 * (19/05/2014 10:13, jkotur)
			 */
			apply();

			GlobalService::get_service().post(
				std::bind(&Protocol::request_location,&prot,loc) );
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

