/* 
  SZARP: SCADA software 

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
#include "szarp_config.h"
#include "liblog.h"
#include "conversion.h"
#include <stdexcept>
#include <assert.h>

#include <algorithm>
#include <tr1/unordered_map>
#include <boost/thread/locks.hpp>

IPKContainer* IPKContainer::_object = NULL;

TParam* IPKContainerInfo::GetParam(const std::basic_string<unsigned char>& pname, bool add_cfg) {
	return GetParam(SC::U2S(pname), add_cfg);
}

TSzarpConfig* IPKContainerInfo::GetConfig(const std::basic_string<unsigned char>& prefix) {
	return GetConfig(SC::U2S(prefix));
}

IPKContainerInfo* IPKContainerInfo::GetObject() {
	return IPKContainer::GetObject();
}

IPKContainer::IPKContainer(const std::wstring& szarp_data_dir, const std::wstring& szarp_system_dir, const std::wstring &language) {
	this->szarp_data_dir = szarp_data_dir;
	this->szarp_system_dir = szarp_system_dir;
	this->language = language;
	this->max_config_id = 0;
}

IPKContainer::~IPKContainer() {
	for (CM::iterator i = configs.begin() ; i != configs.end() ; ++i) {
		delete i->second;
	}
}

void IPKContainer::Init(const std::wstring& szarp_data_dir, const std::wstring& szarp_system_dir, const std::wstring& language) {
	assert(_object == NULL);
	_object = new IPKContainer(szarp_data_dir, szarp_system_dir, language);
}

IPKContainer* IPKContainer::GetObject() {
	assert(_object);
	return _object;
}

TSzarpConfig* IPKContainer::GetConfig(const std::wstring& prefix) {
	boost::upgrade_lock<boost::shared_mutex> lock(m_lock);

	CM::iterator i = configs.find(prefix);
	if (i == configs.end()) {
		boost::upgrade_to_unique_lock<boost::shared_mutex> unique(lock);
		i = configs.find(prefix);
		if (i == configs.end()) {
			AddConfig(prefix, std::wstring());
			i = configs.find(prefix);
		}
	}

	TSzarpConfig *result;
	if (i == configs.end()) {
		sz_log(2, "Config %ls not found", prefix.c_str());
		result = NULL;
	} else {
		result = i->second;
	}
	return result;

}

TParam* IPKContainer::GetParamFromHash(const std::basic_string<unsigned char>& global_param_name) {
	utf_hash_type::iterator i = m_utf_params.find(global_param_name);
	if (i != m_utf_params.end())
		return i->second;
	else
		return NULL;

}

TParam* IPKContainer::GetParamFromHash(const std::wstring& global_param_name) {
	hash_type::iterator i = m_params.find(global_param_name);
	if (i != m_params.end())
		return i->second;
	else
		return NULL;
}

TParam* IPKContainer::GetParam(const std::wstring& pn, bool af) {
	return GetParamImpl(pn, af);
}

TParam* IPKContainer::GetParam(const std::basic_string<unsigned char>& pn, bool af) {
	return GetParamImpl(pn, af);
}

template<class T>  struct comma_char_trait {
};

template<> struct comma_char_trait<wchar_t> {
	static const std::wstring comma;
};

const std::wstring comma_char_trait<wchar_t>::comma = L":";

template<> struct comma_char_trait<unsigned char> {
	static const std::basic_string<unsigned char> comma;
};

const std::basic_string<unsigned char> comma_char_trait<unsigned char>::comma = (const unsigned char*)":";

template <typename T>
TParam* IPKContainer::GetParamImpl(const std::basic_string<T>& global_param_name, bool add_config_if_not_present) {
	{
		boost::shared_lock<boost::shared_mutex> shared_lock(m_lock);
		TParam* param = GetParamFromHash(global_param_name);
		if (param != NULL)
			return param;	
		if (!add_config_if_not_present)
			return NULL;
	}

	
	size_t cp = global_param_name.find_first_of(comma_char_trait<T>::comma);
	if (cp == std::basic_string<T>::npos)
		return NULL;

	std::basic_string<T> prefix = global_param_name.substr(0, cp);
	if (GetConfig(prefix) == NULL)
		return NULL;

	return GetParam(global_param_name, false);
}

template TParam* IPKContainer::GetParamImpl<wchar_t>(const std::basic_string<wchar_t>&, bool);
template TParam* IPKContainer::GetParamImpl<unsigned char>(const std::basic_string<unsigned char>&, bool);

bool IPKContainer::ReadyConfigurationForLoad(const std::wstring &prefix) { 
	boost::unique_lock<boost::shared_mutex> lock(m_lock);
	if (configs_ready_for_load.find(prefix) != configs_ready_for_load.end())
		return true;

	boost::filesystem::wpath _file;
	_file = szarp_data_dir / prefix / L"config" / L"params.xml";

	TSzarpConfig* ipk = new TSzarpConfig(); 

#if BOOST_FILESYSTEM_VERSION == 3
	if (ipk->loadXML(_file.wstring(), prefix)) {
			sz_log(2, "Unable to load config from file: %s", SC::S2U(_file.wstring()).c_str());
#else
	if (ipk->loadXML(_file.file_string(), prefix)) {
			sz_log(2, "Unable to load config from file: %s", SC::S2U(_file.file_string()).c_str());
#endif

		delete ipk;
		return false;
	}

	configs_ready_for_load[prefix] = ipk;

	return true;
}

void IPKContainer::AddUserDefinedImpl(const std::wstring& prefix, TParam *n) {
	m_extra_params[prefix].emplace_back(n);

	CM::iterator i = configs.find(prefix);
	if (i == configs.end())
		return;
	n->SetParentSzarpConfig(i->second);

	ConfigAux& aux = config_aux[prefix];
	if (aux._freeIds.size()) {
		std::set<unsigned>::iterator i = aux._freeIds.begin();
		n->SetParamId(*i);
		aux._freeIds.erase(i);
	} else {
		n->SetParamId(aux._maxParamId++);
	}

	n->SetConfigId(aux._configId);

	AddParamToHash(n);
}

void IPKContainer::RemoveUserDefinedImpl(const std::wstring& prefix, TParam *p) {
	auto& tp = m_extra_params[prefix];
	auto ei = std::find_if(tp.begin(), tp.end(), 
		[p] (std::shared_ptr<TParam> &_p) { return _p.get() == p; }
	);
	assert(ei != tp.end());

	ConfigAux& aux = config_aux[prefix];
	if (p->GetParamId() + 1 == aux._maxParamId)
		aux._maxParamId--;
	else
		aux._freeIds.insert(p->GetParamId());

	auto i = configs.find(prefix);
	if (i != configs.end())
		RemoveParamFromHash(p);

	tp.erase(ei);
}

void IPKContainer::AddUserDefined(const std::wstring& prefix, TParam *n) {
	boost::unique_lock<boost::shared_mutex> lock(m_lock);
	AddUserDefinedImpl(prefix, n);
}

void IPKContainer::RemoveUserDefined(const std::wstring& prefix, TParam *p) {
	boost::unique_lock<boost::shared_mutex> lock(m_lock);
	RemoveUserDefinedImpl(prefix, p);
}

void IPKContainer::RemoveParamFromHash(TParam* p) {
	std::wstring global_param_name = p->GetSzarpConfig()->GetPrefix() + L":" + p->GetName();
	std::wstring translated_global_param_name = p->GetSzarpConfig()->GetPrefix() + L":" + p->GetTranslatedName();

	{
		auto i = m_params.find(global_param_name);
		if (i != m_params.end() && i->second == p)
			m_params.erase(i);
	}

	{
		auto i = m_params.find(translated_global_param_name);
		if (i != m_params.end() && i->second == p)
			m_params.erase(i);
	}

	{
		auto i = m_utf_params.find(SC::S2U(global_param_name));
		if (i != m_utf_params.end() && i->second == p)
			m_utf_params.erase(i);
	}

	{
		auto i = m_utf_params.find(SC::S2U(translated_global_param_name));
		if (i != m_utf_params.end() && i->second == p)
			m_utf_params.erase(i);
	}

}

void IPKContainer::AddParamToHash(TParam* p) {
	std::wstring global_param_name = p->GetSzarpConfig()->GetPrefix() + L":" + p->GetName();
	std::wstring translated_global_param_name = p->GetSzarpConfig()->GetPrefix() + L":" + p->GetTranslatedName();

	m_params[global_param_name] = p;
	m_params[translated_global_param_name] = p;

	m_utf_params[SC::S2U(global_param_name)] = p;
	m_utf_params[SC::S2U(translated_global_param_name)] = p;
}

TSzarpConfig* IPKContainer::AddConfig(const std::wstring& prefix, const std::wstring &file) {
	boost::filesystem::wpath _file;

	TSzarpConfig* ipk; 
	if (configs_ready_for_load.find(prefix) != configs_ready_for_load.end()) {
		ipk = configs_ready_for_load[prefix];
		configs_ready_for_load.erase(prefix);
	} else {
		ipk = new TSzarpConfig();

		if (file.empty())
			_file = szarp_data_dir / prefix / L"config" / L"params.xml";
		else
			_file = file;

		
#if BOOST_FILESYSTEM_VERSION == 3
		if (ipk->loadXML(_file.wstring(), prefix)) {
			sz_log(2, "Unable to load config from file: %s", SC::S2L(_file.wstring()).c_str());
#else
		if (ipk->loadXML(_file.file_string(), prefix)) {
			sz_log(2, "Unable to load config from file: %s", SC::S2L(_file.file_string()).c_str());
#endif
			delete ipk;
			return NULL;
		}
	}

#if BOOST_FILESYSTEM_VERSION == 3
	TDictionary d(szarp_system_dir.wstring());
#else
	TDictionary d(szarp_system_dir.string());
#endif
	d.TranslateIPK(ipk, language);

	ConfigAux ca;
	bool first_time_added = !configs.count(prefix);
	if (!first_time_added) {
		delete configs[prefix];
		ca._configId = config_aux[prefix]._configId;
	} else  {
		ca._configId = max_config_id++;
	}
	ipk->SetConfigId(ca._configId);
	configs[prefix] = ipk;

	unsigned id = 0;
	for (TParam* p = ipk->GetFirstParam(); p; p = p->GetNextGlobal()) {
		p->SetParamId(id++);
		p->SetConfigId(ca._configId);
		AddParamToHash(p);
	}

	auto& pv = m_extra_params[prefix];
	for (auto i = pv.begin(); i != pv.end(); i++) {
		(*i)->SetParamId(id++);
		(*i)->SetParentSzarpConfig(ipk);
		(*i)->SetConfigId(ca._configId);
		AddParamToHash(i->get());
	}

	ca._maxParamId = id;
	config_aux[prefix] = ca;
	return ipk;
}

TSzarpConfig* IPKContainer::LoadConfig(const std::wstring& prefix, const std::wstring& file) {
	boost::unique_lock<boost::shared_mutex> lock(m_lock);
	return AddConfig(prefix, file);
}

void IPKContainer::Destroy() {
	delete _object;
	_object = NULL;
}
