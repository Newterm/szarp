#ifndef __SZBASE_CMD_GET_LATEST_SET_H__
#define __SZBASE_CMD_GET_LATEST_SET_H__

#include <ctime>
#include <iterator>

#include <boost/format.hpp>

#include "data/vars.h"
#include "utils/tokens.h"
#include "utils/exception.h"

#include "locations/command.h"
#include "locations/error_codes.h"

class GetLatestFromSetRcv : public Command {
public:
	GetLatestFromSetRcv( Vars& vars , Protocol& prot )
		: vars(vars)
	{
		(void)prot;
		set_next( std::bind(&GetLatestFromSetRcv::parse_command,this,std::placeholders::_1) );
	}

	virtual ~GetLatestFromSetRcv()
	{
	}

protected:
	void parse_command( const std::string& data )
	{
		auto r = find_quotation( data );
		std::string set_name(r.begin(),r.end());

		auto l = find_after_quotation( data ) ;
		std::string probe_type(l.begin(),l.end());

		auto set = vars.get_sets().get_set( set_name );

		if( !set ) {
			fail( ErrorCodes::unknown_set );
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
			time_t t_max = 0;
			for (auto param = set->begin(); param != set->end(); ++param) {
				std::string param_name = *param;
				auto t = vars.get_szbase()->get_latest( param_name , *pt );
				if (t > t_max) {
					t_max = t;
				}
			}

			apply( str( boost::format("%d") % t_max ) );
		} catch( szbase_error& e ) {
			fail( ErrorCodes::szbase_error , e.what() );
		}
	}

	Vars& vars;
};

#endif /* end of include guard: __SZBASE_CMD_GET_LATEST_SET_H__ */

