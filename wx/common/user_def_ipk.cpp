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
/* 
 * draw3
 * SZARP

 *
 * $Id$
 *
 * Contains user defined IPK manager
 */

#include "user_def_ipk.h"
#include <boost/thread/locks.hpp>

UserDefinedIPKManager::UserDefinedIPKManager(const std::wstring& szarp_data_dir, const std::wstring& szarp_system_dir, const std::wstring& language) {
	ipk = new UserDefinedIPKContainer(szarp_data_dir, szarp_system_dir, language, this);
}

bool UserDefinedIPKManager::PreloadConfig(const std::wstring &prefix) {
	boost::upgrade_lock<boost::shared_mutex> lock(mutex);
	if (configs_ready_for_load.get(prefix) != nullptr)
		return true;

	boost::upgrade_to_unique_lock<boost::shared_mutex> unique(lock);

	auto config = ipk->InitializeIPK(prefix);
	if (!config)
		return false;

	configs_ready_for_load.insert(prefix, config);
	return true;
}

TSzarpConfig* UserDefinedIPKManager::PopConfig(const std::wstring &prefix) {
	TSzarpConfig* config = configs_ready_for_load.get(prefix);
	if (config) {
		configs_ready_for_load.remove(prefix);
	}
	return config;
}

void UserDefinedIPKManager::AddUserDefined(const std::wstring& prefix, TParam *param) {
	boost::unique_lock<boost::shared_mutex> lock(mutex);
	if (ipk->AddUserDefined(prefix, param)) {
		m_extra_params[prefix].emplace_back(param);
	}
}

void UserDefinedIPKManager::RemoveUserDefined(const std::wstring& prefix, TParam *param) {
	boost::unique_lock<boost::shared_mutex> lock(mutex);
	if (ipk->RemoveUserDefined(prefix, param)) {
		auto& tp = m_extra_params[prefix];
		auto ei = std::find_if(tp.begin(), tp.end(), 
			[param] (std::shared_ptr<TParam> &_p) { return _p.get() == param; }
		);

		if (ei != tp.end()) {
			tp.erase(ei);
		}
	}
}

bool UserDefinedIPKContainer::AddUserDefined(const std::wstring& prefix, TParam *n) {
	boost::unique_lock<boost::shared_mutex> lock(ParamCachingIPKContainer::m_lock);

	TSzarpConfig* config;
	if (!(config = ParamCachingIPKContainer::configs.get(prefix))) {
		return false;
	}

	n->SetParentSzarpConfig(config);
	cacher.AddParam(n);

	/* update param's id in config cache */
	/* TODO: move this to TSzarpConfig or separate function */
	ConfigAux& aux = ParamCachingIPKContainer::config_aux[prefix];
	if (aux._freeIds.size()) {
		std::set<unsigned>::iterator i = aux._freeIds.begin();
		n->SetParamId(*i);
		aux._freeIds.erase(i);
	} else {
		n->SetParamId(aux._maxParamId++);
	}

	return true;
}

bool UserDefinedIPKContainer::RemoveUserDefined(const std::wstring& prefix, TParam *p) {
	boost::unique_lock<boost::shared_mutex> lock(ParamCachingIPKContainer::m_lock);

	if (!cacher.RemoveParam(p)) {
		return false;
	}

	/* update param's id in config cache */
	/* TODO: move this to TSzarpConfig or separate function */
	ConfigAux& aux = ParamCachingIPKContainer::config_aux[prefix];
	if (p->GetParamId() + 1 == aux._maxParamId)
		aux._maxParamId--;
	else
		aux._freeIds.insert(p->GetParamId());

	return true;
}

std::vector<std::shared_ptr<TParam>>& UserDefinedIPKManager::GetUserDefinedParams(const std::wstring& prefix) {
	return m_extra_params[prefix];
}

UserDefinedIPKContainer::UserDefinedIPKContainer(const std::wstring& szarp_data_dir, const std::wstring& szarp_system_dir, const std::wstring &language, UserDefinedIPKManager* _ipk_manager): ParamCachingIPKContainer(szarp_data_dir, szarp_system_dir, language), manager(_ipk_manager) {
	assert(IPKContainer::_object == nullptr);
	IPKContainer::_object = this;
}

TSzarpConfig* UserDefinedIPKContainer::AddConfig(const std::wstring& prefix) {

	manager->Lock();

	auto config = manager->PopConfig(prefix);
	if (config) {
		config = AddConfig(config, prefix);
	} else {
		config = ParamCachingIPKContainer::AddConfig(prefix);
	}

	manager->Release();
	return config;
}

TSzarpConfig* UserDefinedIPKContainer::AddConfig(TSzarpConfig* config, const std::wstring& prefix) {
	// No need to lock manager as this version of AddConfig will be called from prefix-only

	ParamCachingIPKContainer::AddConfig(config, prefix);
	// manager->UpdateConfig(ipk, prefix);

	auto& ca = config_aux[prefix];
	unsigned id = ca._maxParamId;
	for (auto p: manager->GetUserDefinedParams(prefix)) {
		p->SetParamId(id++);
		p->SetParentSzarpConfig(config);
		p->SetConfigId(ca._configId);
		cacher.AddParam(p.get());
	}

	return config;
}

