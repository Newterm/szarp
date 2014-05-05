#ifndef __SZBASE_CMD_LIST_SETS_H__
#define __SZBASE_CMD_LIST_SETS_H__

#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "data/vars.h"

#include "utils/ptree.h"

class ListSetsRcv : public Command {
public:
	ListSetsRcv( Vars& vars )
		: vars(vars)
	{
		set_next( std::bind(&ListSetsRcv::parse_command,this,std::placeholders::_1) );
	}

	virtual ~ListSetsRcv()
	{
	}

	void parse_command( const std::string& tag )
	{
		if( tag.empty() )
			send_sets( vars.get_sets() );
		else
			fail( ErrorCodes::ill_formed );
	}

	template<class Container> void send_sets( const Container& container )
	{
		namespace bp = boost::property_tree;

		bp::ptree out;
		bp::ptree params;

		for( auto& p : container )
			params.push_back( std::make_pair( "" , bp::ptree(p) ) );

		out.add_child( "sets" , params );

		apply( ptree_to_json( out , false ) );
	}

protected:
	Vars& vars;
};

#endif /* end of include guard: __SZBASE_CMD_LIST_SETS_H__ */


