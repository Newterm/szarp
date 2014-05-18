#ifndef __SZBASE_CMD_SET_H__
#define __SZBASE_CMD_SET_H__

#include <functional>

#include "locations/command.h"
#include "data/vars.h"

class SetRcv : public Command {
public:
	SetRcv( Vars& vars )
	{
		(void)vars;

		/* FIXME: Allways fail since szarp doesn't support value setting yet (05/05/2014 12:52, jkotur) */
		set_next( std::bind(&SetRcv::fail,this,ErrorCodes::cannot_set_value,"Not implemented") );
	}

	virtual ~SetRcv()
	{
	}
};

#endif /* end of include guard: __SZBASE_CMD_SET_H__ */

