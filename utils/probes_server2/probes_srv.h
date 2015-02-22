#ifndef __PROBES_SRV__
#define __PROBES_SRV__

#include <unordered_set>
#include <unordered_map>

#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include "net/tcp_server.h"
#include "handler.h"
#include "thread_pool.h"


class ProbesServer {
public:
	ProbesServer( boost::asio::io_service& io_service, std::string& cache_dir, std::size_t psize );
	virtual ~ProbesServer();

	void on_new_connection( Connection* con );
	void on_disconnected  ( Connection* con );

        void run_task( boost::function< void() > task );
private:
	SzbCache m_szbcache;
        ThreadPool m_pool;

	std::unordered_map<Connection*, ClientHandler*> handlers;
};

#endif /* end of include guard: __PROBES_SRV__ */

