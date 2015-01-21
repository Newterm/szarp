#ifndef __SERVER_CMD_SEARCH_H__
#define __SERVER_CMD_SEARCH_H__

#include <boost/format.hpp>

#include "command_handler.h"
#include "handler.h"

using boost::format;

class SearchRcv : public CommandHandler {
public:
	SearchRcv( ClientHandler& client_handler )
		: hnd(client_handler)
	{
		set_next( std::bind(&SearchRcv::parse_command, this, p::_1) );
	}

	virtual ~SearchRcv()
	{
	}

        const char * get_tag() { return "SEARCH"; };

	void parse_command( const std::string& line )
	{
                sz_log(10, "SearchRcv::parse_command start %s", line.c_str());
                std::vector<std::string> details;
                boost::split(details, line, boost::is_any_of(" "), ba::token_compress_on);

		if (details.size() != 4) {
			fail(ErrorCodes::bad_request);
		}

		time_t start_time = boost::lexical_cast<time_t>(details[0]);
		time_t end_time = boost::lexical_cast<time_t>(details[1]);
		int direction = boost::lexical_cast<int>(details[2]);
		std::string& param_path = details[3];

		try {
			time_t first, last;
			time_t found = hnd.get_szbcache().search(start_time, end_time, direction, param_path, first, last);

                        sz_log(10, "SearchRcv::parse_command before apply");
			apply( str( format("%ld %lu %lu\r\n") % (long)found % (unsigned long)first % (unsigned long)last ) );
		}
		catch (SzbCache::invalid_path_ex) {
			fail(ErrorCodes::bad_request);
		}
                sz_log(10, "SearchRcv::parse_command end %s", line.c_str());
	}

protected:
	ClientHandler& hnd;
	static const std::string tag;

};


#endif /* end of include guard: __SERVER_CMD_SEARCH_H__ */


