#ifndef __SERVER_CMD_PARAM_UNSUBSCRIBE_H__
#define __SERVER_CMD_PARAM_UNSUBSCRIBE_H__

#include <boost/algorithm/string.hpp>

#include "locations/command.h"

class ParamUnsubscribeRcv : public Command {
public:
	ParamUnsubscribeRcv( Vars& vars , SzbaseProt& prot )
		: vars(vars) , prot(prot)
	{
		set_next( std::bind(&ParamUnsubscribeRcv::parse_command,this,std::placeholders::_1) );
	}

	virtual ~ParamUnsubscribeRcv()
	{
	}

	void parse_command( const std::string& line )
	{
		namespace ba = boost::algorithm;

		auto r = find_quotation( line );
		std::string name(r.begin(),r.end());

		auto p = vars.get_params().get_param( name );	
		if ( p ) {
			/** subscribe to param */
			prot.unsubscribe_param ( p );
			apply();
		} else {
			fail( ErrorCodes::unknown_param );
		}

	}


protected:
	Vars& vars;
	SzbaseProt& prot;

};

#endif /* end of include guard: __SERVER_CMD_SET_UNSUBSCRIBE_H__ */

