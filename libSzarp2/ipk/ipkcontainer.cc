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

TSMutex::~TSMutex() {}

TSMutexLocker::TSMutexLocker(TSMutex *mutex) {
	this->mutex = mutex;
	mutex->Lock();
}

TSMutexLocker::~TSMutexLocker() {
	mutex->Release();
}

IPKContainer* IPKContainer::_object = NULL;

IPKContainer::IPKContainer(const std::wstring& szarp_data_dir, const std::wstring& szarp_system_dir, const std::wstring &language, TSMutex* mutex) {
	this->szarp_data_dir = szarp_data_dir;
	this->szarp_system_dir = szarp_system_dir;
	this->language = language;
	this->mutex = mutex;
}

IPKContainer::~IPKContainer() {
	for (CM::iterator i = configs.begin() ; i != configs.end() ; ++i) {
		delete i->second;
	}
}


void IPKContainer::Init(const std::wstring& szarp_data_dir, const std::wstring& szarp_system_dir, const std::wstring& language, TSMutex *mutex) {
	assert(_object == NULL);
	_object = new IPKContainer(szarp_data_dir, szarp_system_dir, language, mutex);
}

IPKContainer* IPKContainer::GetObject() {
	assert(_object);
	return _object;
}

TSzarpConfig* IPKContainer::GetConfig(const std::wstring& config) {
	TSMutexLocker locker(mutex);

	CM::iterator ci = configs.find(config.c_str());
	if (ci == configs.end()) {
		AddConfig(config, std::wstring());
		ci = configs.find(config);
	}

	TSzarpConfig *result;

	if (ci == configs.end()) {
		sz_log(2, "Config %ls not found", config.c_str());
		result = NULL;
	} else {
		result = ci->second;
	}

	return result;

}

TParam* IPKContainer::GetParam(const std::wstring& global_param_name) {
	size_t cp = global_param_name.find_first_of(L":");
	if (cp == std::wstring::npos)
		return NULL;

	std::wstring prefix = global_param_name.substr(0, cp);

	TSzarpConfig *szarp_config = GetConfig(prefix);
	if (szarp_config == NULL)
		return NULL;

	std::wstring lpname  = global_param_name.substr(cp + 1);
	return szarp_config->getParamByName(lpname);
}

bool IPKContainer::ReadyConfigurationForLoad(const std::wstring &prefix, bool logparams) { 
	TSMutexLocker locker(mutex);

	if (configs_ready_for_load.find(prefix) != configs_ready_for_load.end())
		return true;

	boost::filesystem::wpath _file;
	_file = szarp_data_dir / prefix / L"config" / L"params.xml";

	TSzarpConfig* ipk = new TSzarpConfig(logparams); 

#if BOOST_FILESYSTEM_VERSION == 3
	if (ipk->loadXML(_file.wstring(), prefix)) {
			sz_log(2, "Unable to load config from file:%ls", _file.wstring().c_str());
#else
	if (ipk->loadXML(_file.file_string(), prefix)) {
			sz_log(2, "Unable to load config from file:%ls", _file.file_string().c_str());
#endif

		delete ipk;
		return false;
	}

	configs_ready_for_load[prefix] = ipk;

	return true;

}



void IPKContainer::AddExtraParam(const std::wstring& prefix, TParam *n) {
	TSMutexLocker locker(mutex);
	m_extra_params[prefix].push_back(n);

	CM::iterator i = configs.find(prefix);
	if (i == configs.end())
		return;
	n->SetParentSzarpConfig(i->second);
}

void IPKContainer::RemoveExtraParam(const std::wstring& prefix, TParam *p) {
	TSMutexLocker locker(mutex);

	std::vector<TParam*>& tp = m_extra_params[prefix];
	std::vector<TParam*>::iterator ei = std::remove(tp.begin(), tp.end(), p);
	tp.erase(ei, tp.end());

	delete p;
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

		
#if BOOST_FILESYSTEM_VERSION == 3
		if (ipk->loadXML(_file.wstring(), prefix)) {
			sz_log(2, "Unable to load config from file:%ls", _file.wstring().c_str());
#else
		if (ipk->loadXML(_file.file_string(), prefix)) {
			sz_log(2, "Unable to load config from file:%ls", _file.file_string().c_str());
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
	if (configs.find(prefix) != configs.end())
		delete configs[prefix];
	configs[prefix] = ipk;
	std::vector<TParam*>& pv = m_extra_params[prefix];
	std::for_each(pv.begin(), pv.end(), std::bind2nd(std::mem_fun(&TParam::SetParentSzarpConfig), ipk));
	return ipk;

}

TSzarpConfig* IPKContainer::LoadConfig(const std::wstring& prefix, const std::wstring& file , bool logparams) {
	TSMutexLocker locker(mutex);
	return AddConfig(prefix, file, logparams);
}

void IPKContainer::Destroy() {
	delete _object->mutex;
	delete _object;
	_object = NULL;
}
