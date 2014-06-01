#ifndef __WELCOME_CMD_HELLO_H__
#define __WELCOME_CMD_HELLO_H__

#include "locations/command.h"

class CmdHelloRcv : public Command {
public:
	CmdHelloRcv()
	{
		set_next( std::bind(&CmdHelloRcv::parse_command,this,std::placeholders::_1) );
	}

	virtual ~CmdHelloRcv()
	{
	}

	void parse_command( const std::string& line )
	{
		apply( "ehllo (" + line + ")" );
	}

};

#endif /* end of include guard: __WELCOME_CMD_HELLO_H__ */

