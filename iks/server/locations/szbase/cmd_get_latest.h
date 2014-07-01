#ifndef __SZBASE_CMD_GET_LATEST_H__
#define __SZBASE_CMD_GET_LATEST_H__

#include <ctime>
#include <iterator>

#include <boost/format.hpp>

#include "data/vars.h"
#include "utils/tokens.h"
#include "utils/exception.h"

#include "locations/command.h"
#include "locations/error_codes.h"

class GetLatestRcv : public Command {
public:
	GetLatestRcv( Vars& vars , Protocol& prot )
		: vars(vars)
	{
		(void)prot;
		set_next( std::bind(&GetLatestRcv::parse_command,this,std::placeholders::_1) );
	}

	virtual ~GetLatestRcv()
	{
	}

protected:
	void parse_command( const std::string& data )
	{
		auto r = find_quotation( data );
		std::string name(r.begin(),r.end());

		auto l = find_after_quotation( data ) ;
		std::string probe_type(l.begin(),l.end());

		auto param = vars.get_params().get_param( name );

		if( !param ) {
			fail( ErrorCodes::unknown_param );
			return;
		}

		std::unique_ptr<ProbeType> pt;

		try {
			pt.reset( new ProbeType( probe_type ) );
		} catch( parse_error& e ) {
			fail( ErrorCodes::wrong_probe_type );
			return;
		};

		try {
			auto t = vars.get_szbase()->get_latest( name , *pt );

			apply( str( boost::format("%d") % t ) );
		} catch( szbase_error& e ) {
			fail( ErrorCodes::szbase_error , e.what() );
		}
	}

	Vars& vars;
};

#endif /* end of include guard: __SZBASE_CMD_GET_LATEST_H__ */

