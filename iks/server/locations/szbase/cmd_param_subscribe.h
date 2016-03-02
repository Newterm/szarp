#ifndef __SERVER_CMD_PARAM_SUBSCRIBE_H__
#define __SERVER_CMD_PARAM_SUBSCRIBE_H__

#include <boost/algorithm/string.hpp>

#include "locations/command.h"

class ParamSubscribeRcv : public Command {
public:
	ParamSubscribeRcv( Vars& vars , SzbaseProt& prot )
		: vars(vars) , prot(prot)
	{
		set_next( std::bind(&ParamSubscribeRcv::parse_command,this,std::placeholders::_1) );
	}

	virtual ~ParamSubscribeRcv()
	{
	}

	void parse_command( const std::string& line )
	{
		namespace ba = boost::algorithm;

		auto r = find_quotation( line );
		std::string name(r.begin(),r.end());

		/** subscribe to param */
		prot.subscribe_param ( name );
		apply();

	}


protected:
	Vars& vars;
	SzbaseProt& prot;

};

#endif /* end of include guard: __SERVER_CMD_SET_SUBSCRIBE_H__ */

