
#include "thread_pool.h"


ThreadPool::ThreadPool(boost::asio::io_service& io_service, std::size_t psize)
                : m_io_service(io_service), m_work(io_service), m_available(psize)
{
        for ( std::size_t i = 0; i < m_available; ++i )
        {
                m_threads.create_thread(
                                boost::bind( &boost::asio::io_service::run,
                                        &m_io_service ) );
        }
}

void ThreadPool::run_task( boost::function< void() > task )
{
        boost::unique_lock< boost::mutex > lock( m_mutex );

        // Post a wrapped task into the queue.
        m_io_service.post( boost::bind( &ThreadPool::wrap_task, this,
                                boost::function< void() >( task ) ) );
}


void ThreadPool::wrap_task( boost::function< void() > task )
{
        // Run the user supplied task.
        try
        {
                // std::cerr << "ThreadPool::wrap_task " << boost::this_thread::get_id() << std::endl;
                task();
        }
        // Suppress all exceptions.
        catch ( ... ) {}

}



