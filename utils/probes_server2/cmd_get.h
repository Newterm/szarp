#ifndef __SERVER_CMD_GET_H__
#define __SERVER_CMD_GET_H__

#include "command_handler.h"
#include "handler.h"

using boost::asio::ip::tcp;

class GetRcv : public CommandHandler {
public:
	GetRcv( ClientHandler& client_handler )
		: hnd(client_handler)
	{
		set_next( std::bind(&GetRcv::parse_command, this, p::_1) );
	}

	virtual ~GetRcv()
	{
	}

        const char * get_tag() { return "GET"; };

	void parse_command( const std::string& line )
	{
		sz_log(10, "GetRcv::parse_command %s", line.c_str());
                std::vector<std::string> details;
                boost::split(details, line, boost::is_any_of(" "), ba::token_compress_on);

		if (details.size() != 3) {
			fail(ErrorCodes::bad_request);
			return;
		}

		time_t start_time = boost::lexical_cast<time_t>(details[0]);
		time_t end_time = boost::lexical_cast<time_t>(details[1]);
		if (start_time > end_time) {
			sz_log(2, "GET: request bad start > end");
			fail(ErrorCodes::bad_request);
			return;
		}

		std::string& param_path = details[2];
		if (param_path.empty()) {
			sz_log(2, "GetRcv::parse_command empty param path");
			fail(ErrorCodes::bad_request);
			return;
		}

		sz_log(10, "GetRcv: s: %ld, e: %ld, p: %s",
				start_time, end_time, param_path.c_str());

		SzbCache& szc = hnd.get_szbcache();

		try {
			time_t first, last;
			szc.search_first_last(param_path, first, last);

			size_t to_send = szc.get_block_size(start_time, end_time);
			response( str( format("%ld %lu\n") % (unsigned long)last % to_send  ) );
			sz_log(10, "GetRcv after response");

			if (to_send == 0)
				return;

			int sock_fd = hnd.get_native_fd();
			szc.get(sock_fd, param_path, start_time, end_time);
			sz_log(10, "GetRcv::param_path after SzbCache::get");
		}
		catch (SzbCache::invalid_path_ex) {
			sz_log(2, "GetRcv invalid_path_ex");
			fail(ErrorCodes::bad_request);
			return;
		}
	}

protected:
	ClientHandler& hnd;
	static const std::string tag;
};

#endif /* end of include guard: __SERVER_CMD_GET_H__ */


