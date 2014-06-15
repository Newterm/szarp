#ifndef __REMOTES_CMD_UPDATE_REMOTES_H__
#define __REMOTES_CMD_UPDATE_REMOTES_H__

#include <boost/format.hpp>

#include "locations/command.h"
#include "locations/locations_list.h"
#include "locations/proxy/proxy.h"

class CmdAddRemotesRcv : public Command {
public:
	CmdAddRemotesRcv(
		LocationsList& locs , const std::string& name ,
		const std::string& address , unsigned port )
		: locs(locs) , name(name) , address(address) , port(port)
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
		locs.register_location<ProxyLoc>
			( str( boost::format("%s:%s") % name % line ), address, port );
	}

protected:
	LocationsList& locs;
	const std::string& name;
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

