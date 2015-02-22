#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include <unordered_set>
#include <unordered_map>

#include <boost/asio.hpp>
#include <boost/thread.hpp>

class ThreadPool {
public:
        ThreadPool(boost::asio::io_service& io_service, std::size_t psize);

        /// @brief Adds a task to the thread pool if a thread is currently available.
        void run_task( boost::function< void() > task );

private:

        void wrap_task( boost::function< void() > task );

        boost::asio::io_service& m_io_service;
        boost::asio::io_service::work m_work;

        boost::thread_group m_threads;
        std::size_t m_available;
        boost::mutex m_mutex;

};


#endif /* end of include guard: __THREAD_POOL__ */
