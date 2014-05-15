#ifndef __WELCOME_CMD_HELLO_H__
#define __WELCOME_CMD_HELLO_H__

#include "global_service.h"
#include "locations/command.h"
#include "locations/location.h"
#include "locations/proxy/proxy.h"
#include "locations/szbase/szbase.h"

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
		if( line == "proxy" ) {
			auto prx = std::make_shared<ProxyLoc>( "localhost" , 9002  );
			GlobalService::get_service().post(
				std::bind(&Protocol::request_location,&prot,prx) );
			apply();
			return;
		}

		try {
			auto szb = std::make_shared<SzbaseProt>( line );
			prot.request_protocol( szb );
			apply();
		} catch( ... ) {
			fail( ErrorCodes::szbase_error );
		}
	}

protected:
	Protocol& prot;

};

#endif /* end of include guard: __WELCOME_CMD_HELLO_H__ */

