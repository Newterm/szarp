#ifndef SZ4_LOCATION_CONN_H
#define SZ4_LOCATION_CONN_H

#include <boost/variant.hpp>
#include <tuple>

class IPKContainer;
class IksConnection;

namespace sz4 {

class location_connection : public std::enable_shared_from_this<location_connection> {
	IPKContainer* m_container;
	std::string m_location;
	std::shared_ptr<IksConnection> m_connection;
	std::string m_defined_param_prefix;
	bool m_connected;

	typedef boost::variant<
			std::tuple<std::string, std::string, IksCmdCallback>,
			std::tuple<IksCmdId, std::string, std::string>> cached_entry;

	std::vector<cached_entry> m_cmd_cache;

	void add_param(const std::string& base, TParam* param, IksCmdCallback callback);

	void add_defined_params();

	void send_cached();

	void connect_to_location();
public:
	location_connection(IPKContainer* ipk, boost::asio::io_service& io,
			const std::string& location, const std::string& server,
			const std::string& port, const std::string& defined_param_prefix);

	void add_param(const param_info& param, IksCmdCallback callback);
	void remove_param(const param_info& param, IksCmdCallback callback);

	void send_command(const std::string& cmd, const std::string& data, IksCmdCallback callback);
	void send_command(IksCmdId id, const std::string& cmd , const std::string& data );

	void on_connected();
	void on_connection_error(const boost::system::error_code& ec);
	void on_cmd(const std::string& status, IksCmdId id, const std::string& data);

	void start_connecting();

	void disconnect();

	boost::signals2::signal<void()>						connected_sig;
	boost::signals2::signal<void(const boost::system::error_code &)>	connection_error_sig;
	boost::signals2::signal<void(const std::string&,
					IksCmdId,
					const std::string&)>			cmd_sig;
};

}
#endif
/* vim: set tabstop=8 softtabstop=8 shiftwidth=8 noexpandtab : */
