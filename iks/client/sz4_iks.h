#include "sz4/defs.h"
#include "sz4/api.h"

#include "iks_location_connection.h"

class TParam;

namespace sz4 {

class param_observer;

class iks {

	struct observer_reg {
		IksLocationConnection* client;
		std::string name;
		TParam* param;
		param_observer* observer;
	};

	std::unordered_map<std::wstring, IksLocationConnection*> m_connections;
	std::vector<observer_reg> m_observer_regs;

	IksLocationConnection* connection_for_param(TParam *param);

	template<class T> void search_data(TParam* param, const std::string& dir, const T& start, const T& end, SZARP_PROBE_TYPE probe_type, const search_condition& condition, std::function<void(const error&, T&)> cb);

public:
	template<class V, class T> void get_weighted_sum(TParam* param, const T& start, const T& end, SZARP_PROBE_TYPE probe_type, std::function<void(const error&, const std::vector< weighted_sum<V, T> >&) > cb);

	template<class T> void search_data_right(TParam* param, const T& start, const T& end, SZARP_PROBE_TYPE probe_type, const search_condition& condition, std::function<void(const error&, T&)> cb);

	template<class T> void search_data_left(TParam* param, const T& start, const T& end, SZARP_PROBE_TYPE probe_type, const search_condition& condition, std::function<void(const error&, T&)> cb);

	template<class T> void get_first_time(TParam* param, std::function<void(const error&, T&) > cb);

	template<class T> void get_last_time(TParam* param, std::function<void(const error&, T&) > cb);

	void register_observer(param_observer *observer, const std::vector<TParam*>& params, std::function<void(const error&) > cb);

	void deregister_observer(param_observer *observer, const std::vector<TParam*>& params, std::function<void(const error&) > cb);

/*
	void add_param(TParam* param, std::function<void(const error&) > cb);

	void remove_param(TParam* param, std::function<void(const error&) > cb);
*/

};

}

