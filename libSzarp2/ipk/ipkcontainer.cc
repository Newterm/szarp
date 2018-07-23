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
#include "ipkcontainer.h"
#include "szarp_config.h"
#include "liblog.h"
#include "conversion.h"
#include "custom_assert.h"

#include <boost/thread/locks.hpp>

IPKContainer* IPKContainer::_object = nullptr;

TParam* IPKContainer::GetParam(const std::basic_string<unsigned char>& pname, bool add_cfg) {
	return GetParam(SC::U2S(pname), add_cfg);
}

TSzarpConfig* IPKContainer::GetConfig(const std::basic_string<unsigned char>& prefix) {
	return GetConfig(SC::U2S(prefix));
}

IPKContainer* IPKContainer::GetObject() {
	return _object;
}

void IPKContainer::Destroy() {
	if (_object != nullptr) {
		delete _object;
		_object = nullptr;
	} else {
		ASSERT(!"Destroy called on nullptr object!");
	}
}

ParamCachingIPKContainer::ParamCachingIPKContainer(const std::wstring& szarp_data_dir, const std::wstring& szarp_system_dir, const std::wstring &language) {
	this->szarp_data_dir = szarp_data_dir;
	this->szarp_system_dir = szarp_system_dir;
	this->language = language;
	this->max_config_id = 0;
}

ParamCachingIPKContainer::~ParamCachingIPKContainer() {
	for (auto i = configs.begin() ; i != configs.end() ; ++i) {
		delete i->second;
	}
}

void ParamCachingIPKContainer::Init(const std::wstring& szarp_data_dir, const std::wstring& szarp_system_dir, const std::wstring& language) {
	assert(IPKContainer::_object == nullptr);
	IPKContainer::_object = new ParamCachingIPKContainer(szarp_data_dir, szarp_system_dir, language);
}

TSzarpConfig* ParamCachingIPKContainer::GetConfig(const std::wstring& prefix) {
	boost::upgrade_lock<boost::shared_mutex> lock(m_lock);

	TSzarpConfig *config = configs.get(prefix);
	if (!config) {
		boost::upgrade_to_unique_lock<boost::shared_mutex> unique(lock);
		if ((config = configs.get(prefix))) {
			return config;
		} else {
			config = AddConfig(prefix);
		}
	}

	if (!config) {
		sz_log(2, "Config %ls not found", prefix.c_str());
	}

	return config;

}

TParam* ParamCachingIPKContainer::GetParam(const std::wstring& pn, bool af) {
	return GetParamImpl(pn, af);
}

TParam* ParamCachingIPKContainer::GetParam(const std::basic_string<unsigned char>& pn, bool af) {
	return GetParamImpl(pn, af);
}

template<class T>  struct comma_char_trait {};

template<> struct comma_char_trait<wchar_t> {
	static constexpr wchar_t comma =  L':';
};

template<> struct comma_char_trait<unsigned char> {
	static constexpr unsigned char comma = ':';
};

template <typename T>
TParam* ParamCachingIPKContainer::GetParamImpl(const std::basic_string<T>& global_param_name, bool add_config_if_not_present) {
	{
		boost::shared_lock<boost::shared_mutex> shared_lock(m_lock);
		TParam* param = cacher.GetParam(global_param_name);
		if (param != nullptr)
			return param;	
		if (!add_config_if_not_present)
			return nullptr;
	}

	
	size_t cp = global_param_name.find_first_of(comma_char_trait<T>::comma);
	if (cp == std::basic_string<T>::npos)
		return nullptr;

	std::basic_string<T> prefix = global_param_name.substr(0, cp);
	if (GetConfig(prefix) == nullptr)
		return nullptr;

	return GetParamImpl(global_param_name, false);
}

template TParam* ParamCachingIPKContainer::GetParamImpl<wchar_t>(const std::basic_string<wchar_t>&, bool);
template TParam* ParamCachingIPKContainer::GetParamImpl<unsigned char>(const std::basic_string<unsigned char>&, bool);

TSzarpConfig* ParamCachingIPKContainer::InitializeIPK(const std::wstring &prefix) {
	auto ipk = new TSzarpConfig();

	boost::filesystem::wpath file = szarp_data_dir / prefix / L"config" / L"params.xml";

#if BOOST_FILESYSTEM_VERSION == 3
	if (ipk->loadXML(file.wstring(), prefix)) {
		sz_log(2, "Unable to load config from file: %s", SC::S2L(file.wstring()).c_str());
#else
	if (ipk->loadXML(file.file_string(), prefix)) {
		sz_log(2, "Unable to load config from file: %s", SC::S2L(file.file_string()).c_str());
#endif
		delete ipk;
		return nullptr;
	}

	return ipk;
}

TSzarpConfig* ParamCachingIPKContainer::AddConfig(const std::wstring& prefix) {
	auto ipk = InitializeIPK(prefix);
	if (!ipk)
		return nullptr;

	AddConfig(ipk, prefix);
	return ipk;
}

TSzarpConfig* ParamCachingIPKContainer::AddConfig(TSzarpConfig* ipk, const std::wstring& prefix) {
	ASSERT(ipk);
	if (!ipk)
		return nullptr;

#if BOOST_FILESYSTEM_VERSION == 3
	TDictionary d(szarp_system_dir.wstring());
#else
	TDictionary d(szarp_system_dir.string());
#endif
	d.TranslateIPK(ipk, language);

	ConfigAux ca;
	if (configs.get(prefix) != nullptr) {
		delete configs.get(prefix);
		configs.remove(prefix);
		ca._configId = config_aux[prefix]._configId;
	} else  {
		ca._configId = max_config_id++;
	}

	ipk->SetConfigId(ca._configId);
	configs.insert(prefix, ipk);

	unsigned id = 0;
	for (TParam* p = ipk->GetFirstParam(); p; p = p->GetNextGlobal()) {
		p->SetParamId(id++);
		p->SetConfigId(ca._configId);
		cacher.AddParam(p);
	}

	ca._maxParamId = id;
	config_aux[prefix] = ca;
	return ipk;
}


TSzarpConfig* ParamCachingIPKContainer::LoadConfig(const std::wstring& prefix) {
	boost::unique_lock<boost::shared_mutex> lock(m_lock);
	return AddConfig(prefix);
}

void ParamsCacher::AddParam(TParam* p) {
	ASSERT(p);
	if (!p) return;

	std::wstring global_param_name = p->GetSzarpConfig()->GetPrefix() + L":" + p->GetName();
	std::wstring translated_global_param_name = p->GetSzarpConfig()->GetPrefix() + L":" + p->GetTranslatedName();

	m_params.insert(global_param_name, p);
	m_params.insert(translated_global_param_name, p);

	m_utf_params.insert(SC::S2U(global_param_name), p);
	m_utf_params.insert(SC::S2U(translated_global_param_name), p);
}

TParam* ParamsCacher::GetParam(const US& global_param_name) {
	return m_utf_params.get(global_param_name);
}

TParam* ParamsCacher::GetParam(const SS& global_param_name) {
	return m_params.get(global_param_name);
}

bool ParamsCacher::RemoveParam(TParam* p) {
	ASSERT(p);
	if (!p) return false;

	std::wstring global_param_name = p->GetSzarpConfig()->GetPrefix() + L":" + p->GetName();
	std::wstring translated_global_param_name = p->GetSzarpConfig()->GetPrefix() + L":" + p->GetTranslatedName();

	m_params.remove(global_param_name);
	m_params.remove(translated_global_param_name);
	m_utf_params.remove(SC::S2U(global_param_name));
	m_utf_params.remove(SC::S2U(translated_global_param_name));

	return true;
}

template <typename ST, typename VT, typename HT>
bool Cache<ST, VT, HT>::remove(const ST& pn) {
	auto i = cached_values.find(pn);
	if (i != cached_values.end()) {
		cached_values.erase(i);
		return true;
	}

	return false;
}

template <typename ST, typename VT, typename HT>
void Cache<ST, VT, HT>::insert(const ST& pn, VT* p) {
	cached_values[pn] = p;
}

template <typename ST, typename VT, typename HT>
VT* Cache<ST, VT, HT>::get(const ST& pn) {
	auto i = cached_values.find(pn);
	if (i == cached_values.end())
		return (VT*) nullptr;
	return i->second;
}

template class Cache<std::wstring, TSzarpConfig>;
template class Cache<ParamsCacher::SS, TParam>;
template class Cache<ParamsCacher::US, TParam, ParamsCacher::USHash>;
