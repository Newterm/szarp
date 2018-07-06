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

#ifndef __DEF_IPK_MGR__
#define __DEF_IPK_MGR__

#include "ipkcontainer.h"

#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>

class UserDefinedIPKManager {
private:
	// in c++17 use std::shared_mutex
	boost::shared_mutex mutex;
	std::unordered_map<std::wstring, std::vector<std::shared_ptr<TParam>>> m_extra_params;

	/** Preloaded configurations (prefix - config) */
	// std::unordered_map<std::wstring, TSzarpConfig> configs_ready_for_load;
	Cache<std::wstring, TSzarpConfig> configs_ready_for_load;

	/** Alias for global IPK */
	class UserDefinedIPKContainer* ipk;
	
public:
	UserDefinedIPKManager(const std::wstring& szarp_data_dir, const std::wstring& szarp_system_dir, const std::wstring& language);

	UserDefinedIPKContainer* GetIPKContainer() { return ipk; }

	/* used by draw3 to verify configurations */
	bool PreloadConfig(const std::wstring &prefix);
	TSzarpConfig* PopConfig(const std::wstring &prefix);

	void AddUserDefined(const std::wstring& prefix, TParam *param);
	void RemoveUserDefined(const std::wstring& prefix, TParam *param);
	std::vector<std::shared_ptr<TParam>>& GetUserDefinedParams(const std::wstring& prefix);

	/** Synchronise from outside */
	/** multibase inheritance will work aswell */
	void Lock() { mutex.lock(); }
	void Release() { mutex.unlock(); }
};

/** Synchronized IPKs container able to handle user defined params */
class UserDefinedIPKContainer: public ParamCachingIPKContainer {
private:
	UserDefinedIPKManager* manager;

private:
	/** On config load add existing defined params */
	TSzarpConfig* AddConfig(TSzarpConfig* ipk, const std::wstring& prefix) override;

	/** Check if config was preloaded */
	TSzarpConfig* AddConfig(const std::wstring& prefix) override;

public:
	bool AddUserDefined(const std::wstring& prefix, TParam *param);
	bool RemoveUserDefined(const std::wstring& prefix, TParam *param);

	UserDefinedIPKContainer(const std::wstring& szarp_data_dir, const std::wstring& szarp_system_dir, const std::wstring &language, UserDefinedIPKManager* _ipk_manager);

};

#endif
