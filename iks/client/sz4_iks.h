#include <boost/system/error_code.hpp>

#include "sz4/defs.h"
#include "sz4/api.h"

#include "iks_cmd_id.h"
#include "sz4_connection_mgr.h"
#include "sz4_location_connection.h"
#include "sz4_iks_param_observer.h"
#include "sz4_iks_param_info.h"

namespace sz4 {

class connection_mgr;

typedef std::shared_ptr<param_observer_> param_observer_f;

class iks : public std::enable_shared_from_this<iks> {
	std::shared_ptr<boost::asio::io_service> m_io;
	std::shared_ptr<connection_mgr> m_connection_mgr;

	struct observer_reg {
		std::shared_ptr<connection_mgr> conn_mgr;
		std::string name;
		param_info param;
		std::vector<param_observer_f> observers;

		boost::signals2::scoped_connection error_sig_c;
		boost::signals2::scoped_connection cmd_sig_c;
		boost::signals2::scoped_connection connected_sig_c;

		observer_reg(std::shared_ptr<connection_mgr> conn_mgr, const std::string& name,
			param_info param);

		~observer_reg();

		void on_error(const boost::system::error_code& ec);
		void on_cmd(const std::string& tag, IksCmdId, const std::string &);
		void on_connected(std::wstring prefix);

		void connect();
	};

	std::map<param_info, std::shared_ptr<observer_reg>> m_observer_regs;

	connection_mgr::loc_connection_ptr connection_for_base(const std::wstring& prefix);

	template<class T> void search_data(param_info param, std::string dir, const T start, const T end, SZARP_PROBE_TYPE probe_type, std::function<void(const boost::system::error_code&, const T&)> cb);

	template<class V, class T> void _get_weighted_sum(param_info param, T start, T end, SZARP_PROBE_TYPE probe_type, std::function<void(const boost::system::error_code&, const std::vector< weighted_sum<V, T> >&) > cb);

	void _register_observer(param_observer_f observer, std::vector<param_info> params, std::function<void(const boost::system::error_code&) > cb);

	void _deregister_observer(param_observer_f observer, std::vector<param_info> params, std::function<void(const boost::system::error_code&) > cb);
	template<class T> void _get_first_time(param_info param, std::function<void(const boost::system::error_code&, T&) > cb);

	template<class T> void _get_last_time(param_info param, std::function<void(const boost::system::error_code&, T&) > cb);

	void _add_param(param_info param, std::function<void(const boost::system::error_code&)> cb);

	void _remove_param(param_info param, std::function<void(const boost::system::error_code&)> cb);
public:
	iks(std::shared_ptr<boost::asio::io_service> io, std::shared_ptr<connection_mgr> connection_mgr);

	template<class V, class T> void get_weighted_sum(const param_info& param, const T& start, const T& end, SZARP_PROBE_TYPE probe_type, std::function<void(const boost::system::error_code&, const std::vector< weighted_sum<V, T> >&) > cb);

	template<class T> void search_data_right(const param_info& param, const T& start, const T& end, SZARP_PROBE_TYPE probe_type, std::function<void(const boost::system::error_code&, const T&)> cb);

	template<class T> void search_data_left(const param_info& param, const T& start, const T& end, SZARP_PROBE_TYPE probe_type, std::function<void(const boost::system::error_code&, const T&)> cb);

	template<class T> void get_first_time(const param_info& param, std::function<void(const boost::system::error_code&, T&) > cb);

	template<class T> void get_last_time(const param_info& param, std::function<void(const boost::system::error_code&, T&) > cb);

	void register_observer(param_observer_f observer, const std::vector<param_info>& params, std::function<void(const boost::system::error_code&) > cb);

	void deregister_observer(param_observer_f observer, const std::vector<param_info>& params, std::function<void(const boost::system::error_code&) > cb);

	void add_param(const param_info& param, std::function<void(const boost::system::error_code&)> cb);

	void remove_param(const param_info& param, std::function<void(const boost::system::error_code&)> cb);

};

}

/* vim: set tabstop=8 softtabstop=8 shiftwidth=8 noexpandtab : */
