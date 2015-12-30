#ifndef __SERVER_CMD_NOTIFY_H__
#define __SERVER_CMD_NOTIFY_H__

#include <boost/format.hpp>

#include "data/param.h"
#include "data/probe_type.h"

#include "locations/command.h"

class NotifySnd : public Command {
public:
	NotifySnd( const std::string& p ) : p(p) {}
	
	virtual ~NotifySnd() {}

	virtual to_send send_str()
	{
		return to_send( p );
	}

	virtual bool single_shot()
	{	return true; }

protected:
	std::string p;
};

#endif /* end of include guard: __SERVER_CMD_NOTIFY_H__ */

