#ifndef __REMOTES_CMD_UPDATE_REMOTES_H__
#define __REMOTES_CMD_UPDATE_REMOTES_H__

#include <boost/format.hpp>

#include "locations/command.h"
#include "locations/locations_list.h"
#include "locations/proxy/proxy.h"

class CmdAddRemotesRcv : public Command {
public:
	CmdAddRemotesRcv(
		LocationsList& locs ,
		const std::string& name , const std::string& draw_name ,
		const std::string& address , unsigned port )
		: locs(locs) , name(name) , draw_name(draw_name) , address(address) , port(port)
	{
		set_next( std::bind(
				&CmdAddRemotesRcv::parse_command ,
				this,std::placeholders::_1) );
	}

	virtual ~CmdAddRemotesRcv()
	{
	}

	virtual bool single_shot() { return true; }

	void parse_command( const std::string& line )
	{
		namespace bp = boost::property_tree;
		bp::ptree json;

		try {
			std::stringstream ss(line);
			bp::json_parser::read_json( ss , json );

			locs.register_location<ProxyLoc>(
					str( boost::format("%s:%s") %
						name %
						json.get<std::string>("tag") ) ,
					str( boost::format("%s - %s") %
						draw_name %
						json.get<std::string>("name") ) ,
					json.get<std::string>("type") ,
					address , port );

		} catch( const bp::ptree_error& e ) {
			sz_log(0,"CmdAddRemotesRcv: Received invalid message (%s): %s", line.c_str(), e.what());
			return;
		}
	}

protected:
	LocationsList& locs;
	const std::string& name;
	const std::string& draw_name;
	const std::string& address;
	unsigned port;

};

class CmdRemoveRemotesRcv : public Command {
public:
	CmdRemoveRemotesRcv( LocationsList& locs , const std::string& name )
		: locs(locs) , name(name)
	{
		set_next( std::bind(
			&CmdRemoveRemotesRcv::parse_command ,
			this,std::placeholders::_1) );
	}

	virtual ~CmdRemoveRemotesRcv()
	{
	}

	virtual bool single_shot() { return true; }

	void parse_command( const std::string& line )
	{
		locs.remove_location(str( boost::format("%s:%s") % name % line ));
	}

protected:
	LocationsList& locs;
	const std::string& name;

};

#endif /* end of include guard: __REMOTES_CMD_UPDATE_REMOTES_H__ */

