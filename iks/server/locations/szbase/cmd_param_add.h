#ifndef __SERVER_CMD_PARAM_ADD_H__
#define __SERVER_CMD_PARAM_ADD_H

#include "locations/command.h"

class ParamAddRcv : public Command {
public:
	ParamAddRcv( Vars& vars , SzbaseProt& prot )
		: vars(vars), prot(prot)
	{
		set_next( std::bind(&ParamAddRcv::parse_command,this,std::placeholders::_1) );
	}

	virtual ~ParamAddRcv()
	{
	}

	void parse_command( const std::string& line )
	{

		namespace bp = boost::property_tree;
		namespace balgo = boost::algorithm;

		auto r = find_quotation( line );
		std::string name(r.begin(),r.end());

		auto l = find_after_quotation( line );

		try {
			bp::ptree json;
			std::stringstream ss( std::string( l.begin() , l.end() ) );
			bp::json_parser::read_json( ss , json );

			std::string type = json.get<std::string>("type");
			if ( type != "va" && type != "av" ) 
			{
				fail( ErrorCodes::ill_formed );
				return;
			}
			
			std::string base = json.get<std::string>("base");
			std::string formula = json.get<std::string>("formula");
			int start_time = json.get<unsigned>("start_time");
			int prec = json.get<int>("prec");

			try{
				prot.add_param( name , base , formula , type , prec , start_time );
			} catch (szbase_error& e) {
				fail( ErrorCodes::ill_formed , e.what() );
			}

			apply( );

		} catch( const bp::ptree_error& e ) {
			fail( ErrorCodes::ill_formed );
			return;
		}

	}

protected:
	Vars& vars;
	SzbaseProt& prot;
};

#endif
