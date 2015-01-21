#ifndef __GLOBAL_SERVICE_H__
#define __GLOBAL_SERVICE_H__

#include <boost/asio.hpp>

/**
 * TODO: Expand this class in case of multi-threaded (17/03/2014 13:00, jkotur)
 */
class GlobalService {
public:
	static boost::asio::io_service& get_service()
	{	return service; }

protected:
	static boost::asio::io_service service;
};

#endif /* end of include guard: __GLOBAL_SERVICE_H__ */

