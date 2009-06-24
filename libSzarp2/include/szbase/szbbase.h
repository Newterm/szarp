/* 
  SZARP: SCADA software 

*/
/*
 * szbase - szbdbase.h - main public interfaces
 * $Id$
 * <pawel@praterm.com.pl>
 */

#ifndef __SZBBASE_H__
#define __SZBBASE_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "szbdefines.h"
#include "szbname.h"
#include "szbdate.h"
#include "szbbuf.h"
#include "szbfile.h"
#include <szbase/szbfilewatcher.h>

#include <tr1/unordered_map>
#include <string>
#include <stack>
#include <utility>
#include <exception>

#ifndef NO_LUA
#include <lua.hpp>
#endif

#include <string>
#include <utility>
#include <exception>

#include <boost/filesystem/path.hpp>
#include <boost/thread/recursive_mutex.hpp>

typedef boost::recursive_mutex::scoped_lock recursive_scoped_lock;

#ifndef NO_LUA
bool szb_compile_lua_formula(lua_State *lua, const char *formula, const char *formula_name = "param_fomula");
#endif


class Szbase {
	typedef std::tr1::unordered_map<std::wstring, std::pair<szb_buffer_t*, TParam*> > TBP;

	class UnsignedStringHash {
		std::tr1::hash<std::string> m_hasher;
		public:
		size_t operator() (const std::basic_string<unsigned char>& v) const {
			return m_hasher((const char*) v.c_str());
		}
	};

	typedef std::tr1::unordered_map<std::basic_string<unsigned char>, std::pair<szb_buffer_t*, TParam*>, UnsignedStringHash > TBPU8;
	typedef std::tr1::unordered_map<std::wstring, std::pair<szb_buffer_t*, TSzarpConfig*> > TBI;
	TBP m_params;
	TBPU8 m_u8params;
	TBI m_ipkbasepair;
	boost::filesystem::wpath m_szarp_dir;
	SzbFileWatcher m_file_watcher;
	void (*m_config_modification_callback)(std::wstring, std::wstring);

	bool AddBase(const std::wstring& prefix);

	bool FindParam(const std::basic_string<unsigned char>& param, std::pair<szb_buffer_t*, TParam*>& bp);
	bool FindParam(const std::wstring& param, std::pair<szb_buffer_t*, TParam*>& bp);

	static Szbase* _instance;
	Szbase(const std::wstring& szarp_dir);
	~Szbase();

	void AddParamToHash(const std::wstring& prefix, TParam *param);

	boost::recursive_mutex szbase_mutex;

	long m_current_query;
public:
	void NotifyAboutConfigurationChanges();
	time_t SearchFirst(const std::wstring& param, bool &ok);
	time_t SearchFirst(const std::basic_string<unsigned char> & param, bool &ok);
	time_t SearchLast(const std::wstring& param, bool &ok);
	time_t SearchLast(const std::basic_string<unsigned char>& param, bool &ok);
	void AddExtraParam(const std::wstring& prefix, TParam *param);
	void RemoveExtraParam(const std::wstring& prefix, TParam *param);
	bool AddBase(const std::wstring& szbase_dir, const std::wstring &prefix);
	double GetValue(const std::wstring& param, time_t time, SZARP_PROBE_TYPE probe_type, int custom_length, bool *isFixed, bool &ok, std::wstring &error);
	double GetValue(const std::basic_string<unsigned char>& param, time_t time, SZARP_PROBE_TYPE probe_type, int custom_length, bool *isFixed, bool &ok, std::wstring &error);
	double GetValue(std::pair<szb_buffer_t*, TParam*>& bp, time_t time, SZARP_PROBE_TYPE probe_type, int custom_length, bool *isFixed, bool &ok, std::wstring &error);
#ifndef NO_LUA
	bool CompileLuaFormula(const std::wstring& formula, std::wstring& error);
#endif
	static void Init(const std::wstring& szarp_dir, bool write_cache = false);
	static void Init(const std::wstring& szarp_dir, void (*callback)(std::wstring, std::wstring), bool write_cache);
	static void Destroy();
	szb_buffer_t* GetBuffer(const std::wstring& prefix);
	static Szbase* GetObject();
	std::wstring GetCacheDir(const std::wstring& prefix);
	void ClearCacheDir(const std::wstring& prefix);
	void ClearParamFromCache(const std::wstring& prefix, TParam* param);
	void RemoveConfig(const std::wstring& prefix);
	const long& GetQueryId() { return m_current_query; }
	void NextQuery();
};

#ifndef NO_LUA
class Lua {
	static lua_State* lua;
	Lua();
	
public:
	static void Init();
	static lua_State* GetInterpreter();
	static std::stack<bool> fixed;
};
#endif

#endif

