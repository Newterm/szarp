#include <string>

#include "probes_srv.h"
#include "thread_pool.h"


ProbesServer::ProbesServer( boost::asio::io_service& io_service, std::string& cache_dir, std::size_t psize )
	: m_szbcache(cache_dir), m_pool(io_service, psize)
{
}

ProbesServer::~ProbesServer()
{
}

void ProbesServer::on_new_connection( Connection* con )
{
	handlers.emplace( con , new ClientHandler( m_szbcache , con , m_pool ) );
}

void ProbesServer::on_disconnected( Connection* con )
{
	auto itr = handlers.find( con );
	if( itr == handlers.end() )
		return;

	delete itr->second;
	handlers.erase( itr );
}


void ProbesServer::run_task( boost::function< void() > task )
{
        m_pool.run_task(task);
}
