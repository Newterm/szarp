#ifndef __SIMPLE_MOCKS_H__
#define __SIMPLE_MOCKS_H__

#include "conversion.h"

namespace mocks {

class IPKContainerMock {
	TParam param;
	TSzarpConfig config;
	int param_no;
public:
	IPKContainerMock() : param(NULL) {
		param.SetConfigId(0);
		param.SetParamId(0);
		param.SetParentSzarpConfig(&config);
		
		config.SetName(L"BASE", L"BASE");
		param_no = 1;
	}

	void add_param(TParam* param) {
		param->SetConfigId(0);
		param->SetParamId(param_no++);
		param->SetParentSzarpConfig(&config);

	}
	TSzarpConfig* GetConfig(const std::wstring&) { return (TSzarpConfig*) 1; }
	template<class T> TParam* GetParam(const std::basic_string<T>&) { return &param; }
};

class IPKContainerMockBase {
protected:
	TParam m_heartbeat_param;
	TSzarpConfig m_config;
	int m_next_free_param_id;
	static const int conf_id = 0;
public:
	IPKContainerMockBase() : m_heartbeat_param(NULL, NULL, std::wstring(), TParam::NONE, TParam::P_REAL) {
		m_next_free_param_id = 0;
		m_heartbeat_param.SetConfigId(0);
		m_heartbeat_param.SetParamId(m_next_free_param_id++);
		m_heartbeat_param.SetDataType(TParam::SHORT);
		m_heartbeat_param.SetParentSzarpConfig(&m_config);
		m_heartbeat_param.SetName(L"Status:Meaner3:program_uruchomiony");
		m_config.SetName(L"BASE", L"BASE");
	}

	TSzarpConfig* GetConfig(const std::wstring& prefix) { return &m_config; }

	TParam* GetParam(const std::wstring& name) {
		if (name.find(L":Status:Meaner3:program_uruchomiony") != std::wstring::npos)
			return &m_heartbeat_param;
		else
			return DoGetParam(name);
	}

	TParam* GetParam(const std::basic_string<unsigned char>& name) {
		return GetParam(SC::U2S(name));
	}

	virtual TParam* DoGetParam(const std::wstring& name) = 0;

	void AddParam(TParam* param) {
		param->SetConfigId(0);
		param->SetParamId(m_next_free_param_id++);
		param->SetParentSzarpConfig(&m_config);
	}
};

template<class value_type, class time_type, class base> class fake_entry_type {
public:
	fake_entry_type(base* _base, TParam* param, const boost::filesystem::wpath& path) {}

	void get_weighted_sum_impl(time_type start, time_type end, SZARP_PROBE_TYPE probe_type, sz4::weighted_sum<value_type, time_type>& sum) {}


	time_type search_data_right_impl(time_type start, time_type end, SZARP_PROBE_TYPE probe_type, const sz4::search_condition& condition) {
		return start;
	}

	time_type search_data_left_impl(time_type start, time_type end, SZARP_PROBE_TYPE probe_type, const sz4::search_condition& condition) {
		return start;
	}

	void get_first_time(std::list<sz4::generic_param_entry*>& referred_params, time_type &t) {
		t = sz4::time_trait<time_type>::first_valid_time;
	}

	void get_last_time(const std::list<sz4::generic_param_entry*>& referred_params, time_type &t) {
		t = sz4::time_trait<time_type>::last_valid_time;
	}

	void register_at_monitor(sz4::generic_param_entry* entry, SzbParamMonitor* monitor) { }

	void deregister_from_monitor(sz4::generic_param_entry* entry, SzbParamMonitor* monitor) { }

	void param_data_changed(TParam*, const std::string& path) { }

	void refferred_param_removed(sz4::generic_param_entry* param_entry) { }

	void set_live_block(sz4::generic_live_block* block) {}

	virtual ~fake_entry_type() {}
};

struct mock_param_factory {
	template<
		template<typename DT, typename TT, class BT> class entry_type,
		typename base
	>
	sz4::generic_param_entry* create(base* _base, TParam* param, const boost::filesystem::wpath &buffer_directory) {
		if(param->GetName() == L"Status:Meaner3:program_uruchomiony") {
			return sz4::param_entry_factory().template create<fake_entry_type, base>(_base, param, buffer_directory);
		} else {
			return sz4::param_entry_factory().template create<entry_type, base>(_base, param, buffer_directory);
		}
	}

};

struct mock_types {
	typedef IPKContainerMock ipk_container_type;
	typedef sz4::param_entry_factory param_factory;
};


}


#endif
