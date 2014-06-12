#ifndef __REMOTES_CMD_LIST_REMOTES_H__
#define __REMOTES_CMD_LIST_REMOTES_H__

#include <sstream>

#include "locations/command.h"
#include "locations/locations_list.h"
#include "locations/proxy/proxy.h"

#include "utils/ptree.h"

class CmdListRemotesSnd : public Command {
public:
	CmdListRemotesSnd( 
		LocationsList& locs , const std::string& name ,
		const std::string& address , unsigned port )
		: locs(locs) , name(name) , address(address) , port(port)
	{
		set_next( std::bind(
				&CmdListRemotesSnd::parse_command ,
				this,std::placeholders::_1) );
	}

	virtual ~CmdListRemotesSnd() {}

	virtual to_send send_str()
	{	return to_send( "" ); }

	void parse_command( const std::string& line )
	{
		namespace bp = boost::property_tree;

		std::stringstream ss(line);
		bp::ptree json;

		try {
			bp::json_parser::read_json( ss , json );
		} catch( const bp::ptree_error& e ) {
			/* TODO: Log error (19/05/2014 21:48, jkotur) */
			return;
		}

		try {
			for( auto itr=json.begin() ; itr!=json.end() ; ++itr )
				locs.register_location<ProxyLoc>
					( str( boost::format("%s:%s") % name % itr->first ) ,
					  itr->second.get<std::string>("name") ,
					  itr->second.get<std::string>("type") ,
					  address, port );

		} catch( const bp::ptree_error& e ) {
			/* TODO: Log errors (12/06/2014 16:14, jkotur) */
			return;
		}
	}

protected:
	LocationsList& locs;
	const std::string& name;
	const std::string& address;
	unsigned port;

};

#endif /* end of include guard: __REMOTES_CMD_LIST_REMOTES_H__ */

