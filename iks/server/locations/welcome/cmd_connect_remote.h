#ifndef __WELCOME_CMD_CONNECT_REMOTE_H__
#define __WELCOME_CMD_CONNECT_REMOTE_H__

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include "global_service.h"
#include "locations/command.h"

#include "locations/locations_list.h"

#include "locations/szbase/szbase.h"

class CmdConnectRemoteRcv : public Command {
public:
	CmdConnectRemoteRcv( Protocol& prot , LocationsList& locs )
		: prot(prot) , locs(locs)
	{
		set_next( std::bind(&CmdConnectRemoteRcv::parse_command,this,std::placeholders::_1) );
	}

	virtual ~CmdConnectRemoteRcv()
	{
	}

	void parse_command( const std::string& line )
	{
		try {
			auto loc = locs.make_location( line );

			if( !loc ) {
				fail( ErrorCodes::unknown_remote );
				return;
			}

			/* FIXME: Because request_location cat delete location
			 * apply should be called before this. This is pretty ugly
			 * and should be fixed so apply is safe in case of object
			 * deletion.
			 * (19/05/2014 10:13, jkotur)
			 */
			apply();

			GlobalService::get_service().post(
				std::bind(&Protocol::request_location,&prot,loc) );
		} catch( const std::exception& e ) {
			fail( ErrorCodes::internal_error , e.what() );
		}
	}

protected:
	Protocol& prot;
	LocationsList& locs;

};

#endif /* end of include guard: __WELCOME_CMD_CONNECT_REMOTE_H__ */

