#include <boost/fusion/include/make_map.hpp>
#include <boost/fusion/sequence/intrinsic/at_key.hpp>

#include "conversion.h"
#include "data/probe_type.h"
#include "sz4/defs.h"

namespace sz4 {

const auto value_type_2_name = boost::fusion::make_map<short , int , float , double>("short", "int", "float", "double");
const auto time_type_2_name  = boost::fusion::make_map<second_time_t, nanosecond_time_t>("second_t", "nanosecond_t");

template<class T> void iks::search_data(TParam* param, std::string dir, T start, T end, SZARP_PROBE_TYPE probe_type, std::function<void(const error&, const T&) > cb) {
	auto client = connection_for_param(param);
	if (!client) {
		cb(error::not_configured, T());
		return;
	}

	std::ostringstream ss;
	ss << "\"" << SC::S2U(param->GetName()) << "\" "
		<< ProbeType(probe_type).to_string() << " "
		<< dir << " "
		<< boost::fusion::at_key<T>(time_type_2_name) << " "
		<< start << " " << end;

	client->send_command("search_data", ss.str(), [cb] (IksError _error, const std::string& status, std::string& data) {
		if (_error) {
			cb(error::connection_error, T());
			return IksCmdStatus::cmd_done;
		}

		if (status != "k") {
			cb(error::invalid_response, T());
			return IksCmdStatus::cmd_done;
		}

		T result;
		if (std::istringstream(data) >> result)
			cb(error::no_error, result);
		else
			cb(error::invalid_response, result);

		return IksCmdStatus::cmd_done;
	});
}

template<class V, class T> void iks::_get_weighted_sum(TParam* param, T start, T end, SZARP_PROBE_TYPE probe_type, std::function<void(const error&, const std::vector< weighted_sum<V, T> >&) > cb) {
	typedef std::vector< weighted_sum<V , T> > result_t;

	auto client = connection_for_param(param);
	if (!client) {
		cb(error::not_configured, result_t());
		return;
	}

	std::ostringstream ss;
	ss << "\"" << SC::S2U(param->GetName()) << "\" "
		<< ProbeType(probe_type).to_string() << " "
		<< boost::fusion::at_key<V>(value_type_2_name) << " "
		<< boost::fusion::at_key<T>(time_type_2_name) << " "
		<< start << " " << end;
	  
	client->send_command("get_data", ss.str(), [cb] (IksError _error, const std::string& status, std::string& data) {
		if (_error) {
			cb(error::connection_error, result_t());
			return IksCmdStatus::cmd_done;
		}

		if (status != "k") {
			cb(error::invalid_response, result_t());
			return IksCmdStatus::cmd_done;
		}

		class _wsum : public weighted_sum<V , T> {
		public:
			typedef weighted_sum<V , T> parent_type;
			typename parent_type::sum_type& _sum() { return this->m_wsum; }
			typename parent_type::time_diff_type& _weight() { return this->m_weight; }
			typename parent_type::time_diff_type& _no_data_weight() { return this->m_no_data_weight; }
			bool& _fixed() { return this->m_fixed; }
		};

		std::istringstream ss(data);
		_wsum wsum;
		result_t response;

		while (ss >> wsum._sum() >> wsum._weight() >> wsum._no_data_weight() >> wsum._fixed())
			response.push_back(wsum);

		if (ss.eof())
			cb(error::no_error, response);
		else
			cb(error::invalid_response, result_t());

		return IksCmdStatus::cmd_done;
	});
	
}

template<class V, class T> void iks::get_weighted_sum(TParam* param ,const T& start ,const T& end, SZARP_PROBE_TYPE probe_type, std::function< void( const error& , const std::vector< weighted_sum<V , T> >& ) > cb) {
	m_io->post(std::bind(&iks::_get_weighted_sum<V,T>, shared_from_this(), param, start, end, probe_type, cb));
}

template<class T> void iks::search_data_right(TParam* param, const T& start, const T& end, SZARP_PROBE_TYPE probe_type, std::function<void(const error&, const T&) > cb) {
	m_io->post(
		std::bind(&iks::search_data<T>, shared_from_this(), param, "right", start, end, probe_type, cb));
}

template<class T> void iks::search_data_left(TParam* param, const T& start, const T& end, SZARP_PROBE_TYPE probe_type, std::function<void(const error&, const T&) > cb) {
	m_io->post(std::bind(&iks::search_data<T>, shared_from_this(), param, "left", start, end, probe_type, cb));
}

template<class T> void iks::get_first_time(TParam* param, std::function<void(const error&, T&) > result) {
//TODO
}

template<class T> void iks::get_last_time(TParam* param, std::function<void(const error&, T&) > result) {
//TODO
}

}
