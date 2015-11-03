#include <boost/fusion.hpp>

namespace sz4 {

const auto value_type_2_name = boost::fusion::make_map<short , int , float , double>("short", "int", "float", "double");
const auto time_type_2_name  = boost::fusion::make_map<second_time_t, nanosecond_time_t>("second_t", "nanosecond_t");

template<class T> void iks::search_data(TParam* param, const std::string dir, const T& start, const T& end, SZARP_PROBE_TYPE probe_type, const search_condition& condition, std::function<void(error&, T&) > cb) {
	auto client = client_for_param(param);
	if (!client) {
		result(error::not_configured, T());
		return;
	}

	std::ostringstream ss;
	ss << "\"" << SC::S2U(param->GetName()) << "\" "
		<< ProbeType(probe_type).to_string() << " "
		<< boost::fusion::at_key<T>(time_type_2_name) << " "
		<< start << " " << end;

	client->send_command("search_data", ss.str(), [cb] (IksClient::error, const std::string& status, std::string& data) {
		if (error) {
			cb(error::connection_errror, T());
			return IksClient::cmd_done;
		}

		if (status != "k") {
			cb(error::response_error, T());
			return IksClient::cmd_done;
		}

		T result;
		if (std::istringstream(data) >> result)
			cb(error::no_error, result);
		else
			cb(error::response_error, result);

		return IksClient::cmd_done;
	});
}

template<class V, class T> void iks::get_weighted_sum(TParam* param ,const T& start ,const T& end, SZARP_PROBE_TYPE probe_type, std::function< void( error& , const std::vector< weighted_sum<V , T> >& ) > cb) {
	typedef response_t std::vector< weighted_sum<V , T> >;

	IksClient* client = client_for_param(param);
	if (!client) {
		result(error::not_configured, response_type());
		return;
	}

	std::ostringstream ss;
	ss << "\"" << SC::S2U(param->GetName()) << "\" "
		<< boost::fusion::at_key<V>(value_type_2_name) << " "
		<< boost::fusion::at_key<T>(time_type_2_name) << " "
		<< ProbeType(probe_type).to_string() << " "
		<< start << " " << end;
	  
	client->send_command("get_data", ss.str(), [cb] (IksClient::error, const std::string& status, std::string& data) {
		if (error) {
			cb(error::connection_errror, response_t());
			return IksClient::cmd_done;
		}

		if (status != "k") {
			cb(error::response_error, response_type());
			return IksClient::cmd_done;
		}

		std::istringstream ss(data);

		class _wsum : public weighted_sum<V , T> {
		public:
			sum_type& _wsum() { return m_wsum; }
			time_diff_type& _weight { return m_weight; }
			time_diff_type& _no_data_weight { return m_no_data_weight; }
			bool& _fixed { return _fixed; }
		};

		_wsum wsum;

		response_t response;
		while (ss >> wsum._sum() >> wsum._weight() >> wsum._no_data_weight() >> wsum.fixed())
			response.push_back(wsum);

		if (ss.eof())
			cb(error::ok, response);
		else
			cb(error::response_error, response_type());

		return IksClient::cmd_done;
	});
	
}

template<class T> void iks::search_data_right(TParam* param, const T& start, const T& end, SZARP_PROBE_TYPE probe_type, const search_condition& condition, std::function<void(error&, T&) > cb) {
	search_data(param, "right", start, end, probe_type, condition, cb);
}

template<class T> void iks::search_data_left(TParam* param, const T& start, const T& end, SZARP_PROBE_TYPE probe_type, const search_condition& condition, std::function<void(error&, T&) > cb) {
	search_data(param, "left", start, end, probe_type, condition, cb);
}

template<class T> void iks::get_first_time(TParam* param, std::function<void(error&, T&) > result) {
//TODO
}

template<class T> void iks::get_last_time(TParam* param, std::function<void(error&, T&) > result) {
//TODO
}

}
