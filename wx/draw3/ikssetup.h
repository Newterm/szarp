#ifndef __IKSSETUP_H__
#define __IKSSETUP_H__

std::tuple<
	std::shared_ptr<sz4::connection_mgr>,
	std::shared_ptr<sz4::iks>,
	std::shared_ptr<boost::asio::io_service>
> 
build_iks_client(IPKContainer *conatiner,
		const std::wstring& address, const std::wstring& port);

boost::thread start_connection_manager(std::shared_ptr<sz4::connection_mgr> c);
#endif
