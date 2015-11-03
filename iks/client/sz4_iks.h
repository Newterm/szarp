#include "iks_client.h"

namespace sz4 {

class param_observer;

class iks {

	struct observer_reg {
		IksConnection* c;
		std::string name;
		TParam* param;
		param_observer* p;
	};

	std::unordered_map<std::wstring, IksClient*> m_clients;
	std::vector<observer_reg> m_registrations;

	IksClient* client_for_param(TParam *param);

	template<class T> void search_data(TParam* param, const std::string& dir, const T& start, const T& end, SZARP_PROBE_TYPE probe_type, const search_condition& condition, std::function<void(error&, T&)> cb);

public:
	template<class V, class T> void get_weighted_sum(TParam* param, const T& start, const T& end, SZARP_PROBE_TYPE probe_type, std::function<void(error&, const std::vector< weighted_sum<V, T>& sum> >&) > cb);

	template<class T> void search_data_right(TParam* param, const T& start, const T& end, SZARP_PROBE_TYPE probe_type, const search_condition& condition, std::function<void(error&, T&) > cb);

	template<class T> void search_data_left(TParam* param, const T& start, const T& end, SZARP_PROBE_TYPE probe_type, const search_condition& condition, std::function<void(error&, T&) > cb);

	template<class T> void get_first_time(TParam* param, std::function<void(error&, T&) > cb);

	template<class T> void get_last_time(TParam* param, std::function<void(error&, T&) > cb);

	void register_observer(param_observer *observer, const std::vector<TParam*>& params, std::function<void(error&) > cb);

	void deregister_observer(param_observer *observer, const std::vector<TParam*>& params, std::function<void(error&) > cb);

/*
	void add_param(TParam* param, std::function<void(error&) > cb);

	void remove_param(TParam* param, std::function<void(error&) > cb);
*/

};

}

