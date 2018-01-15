#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include "sz4_iks_param_info.h"
#include "sz4_iks.h"
#include "conversion.h"
#include "sz4_connection_mgr.h"
#include "ikssetup.h"

std::tuple<
	std::shared_ptr<sz4::connection_mgr>,
	std::shared_ptr<sz4::iks>,
	std::shared_ptr<boost::asio::io_service>
> 
build_iks_client(IPKContainer *container,
		 const std::wstring& address,
		 const std::wstring& port,
		 const std::wstring& user_prefix) {
	auto io = std::make_shared<boost::asio::io_service>();

	auto conn_mgr = std::make_shared<sz4::connection_mgr>(
				container, 
				(const char*)SC::S2U(address).c_str(),
				(const char*)SC::S2U(port).c_str(),
				(const char*)SC::S2U(user_prefix).c_str(),
				io);

	return std::make_tuple(
		conn_mgr,
		std::make_shared<sz4::iks>(io, conn_mgr),
		io);
}

boost::thread start_connection_manager(std::shared_ptr<sz4::connection_mgr> c) {
	return boost::thread([c]() {
#ifndef MINGW32
		uselocale(newlocale(LC_ALL_MASK, "C", 0));
#endif
		c->run();
	});
}
