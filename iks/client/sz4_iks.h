#include <boost/system/error_code.hpp>

#include "sz4/defs.h"
#include "sz4/api.h"

#include "iks_cmd_id.h"
#include "sz4_connection_mgr.h"
#include "sz4_location_connection.h"

class TParam;

namespace sz4 {

class param_observer;
class connection_mgr;

class iks : public std::enable_shared_from_this<iks> {
	std::shared_ptr<boost::asio::io_service> m_io;
	std::shared_ptr<connection_mgr> m_connection_mgr;

	struct observer_reg {
		connection_mgr::loc_connection_ptr connection;
		std::string name;
		TParam* param;
		param_observer* observer;

		boost::signals2::scoped_connection error_sig_c;
		boost::signals2::scoped_connection cmd_sig_c;

		observer_reg(connection_mgr::loc_connection_ptr connection, const std::string& name,
			TParam* param, param_observer* observer);

		void on_error(const boost::system::error_code& ec);
		void on_cmd(const std::string& tag, IksCmdId, const std::string &);
	};

	std::list<observer_reg> m_observer_regs;

	connection_mgr::loc_connection_ptr connection_for_param(TParam *p);

	template<class T> void search_data(TParam* param, std::string dir, const T start, const T end, SZARP_PROBE_TYPE probe_type, std::function<void(const boost::system::error_code&, const T&)> cb);

	template<class V, class T> void _get_weighted_sum(TParam* param, T start, T end, SZARP_PROBE_TYPE probe_type, std::function<void(const boost::system::error_code&, const std::vector< weighted_sum<V, T> >&) > cb);

	void _register_observer(param_observer *observer, std::vector<TParam*> params, std::function<void(const boost::system::error_code&) > cb);

	void _deregister_observer(param_observer *observer, std::vector<TParam*> params, std::function<void(const boost::system::error_code&) > cb);
	template<class T> void _get_first_time(TParam* param, std::function<void(const boost::system::error_code&, T&) > cb);

	template<class T> void _get_last_time(TParam* param, std::function<void(const boost::system::error_code&, T&) > cb);

public:
	iks(std::shared_ptr<boost::asio::io_service> io, std::shared_ptr<connection_mgr> connection_mgr);

	template<class V, class T> void get_weighted_sum(TParam* param, const T& start, const T& end, SZARP_PROBE_TYPE probe_type, std::function<void(const boost::system::error_code&, const std::vector< weighted_sum<V, T> >&) > cb);

	template<class T> void search_data_right(TParam* param, const T& start, const T& end, SZARP_PROBE_TYPE probe_type, std::function<void(const boost::system::error_code&, const T&)> cb);

	template<class T> void search_data_left(TParam* param, const T& start, const T& end, SZARP_PROBE_TYPE probe_type, std::function<void(const boost::system::error_code&, const T&)> cb);

	template<class T> void get_first_time(TParam* param, std::function<void(const boost::system::error_code&, T&) > cb);

	template<class T> void get_last_time(TParam* param, std::function<void(const boost::system::error_code&, T&) > cb);

	void register_observer(param_observer *observer, const std::vector<TParam*>& params, std::function<void(const boost::system::error_code&) > cb);

	void deregister_observer(param_observer *observer, const std::vector<TParam*>& params, std::function<void(const boost::system::error_code&) > cb);

};

}

/* vim: set tabstop=8 softtabstop=8 shiftwidth=8 noexpandtab : */
