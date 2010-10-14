/* 
  SZARP: SCADA software 

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

bool IPKContainer::ReadyConfigurationForLoad(const std::wstring &prefix) {
	TSMutexLocker locker(mutex);

	if (configs_ready_for_load.find(prefix) != configs_ready_for_load.end())
		return true;

	boost::filesystem::wpath _file;
	_file = szarp_data_dir / prefix / L"config" / L"params.xml";

	TSzarpConfig* ipk = new TSzarpConfig(); 

	if (ipk->loadXML(_file.file_string(), prefix)) {
			sz_log(2, "Unable to load config from file:%ls", _file.file_string().c_str());
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

	TSzarpConfig *ipk = i->second;
	TParam *p = ipk->GetFirstParam();
	assert(p);

	while (p->GetNext(true))
		p = p->GetNext(true);

	p->SetNext(n); 
	n->SetNext(NULL);

	n->SetParentSzarpConfig(ipk);

}

void IPKContainer::RemoveExtraParam(const std::wstring& prefix, TParam *p) {
	TSMutexLocker locker(mutex);

	std::vector<TParam*>& tp = m_extra_params[prefix];
	std::vector<TParam*>::iterator ei = std::remove(tp.begin(), tp.end(), p);
	tp.erase(ei, tp.end());

	CM::iterator i = configs.find(prefix);
	if (i == configs.end()) {
		delete p;
		return;
	}

	TSzarpConfig *ipk = i->second;
	TParam *pp = NULL;
	for (TParam *cp = ipk->GetFirstParam(); cp; cp = cp->GetNext(true)) {
		if (cp == p) {
			assert(pp);
			pp->SetNext(cp->GetNext(true));
			break;
		}
		pp = cp;
	}
	p->SetNext(NULL);
	delete p;
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

		
		if (ipk->loadXML(_file.file_string(), prefix)) {
			sz_log(2, "Unable to load config from file:%ls", _file.file_string().c_str());
			delete ipk;
			return NULL;
		}

	}

	std::wstring name = prefix;

	TDictionary d(szarp_system_dir.string());
	d.TranslateIPK(ipk, language);
		
	std::vector<TParam*>& pv = m_extra_params[name];

	if (configs.find(name) != configs.end()) {
		TSzarpConfig *ipk = configs[name];

		TParam *p = ipk->GetFirstParam();
		while (p && p->GetNext(true)) {
			std::vector<TParam*>::iterator i = std::find(pv.begin(), pv.end(), p->GetNext(true));
			if (i != pv.end())
				p->SetNext(p->GetNext(true)->GetNext(true));
			else 
				p = p->GetNext(true);
		}

		delete configs[name];
	}

	configs[name] = ipk;

	TParam *prv = ipk->GetFirstParam();
	while (prv && prv->GetNext(true) != NULL) 
		prv = prv->GetNext(true);

	if (prv) for (std::vector<TParam*>::iterator i = pv.begin(); 
			i != pv.end();
			i++) {

		TParam *p = *i;

		prv->SetNext(p);

		p->SetParentSzarpConfig(ipk);
		p->SetNext(NULL);

		prv = p;
	}

	return ipk;

}

TSzarpConfig* IPKContainer::LoadConfig(const std::wstring& prefix, const std::wstring& file) {
	TSMutexLocker locker(mutex);
	return AddConfig(prefix, file);
}


void IPKContainer::Destroy() {
	delete _object->mutex;
	delete _object;
	_object = NULL;
}
