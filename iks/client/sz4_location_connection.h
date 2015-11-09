#ifndef SZ4_LOCATION_CONN_H
#define SZ4_LOCATION_CONN_H

#include <boost/variant.hpp>
#include <tuple>

#include "iks_connection.h"

class IPKContainer;

namespace sz4 {

class location_connection : public std::enable_shared_from_this<location_connection> {
	IPKContainer* m_container;
	std::string m_location;
	std::shared_ptr<IksConnection> m_connection;
	bool m_connected;

	typedef boost::variant<
			std::tuple<std::string, std::string, IksConnection::CmdCallback>,
			std::tuple<std::string, IksConnection::CmdId, IksConnection::CmdCallback>> cached_entry;

	void connect();
public:
	location_connection(IPKContainer* ipk, boost::asio::io_service& io,
			const std::string& location, const std::string& server,
			const std::string& port);

	void send_command(const std::string& cmd, const std::string& data, IksConnection::CmdCallback callback);
	void send_command(IksConnection::CmdId id, const std::string& cmd , const std::string& data );

	void on_connected();
	void on_connection_error(const boost::system::error_code& ec);
	void on_cmd(const std::string& status, IksConnection::CmdId id, const std::string& data);

	void disconnect();

	boost::signals2::signal<void()>					connected_sig;
	boost::signals2::signal<void(boost::system::error_code)>	connection_error_sig;
	boost::signals2::signal<void(const std::string&,
					IksConnection::CmdId,
					const std::string&)>		cmd_sig;
};

}
#endif
/* vim: set tabstop=8 softtabstop=8 shiftwidth=8 noexpandtab : */
