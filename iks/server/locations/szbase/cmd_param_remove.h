#ifndef __SERVER_CMD_PARAM_REMOVE_H__
#define __SERVER_CMD_PARAM_REMOVE_H

#include "locations/command.h"

class ParamRemoveRcv : public Command {
public:
	ParamRemoveRcv( Vars& vars , SzbaseProt& prot )
		: vars(vars), prot(prot)
	{
		set_next( std::bind(&ParamRemoveRcv::parse_command,this,std::placeholders::_1) );
	}

	virtual ~ParamRemoveRcv()
	{
	}

	void parse_command( const std::string& line )
	{

		namespace bp = boost::property_tree;
		namespace balgo = boost::algorithm;

		auto r = find_quotation( line );
		std::string name(r.begin(),r.end());

		try{
			prot.remove_param( name );
			apply();
		} catch ( szbase_param_not_found_error& ) {
			fail( ErrorCodes::unknown_param );
		}
	}

protected:
	Vars& vars;
	SzbaseProt& prot;
};

#endif
