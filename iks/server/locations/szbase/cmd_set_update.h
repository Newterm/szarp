#ifndef __SERVER_CMD_SET_UPDATE_H__
#define __SERVER_CMD_SET_UPDATE_H__

#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "locations/command.h"

#include "utils/tokens.h"

class GetSetRcv : public Command {
public:
	GetSetRcv( Vars& vars )
		: vars(vars)
	{
		set_next( std::bind(&GetSetRcv::parse_command,this,std::placeholders::_1) );
	}

	virtual ~GetSetRcv()
	{
	}

	void parse_command( const std::string& line )
	{
		std::string name;

		try {
			auto r = find_quotation( line );
			name.assign(r.begin(),r.end());
		} catch( parse_error& e ) {
			fail( ErrorCodes::ill_formed );
			return;
		}

		auto s = vars.get_sets().get_set( name );

		if( s )
			apply( s->to_json(false) );
		else
			fail( ErrorCodes::unknown_set );
	}

protected:
	Vars& vars;
};

class SetUpdateRcv : public Command {
public:
	SetUpdateRcv( Vars& vars )
	{
		(void)vars;
		/* TODO: Szarp doesn't allow to change sets yet (05/05/2014 14:22, jkotur) */
		set_next( std::bind(&SetUpdateRcv::fail,this,ErrorCodes::set_read_only) );
	}

	virtual ~SetUpdateRcv()
	{
	}
};

class SetUpdateSnd : public Command {
public:
	SetUpdateSnd( const std::string& name , std::shared_ptr<const Set> s )
		: name(name) , s(s)
	{
	}

	virtual ~SetUpdateSnd()
	{
	}

	virtual to_send send_str()
	{
		auto json = s ? s->to_json() : "";
		return to_send( str( boost::format("\"%s\" %s") % name % json ) );
	}

	virtual bool single_shot()
	{	return true; }

protected:
	std::string name;
	std::shared_ptr<const Set> s;
};

#endif /* end of include guard: __SERVER_CMD_SET_UPDATE_H__ */

