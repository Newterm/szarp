#ifndef __SERVER_CMD_GET_CONFIG_H__
#define __SERVER_CMD_GET_CONFIG_H__

#include "locations/command.h"

#include "utils/ptree.h"

class GetConfigRcv : public Command {
public:
	GetConfigRcv( Vars& vars )
		: vars(vars)
	{
		set_next( std::bind(&GetConfigRcv::parse_command,this,std::placeholders::_1) );
	}

	virtual ~GetConfigRcv()
	{
	}

	void parse_command( const std::string& line )
	{
		namespace bp = boost::property_tree;

		bp::ptree opts;

		/** Send all options */
		if( line.empty() ) {
			for( auto& o : vars.get_config() )
				/** prevent '.' from beeing path separator */
				opts.put( boost::property_tree::path(o.first,'\0') , o.second );

			apply( ptree_to_json( opts ) );
			return;
		}

		std::stringstream ss(line);
		bp::ptree json;

		try {
			bp::json_parser::read_json( ss , json );

			auto& cfg = vars.get_config();
			for( auto& p : json.get_child("options") )
			{
				auto opt = p.second.data();
				if( !cfg.has( opt ) ) {
					fail( ErrorCodes::unknown_option );
					return;
				}

				opts.put(
						boost::property_tree::path(opt,'\0') ,
						cfg.get(opt,"") );
			}

		} catch( const bp::ptree_error& e ) {
			fail( ErrorCodes::ill_formed );
			return;
		}

		apply( ptree_to_json( opts ) );
	}

protected:
	Vars& vars;
};

class ConfigUpdateSnd : public Command {
public:
	virtual ~ConfigUpdateSnd()
	{}

	virtual to_send send_str()
	{	return to_send( "" ); }

	virtual bool single_shot()
	{	return true; }

};

#endif /* end of include guard: __SERVER_CMD_GET_CONFIG_H__ */


