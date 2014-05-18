#ifndef __SERVER_CMD_VALUE_H__
#define __SERVER_CMD_VALUE_H__

#include <boost/format.hpp>

#include "locations/command.h"

class ValueSnd : public Command {
public:
	ValueSnd( std::shared_ptr<const Param> p ) : p(p) {}
	
	virtual ~ValueSnd() {}

	virtual to_send send_str()
	{
		return to_send( str(
				boost::format("\"%s\" %d") % p->get_name() % p->get_value() ) );
	}

	virtual bool single_shot()
	{	return true; }

protected:
	std::shared_ptr<const Param> p;
};

#endif /* end of include guard: __SERVER_CMD_VALUE_H__ */

