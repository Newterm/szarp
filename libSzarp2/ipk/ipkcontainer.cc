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

IPKContainer* IPKContainer::_object = NULL;

IPKContainer::IPKContainer(const std::wstring& szarp_data_dir, const std::wstring& szarp_system_dir, const std::wstring &language, TSMutex* mutex) {
	this->szarp_data_dir = szarp_data_dir;
	this->szarp_system_dir = szarp_system_dir;
	this->language = language;
	this->mutex = mutex;
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

TSzarpConfig* IPKContainer::GetConfig(const std::basic_string<unsigned char>& prefix) {
	return SC::U2S(GetConfig(prefix));
}

TSzarpConfig* IPKContainer::GetConfig(const std::wstring& prefix) {
	boost::mutex::upgrade_lock shared_lock(m_lock);

	CM::iterator i = prefixs.find(prefix.c_str());
	if (i != prefixs.end()) {
		boost::mutex::upgrade_to_unique(shared_lock);
		i = prefixs.find(prefix.c_str());
		if (i == prefix.end()) {
			AddConfig(prefix, std::wstring());
			i = prefixs.find(prefix);
		}
	}

	TSzarpConfig *result;
	if (i == prefixs.end()) {
		sz_log(2, "Config %ls not found", prefix.c_str());
		result = NULL;
	} else {
		result = i->second;
	}
	return result;

}

TParam* IPKContainer::GetParamFromHash(const std::basic_string<unsigned char>& global_param_name) {
	utf_hash_type::iterator i = m_utf_params_hash.find(global_param_name);
	if (i != m_utf_params_hash.end())
		return *i;
	else
		return NULL;

}

TParam* IPKContainer::GetParamFromHash(const std::wstring& global_param_name) {
	hash_type::iterator i = m_params_hash.find(global_param_name);
	if (i != m_params_hash.end())
		return *i;
	else
		return NULL;
}

template<class T>  struct comma_char_trait {
};

template<> struct comma_char_trait<wchar_t> {
	static const std::wstring comma = L":';
};

template<> struct comma_char_trait<unsigned char> {
	static const std::basic_string<unsigned char> comma = (const unsigned char*)":";
};

template<class T> TParam* IPKContainer::GetParam(const std::basic_string<T>char>& global_param_name, bool add_config_if_not_present) {
	{
		boost::thread::shared_lock shared_lock(m_mutex);
		TParam* param = GetParamFromHash(global_param_name);
		if (param != NULL)
			return param;	
		if (i != m_params_hash.end())
			return i->second;
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


bool IPKContainer::ReadyConfigurationForLoad(const std::wstring &prefix, bool logparams) { 
	boost::mutex::unique_lock lock(m_lock);
	if (configs_ready_for_load.find(prefix) != configs_ready_for_load.end())
		return true;

	boost::filesystem::wpath _file;
	_file = szarp_data_dir / prefix / L"config" / L"params.xml";

	TSzarpConfig* ipk = new TSzarpConfig(logparams); 
	if (ipk->loadXML(_file.file_string(), prefix)) {
			sz_log(2, "Unable to load config from file:%ls", _file.file_string().c_str());
		delete ipk;
		return false;
	}

	configs_ready_for_load[prefix] = ipk;

	return true;
}

void IPKContainer::AddExtraParam(const std::wstring& prefix, TParam *n) {
	boost::mutex::unique_lock lock(m_lock);
	m_extra_params[prefix].push_back(n);

	CM::iterator i = configs.find(prefix);
	if (i == configs.end())
		return;
	n->SetParentSzarpConfig(i->second);

	ConfigAux& aux = config_aux[prefix];
	if (aux._freeIds.size()) {
		std::set<unsigned>::iterator i = aux._freeIds.begin();
		n->SetParamId(*i);
		aux._freeIds.erase(i);
	} else
		n->SetParamId(aux._maxParamId++);
	n->SetConfigId(aux._configId);

	AddParamToHash(n);
}

void IPKContainer::RemoveExtraParam(const std::wstring& prefix, TParam *p) {
	boost::mutex::unique_lock lock(m_lock);

	std::vector<TParam*>& tp = m_extra_params[prefix];
	std::vector<TParam*>::iterator ei = std::remove(tp.begin(), tp.end(), p);
	tp.erase(ei, tp.end());

	ConfigAux& aux = config_aux[prefix];
	if (p->GetParamId() + 1 == aux._maxParamId)
		aux._maxParamId--;
	else
		aux._freeIds.insert(p->GetParamId());

	RemoveParamFromHash(p);
	delete p;
}

void IPKContainer::RemoveParamFromHash(TParam* p) {
	std::wstring global_param_name = p->GetSzarpConfig()->GetPrefix() + L":" + p->GetName();
	std::wstring translated_global_param_name = p->GetSzarpConfig()->GetPrefix() + L":" + p->GetTranslatedName();

	m_params_hash.erase(global_param_name);
	m_params_hash.erase(translated_global_param_name);

	m_utf_params_hash.erase(SC::S2U(global_param_name));
	m_utf_params_hash.erase(SC::S2U(translated_global_param_name));
}

void IPKContainer::AddParamToHash(TParam* p) {
	std::wstring global_param_name = p->GetSzarpConfig()->GetPrefix() + L":" + p->GetName();
	std::wstring translated_global_param_name = p->GetSzarpConfig()->GetPrefix() + L":" + p->GetTranslatedName();

	m_params_hash[global_param_name] = p;
	m_params_hash[translated_global_param_name] = p;

	m_utf_params_hash[SC::S2U(global_param_name)] = p;
	m_utf_params_hash[SC::S2U(translated_global_param_name)] = p;
}

TSzarpConfig* IPKContainer::AddConfig(const std::wstring& prefix, const std::wstring &file, bool logparams ) {
	boost::filesystem::wpath _file;

	TSzarpConfig* ipk; 
	if (configs_ready_for_load.find(prefix) != configs_ready_for_load.end()) {
		ipk = configs_ready_for_load[prefix];
		configs_ready_for_load.erase(prefix);
	} else {
		ipk = new TSzarpConfig(logparams);

		if (file.empty())
			_file = szarp_data_dir / prefix / L"config" / L"params.xml";
		else
			_file = file;

		if (ipk->loadXML(_file.file_string(), prefix)) {
			sz_log(2, "Unable to load config from file:%ls", _file.file_string().c_str());
			delete ipk;
			return NULL;
		}
	}

	TDictionary d(szarp_system_dir.string());
	d.TranslateIPK(ipk, language);

	ConfigAux ca;
	if (configs.find(prefix) != configs.end()) {
		delete configs[prefix];
		ca._configId = config_aux[prefix]._configId;
	} else  {
		ca._configId = max_config_id++;
	}
	ipk->SetConfigId(ca._configId);
	configs[prefix] = ipk;

	unsigned id = 0;
	for (TParam* p = ipk->GetFirstParam(); p; p = p->GetNext(true)) {
		p->SetParamId(id++);
		p->SetConfigId(ca._configId);
		AddParamToHash(p);
	}

	std::vector<TParam*>& pv = m_extra_params[prefix];
	for (std::vector<TParam*>::iterator i = pv.begin(); i != pv.end(); i++) {
		(*i)->SetParamId(id++);
		(*i)->SetParentSzarpConfig(ipk);
		(*i)->SetConfigId(ca._configId);
		AddParamToHash(*i);
	}

	ca._maxParamId = id;
	config_aux[prefix] = ca;

	return ipk;
}

TSzarpConfig* IPKContainer::LoadConfig(const std::wstring& prefix, const std::wstring& file , bool logparams) {
	boost::mutex::unique_lock lock(m_lock);
	return AddConfig(prefix, file, logparams);
}

void IPKContainer::Destroy() {
	delete _object;
	_object = NULL;
}
