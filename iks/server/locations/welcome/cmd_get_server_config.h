#ifndef __SERVER_CMD_GET_SERVER_CONFIG_H__
#define __SERVER_CMD_GET_SERVER_CONFIG_H__

#include "locations/command.h"
#include "locations/config_container.h"

#include "utils/ptree.h"

class GetServerConfigRcv : public Command {
public:
	GetServerConfigRcv( Protocol& prot , LocationsList& locs )
		: prot(prot) , locs(locs)
	{
		(void)prot;
		set_next( std::bind(&GetServerConfigRcv::parse_command,this,std::placeholders::_1) );
	}

	virtual ~GetServerConfigRcv()
	{
	}

	void parse_command( const std::string& line )
	{
		namespace bp = boost::property_tree;

		bp::ptree opts;

		/** Send all options */
		Config& config = dynamic_cast<ConfigContainer*>(&prot)->get_config();

		for( auto io=config.begin() ; io!=config.end() ; ++io )
			/** prevent '.' from being path separator */
			opts.put( boost::property_tree::path(io->first,'\0') , io->second );

		apply( ptree_to_json( opts ) );
		return;
	}

protected:
	Protocol& prot;
	LocationsList& locs;
};

#endif /* end of include guard: __SERVER_CMD_GET_SERVER_CONFIG_H__ */


