#ifndef __SZBASE_CMD_LIST_PARAMS_H__
#define __SZBASE_CMD_LIST_PARAMS_H__

#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "data/vars.h"

#include "utils/ptree.h"
#include "utils/tokens.h"

class ListParamsRcv : public Command {
public:
	ListParamsRcv( Vars& vars )
		: vars(vars)
	{
		set_next( std::bind(&ListParamsRcv::parse_command,this,std::placeholders::_1) );
	}

	virtual ~ListParamsRcv()
	{
	}

	void parse_command( const std::string& tag )
	{
		if( tag.empty() )
			send_params( vars.get_params() );
		else {
			std::string name;
			try {
				auto r = find_quotation(tag);
				name.assign(r.begin(),r.end());
			} catch( parse_error& e ) {
				fail( ErrorCodes::ill_formed );
				return;
			}

			auto ps = vars.get_sets().get_set(name);
			if( ps )
				send_params( *ps );
			else
				fail( ErrorCodes::unknown_set );
		}
	}

	template<class Container> void send_params( const Container& container )
	{
		using boost::property_tree::ptree;

		ptree out;
		ptree params;

		for( auto& p : container )
			params.push_back( std::make_pair( "" , ptree(p) ) );

		out.add_child( "params" , params );

		apply( ptree_to_json( out , false ) );
	}

protected:
	Vars& vars;
};

#endif /* end of include guard: __SZBASE_CMD_LIST_PARAMS_H__ */

