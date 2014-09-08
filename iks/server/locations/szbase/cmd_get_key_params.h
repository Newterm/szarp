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

		ptree sets_tree;

		auto & sets = vars.get_sets();
		for( auto si = sets.begin(); si != sets.end(); ++si ) {
			std::string set_name = *si;
			auto set = sets.get_set(set_name);

			std::function<bool (std::string, std::string)> param_match =
				[&] (std::string name, std::string draw_name) {
					return contains(name, key) || contains(draw_name, key);
				};
			if( contains(set_name, key) ) {
				param_match = [&] (std::string, std::string) { return true; };
			}

			ptree set_tree;
			for( auto pi = set->begin(); pi != set->end(); ++pi ) {
				std::string param_name = *pi;
				std::string draw_name = vars.get_params().get_param( param_name )->get_draw_name();
				if( param_match(param_name, draw_name) ) {
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

	bool contains(std::string str, std::string key) const {
		using boost::algorithm::ifind_first;
		if( key == "" ) {
			return true;
		} else {
			std::locale locale(setlocale(LC_ALL, NULL));	// get system locale (by default std::locale uses C)
			return !ifind_first(str, key, locale).empty();
		}
	}
	Vars& vars;
};

#endif /* end of include guard: __SZBASE_CMD_GET_KEY_PARAMS_H__ */

