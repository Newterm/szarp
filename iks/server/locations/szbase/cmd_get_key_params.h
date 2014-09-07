#ifndef __SZBASE_CMD_GET_KEY_PARAMS_H__
#define __SZBASE_CMD_GET_KEY_PARAMS_H__

#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "data/vars.h"

#include "utils/ptree.h"
#include "utils/tokens.h"

class GetParamsForKeyRcv : public Command {
public:
	GetParamsForKeyRcv( Vars& vars , Protocol& prot )
		: vars(vars)
	{
		(void)prot;
		set_next( std::bind(&GetParamsForKeyRcv::parse_command,this,std::placeholders::_1) );
	}

	virtual ~GetParamsForKeyRcv()
	{
	}

	void parse_command( const std::string& tag )
	{
		std::string key;
		try {
			auto r = find_quotation(tag);
			key.assign(r.begin(),r.end());
		} catch( parse_error& e ) {
			fail( ErrorCodes::ill_formed );
			return;
		}

		using boost::property_tree::ptree;
		using boost::algorithm::ifind_first;

		ptree sets_tree;

		auto & sets = vars.get_sets();
		std::locale locale(setlocale(LC_ALL, NULL));	// get system locale (by default std::locale uses C)
		for( auto si = sets.begin(); si != sets.end(); ++si ) {
			std::string set_name = *si;
			auto set = sets.get_set(set_name);
			ptree set_tree;
			if( ifind_first(set_name, key, locale).empty() ) {
				// add only matching params
				for( auto pi = set->begin(); pi != set->end(); ++pi ) {
					std::string param_name = *pi;
					std::string draw_name = vars.get_params().get_param( param_name )->get_draw_name();
					if( !ifind_first(param_name, key, locale).empty()
							|| !ifind_first(draw_name, key, locale).empty() ) {
						set_tree.push_back( std::make_pair( "" , ptree(draw_name) ) );
					}
				}
			} else {
				// add whole set
				for( auto pi = set->begin(); pi != set->end(); ++pi ) {
					std::string param_name = *pi;
					std::string draw_name = vars.get_params().get_param( param_name )->get_draw_name();
					set_tree.push_back( std::make_pair( "" , ptree(draw_name) ) );
				}
			}
			if( !set_tree.empty() ) {
				sets_tree.add_child( set_name, set_tree );
			}
		}

		ptree out;
		out.add_child( "sets" , sets_tree );

		apply( ptree_to_json( out , false ) );
	}

protected:
	Vars& vars;
};

#endif /* end of include guard: __SZBASE_CMD_GET_KEY_PARAMS_H__ */

