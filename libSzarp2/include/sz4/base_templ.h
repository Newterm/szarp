namespace sz4 {

template<class types>
base_templ<types>::base_templ(const std::wstring& szarp_data_dir, ipk_container_type* ipk_container, live_cache_config* live_config) : m_szarp_data_dir(szarp_data_dir), m_ipk_container(ipk_container) {
	m_interperter.reset(new lua_interpreter<base>());
	m_interperter->initialize(this, m_ipk_container);

	if (live_config)
		m_live_cache.reset(new live_cache(*live_config, new zmq::context_t(1)));


}

template<class types> boost::optional<SZARP_PROBE_TYPE>& base_templ<types>::read_ahead() {
	return m_read_ahead;
}

template<class types> buffer_templ<base_templ<types>>* base_templ<types>::buffer_for_param(TParam* param) {
	buffer_templ<base>* buf;
	if (param->GetConfigId() >= m_buffers.size())
		m_buffers.resize(param->GetConfigId() + 1, NULL);
	buf = m_buffers[param->GetConfigId()];

	if (buf == NULL) {
		std::wstring prefix = param->GetSzarpConfig()->GetPrefix();	
#if BOOST_FILESYSTEM_VERSION == 3
		buf = m_buffers[param->GetConfigId()] = new buffer_templ<base>(this, &m_monitor, m_ipk_container, prefix, (m_szarp_data_dir / prefix / L"szbase").wstring());
#else
		buf = m_buffers[param->GetConfigId()] = new buffer_templ<base>(this, &m_monitor, m_ipk_container, prefix, (m_szarp_data_dir / prefix / L"szbase").file_string());
#endif
	}
	return buf;
}

template<class types>
generic_param_entry* base_templ<types>::get_param_entry(TParam* param) {
	return buffer_for_param(param)->get_param_entry(param);
}

template<class types>
void base_templ<types>::remove_param(TParam* param) {
	if (param->GetConfigId() >= m_buffers.size())
		return;

	buffer_templ<base>* buf = m_buffers[param->GetConfigId()];
	if (buf == NULL)
		return;

	buf->remove_param(param);
}

template<class types>
void base_templ<types>::register_observer(param_observer *observer, const std::vector<TParam*>& params) {
	for (std::vector<TParam*>::const_iterator i = params.begin(); i != params.end(); i++) {
		generic_param_entry *entry = get_param_entry(*i);
		if (entry)
			entry->register_observer(observer);
	}
}

template<class types>
void base_templ<types>::deregister_observer(param_observer *observer, const std::vector<TParam*>& params) {
	for (std::vector<TParam*>::const_iterator i = params.begin(); i != params.end(); i++) {
		generic_param_entry *entry = get_param_entry(*i);
		if (entry)
			entry->deregister_observer(observer);
	}
}

template<class types>
fixed_stack_type& base_templ<types>::fixed_stack() { return m_fixed_stack; }

template<class types>
SzbParamMonitor& base_templ<types>::param_monitor() { return m_monitor; }

template<class types>
lua_interpreter<base_templ<types>>& base_templ<types>::get_lua_interpreter() { return *m_interperter; }

template<class types>
typename base_templ<types>::ipk_container_type* base_templ<types>::get_ipk_container() { return m_ipk_container; }

template<class types>
block_cache* base_templ<types>::cache() { return &m_cache; }

template<class types>
live_cache* base_templ<types>::get_live_cache() { return m_live_cache.get(); }

template<class types>
base_templ<types>::~base_templ() {
	for (auto i = m_buffers.begin(); i != m_buffers.end(); i++)
		delete *i;
	m_monitor.terminate();
}

}
