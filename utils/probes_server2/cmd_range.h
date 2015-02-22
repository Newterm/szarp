#ifndef __SERVER_CMD_RANGE_H__
#define __SERVER_CMD_RANGE_H__

#include <boost/format.hpp>

#include "command_handler.h"
#include "handler.h"

using boost::format;

class RangeRcv : public CommandHandler {
public:
	RangeRcv( ClientHandler& client_handler )
		: hnd(client_handler)
	{
		set_next( std::bind(&RangeRcv::parse_command, this, p::_1) );
	}

	virtual ~RangeRcv()
	{
	}

        const char * get_tag() { return "RANGE"; };

	void parse_command( const std::string& line )
	{
                sz_log(10, "RangeRcv::parse_command: %s", line.c_str());

                time_t first, last;
                hnd.get_szbcache().range(&first, &last);

		apply( str( format("%lu %lu\r\n") % (unsigned long)last % (unsigned long)first ) );
	}

protected:
	ClientHandler& hnd;
	static const std::string tag;
};

#endif /* end of include guard: __SERVER_CMD_RANGE_H__ */


