#ifndef __SERVER_CMD_REPORT_UPDATE_H__
#define __SERVER_CMD_REPORT_UPDATE_H__

#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "locations/command.h"

#include "utils/tokens.h"

class GetReportRcv : public Command {
public:
	GetReportRcv( Vars& vars , Protocol& prot )
		: vars(vars)
	{
		(void)prot;
		set_next( std::bind(&GetReportRcv::parse_command,this,std::placeholders::_1) );
	}

	void parse_command( const std::string& line )
	{
		std::string name;

		try {
			auto r = find_quotation( line );
			name.assign(r.begin(),r.end());
		} catch( parse_error& e ) {
			fail( ErrorCodes::ill_formed );
			return;
		}

		auto s = vars.get_sets().get_report( name );

		if( s )
			apply( s->to_json(false) );
		else
			fail( ErrorCodes::unknown_set );
	}

protected:
	Vars& vars;
};

#endif /* end of include guard: __SERVER_CMD_SET_UPDATE_H__ */

