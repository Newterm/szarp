#ifndef __WELCOME_CMD_HELLO_H__
#define __WELCOME_CMD_HELLO_H__

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include "global_service.h"
#include "locations/command.h"
#include "locations/location.h"
#include "locations/proxy/proxy.h"
#include "locations/szbase/szbase.h"

namespace balgo = boost::algorithm;

class CmdHelloRcv : public Command {
public:
	CmdHelloRcv( Protocol& prot )
		: prot(prot)
	{
		set_next( std::bind(&CmdHelloRcv::parse_command,this,std::placeholders::_1) );
	}

	virtual ~CmdHelloRcv()
	{
	}

	void parse_command( const std::string& line )
	{
		std::vector<std::string> tags;
		balgo::split(
				tags , line ,
				balgo::is_any_of(" "), balgo::token_compress_on );

		if( tags[0] == "proxy" ) {
			if( tags.size() < 3 ) {
				fail( ErrorCodes::ill_formed );
				return;
			}

			auto prx = std::make_shared<ProxyLoc>(
				tags[1] ,
				boost::lexical_cast<unsigned>(tags[2]) );

			GlobalService::get_service().post(
				std::bind(&Protocol::request_location,&prot,prx) );

			apply();
		} else if( tags[0] == "szbase" ) {
			if( tags.size() < 2 ) {
				fail( ErrorCodes::ill_formed );
				return;
			}

			try {
				auto szb = std::make_shared<SzbaseProt>( tags[1] );
				prot.request_protocol( szb );
				apply();
			} catch( ... ) {
				fail( ErrorCodes::szbase_error );
			}
		} else {
			fail( ErrorCodes::ill_formed );
		}
	}

protected:
	Protocol& prot;

};

#endif /* end of include guard: __WELCOME_CMD_HELLO_H__ */

