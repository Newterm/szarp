#ifndef __SERVER_CMD_REPORT_SUBSCRIBE_H__
#define __SERVER_CMD_REPORT_SUBSCRIBE_H__

#include <boost/algorithm/string.hpp>

#include "locations/command.h"

class ReportSubscribeRcv : public Command {
public:
	ReportSubscribeRcv( Vars& vars , SzbaseProt& prot )
		: vars(vars) , prot(prot)
	{
		set_next( std::bind(&ReportSubscribeRcv::parse_command,this,std::placeholders::_1) );
	}

	void parse_command( const std::string& line )
	{
		namespace ba = boost::algorithm;

		std::string set_name;

		try {
			auto r = find_quotation( line );
			set_name.assign(r.begin(),r.end());
		} catch( parse_error& e ) {
			fail( ErrorCodes::ill_formed );
			return;
		}

		if( set_name.empty() ) {
			/** set current set to none */
			prot.set_current_set();
			apply();
			return;
		}

		auto s = vars.get_sets().get_report( set_name );

		if( !s ) {
			fail( ErrorCodes::unknown_set );
			return;
		}

		/** subscribe to set */
		prot.set_current_set( s , ProbeType() );
		apply();
	}

protected:
	Vars& vars;
	SzbaseProt& prot;

};

#endif /* end of include guard: __SERVER_CMD_SET_SUBSCRIBE_H__ */
