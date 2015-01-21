#ifndef __SERVER_HANDLER_H__
#define __SERVER_HANDLER_H__

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include <boost/asio.hpp>

#include "net/tcp_server.h"

#include "error_codes.h"

#include "command_handler.h"
#include "szbcache.h"
#include "probes_srv.h"
#include "thread_pool.h"

class ClientHandler : public std::enable_shared_from_this<ClientHandler> {
public:
	ClientHandler( SzbCache& szbcache , Connection* connection , ThreadPool& tpool );
	virtual ~ClientHandler();

	bool is_client_local() const;

        SzbCache& get_szbcache();
        int get_native_fd();

private:
	void run_cmd( CommandHandler* cmd );
	void erase_cmd( CommandHandler* cmd );
	void erase_cmd( id_t id );

	id_t generate_id();

	template<class Cmd,class... Args> void send_cmd( const Args&... args );

	void send_response(
		CommandHandler::ResponseType type ,
		const std::string& data ,
		CommandHandler* cmd );

	void send_fail( ErrorCodes code );

	void parse_line( const std::string& line );

	SzbCache& m_szbcache;
	Connection* connection;
	ThreadPool& thread_pool;

	std::unordered_map<id_t, CommandHandler*> commands;

	boost::signals2::scoped_connection conn_line;

	std::default_random_engine rnd;

};

#endif /* end of include guard: __SERVER_HANDLER_H__ */

