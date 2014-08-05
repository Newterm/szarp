#ifndef __WELCOME_CMD_LIST_REMOTES_H__
#define __WELCOME_CMD_LIST_REMOTES_H__

#include "locations/command.h"

#include "locations/locations_list.h"

#include "utils/ptree.h"

class CmdListRemotesRcv : public Command {
public:
	CmdListRemotesRcv( Protocol& prot , LocationsList& locs )
		: prot(prot) , locs(locs)
	{
		set_next( std::bind(&CmdListRemotesRcv::parse_command,this,std::placeholders::_1) );
	}

	virtual ~CmdListRemotesRcv()
	{
	}

	void parse_command( const std::string& line )
	{
		if( !line.empty() ) {
			fail( ErrorCodes::ill_formed );
			return;
		}

		namespace bp = boost::property_tree;

		bp::ptree out;

		for( auto itr=locs.begin() ; itr!=locs.end() ; ++itr )
		{
			out.put( *itr + ".name" , itr.get_name() );
			out.put( *itr + ".type" , itr.get_type() );
		}

		apply( ptree_to_json( out , false ) );
	}

protected:
	Protocol& prot;
	LocationsList& locs;

};

#endif /* end of include guard: __WELCOME_CMD_LIST_REMOTES_H__ */

