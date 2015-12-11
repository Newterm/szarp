#ifndef SZ4_CONNECTION_MGR_H
#define SZ4_CONNECTION_MGR_H

#include <unordered_map>
#include <boost/signals2.hpp>

#include "iks_cmd_id.h"

class TParam;
class IksConnection;

namespace boost { namespace asio {
class io_service;
}}

namespace sz4 {

class location_connection;

class connection_mgr : public std::enable_shared_from_this<connection_mgr> {
public:
	typedef std::shared_ptr<location_connection> loc_connection_ptr;
	typedef std::shared_ptr<IksConnection> connection_ptr;
private:
	class timer;

	typedef std::pair<std::wstring, std::string> remote_entry;

	IPKContainer* m_ipk_container;
	std::string m_address;
	std::string m_port;
	connection_ptr m_connection;
	std::vector<remote_entry> m_remotes;
	std::unordered_map<std::wstring, loc_connection_ptr> m_location_connections;
	std::shared_ptr<timer> m_reconnect_timer;
	std::shared_ptr<boost::asio::io_service> m_io;

	bool parse_remotes(const std::string &data, std::vector<remote_entry>& remotes);


	void connect();
	void connect_location(const std::wstring& name, const std::string& location);
	void disconnect_location(const std::wstring& name);

	void schedule_reconnect();

	void on_connected();
	void on_error(const boost::system::error_code& ec);
	void on_cmd(const std::string&, IksCmdId, const std::string&);

	void on_location_connected(std::wstring);
	void on_location_error(std::wstring, const boost::system::error_code& ec);

public:
	connection_mgr(IPKContainer* conatiner, const std::string& address, const std::string& port, std::shared_ptr<boost::asio::io_service> io);
	loc_connection_ptr connection_for_base(const std::wstring& prefix);

	void run();

	boost::signals2::signal<void()> connected_sig;
	boost::signals2::signal<void(const boost::system::error_code &)> connection_error_sig;

	boost::signals2::signal<void(std::wstring)> connected_location_sig;
	boost::signals2::signal<void(std::wstring)> disconnected_location_sig;
	boost::signals2::signal<void(std::wstring, const boost::system::error_code &)> error_location_sig;
};

}

#endif
/* vim: set tabstop=8 softtabstop=8 shiftwidth=8 noexpandtab : */
