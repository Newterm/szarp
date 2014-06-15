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
		bp::ptree remotes;

		for( auto itr=locs.begin() ; itr!=locs.end() ; ++itr )
			remotes.push_back( std::make_pair( "" , bp::ptree(*itr) ) );

		out.add_child( "remotes" , remotes );

		apply( ptree_to_json( out , false ) );
	}

protected:
	Protocol& prot;
	LocationsList& locs;

};

#endif /* end of include guard: __WELCOME_CMD_LIST_REMOTES_H__ */

