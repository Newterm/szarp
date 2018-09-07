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

#include "ipkcontainer.h"

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
#include <set>
#include <utility>
#include <exception>

#include <boost/filesystem/path.hpp>
#include <boost/thread/recursive_mutex.hpp>

typedef boost::recursive_mutex::scoped_lock recursive_scoped_lock;

class SzbParamObserver;
class SzbParamMonitor;

/** Highest level szbase API, provides access to szbase content
 This class should be initialized by calling one of Init static method.
 Also before this class can be used @see IPKContiner should be initialized.
*/
class Szbase {
	std::vector<szb_buffer_t*> m_szb_buffers;

	/**Stores address of probes servers for particular prefixes.*/
	std::tr1::unordered_map<std::wstring, std::pair<std::wstring, std::wstring> >
		m_probers_addresses;
	/** Path to root szarp dir*/
	boost::filesystem::wpath m_szarp_dir;
	/** Object used for monitoring updates of params.xml files*/
	SzbFileWatcher m_file_watcher;

	void (*m_config_modification_callback)(std::wstring, std::wstring);
#ifndef NO_LUA
#if LUA_PARAM_OPTIMISE
	/**Object keeps track of references between lua optimized params*/
	std::map<TParam*, std::set<TParam*> > m_lua_opt_param_reference_map;
#endif
#endif
	/**Current 'transaction id' number. Blocks from @see szb_buffer_t 
	held by this object are refreshed only when their last_current_id is 
	different than this value (refresh operation causes their @see last_current_id
	to be set to m_current_query). The intention of this mechanism is to
	give client apps explicit control over data refresh.*/
	long m_current_query;
	
	/**value passed to @see szb_buffer_t upon buffer constrution*/
	int m_buffer_cache_size;

	/**maximum serach time*/
	time_t m_maximum_search_time;

	SzbParamMonitor* m_monitor;

	/**Load configuration for this prefix into internal hashes*/
	bool AddBase(const std::wstring& prefix);

	IPKContainer* m_ipk_containter;

	static Szbase* _instance;

	Szbase(const std::wstring& szarp_dir);

	~Szbase();

	std::map<std::wstring, std::set<TParam*> > m_extra_params;

	void AddParamToHash(const std::wstring& prefix, TParam *param);
public:
	/**Notifies listener about changes in configuration*/
	void NotifyAboutConfigurationChanges();

	szb_buffer_t* GetBufferForParam(TParam* p, bool add_if_not_present = true);

	time_t SearchFirst(const std::wstring& param, bool &ok);
	time_t SearchFirst(const std::basic_string<unsigned char> & param, bool &ok);
	time_t SearchLast(const std::wstring& param, bool &ok);
	time_t SearchLast(const std::basic_string<unsigned char>& param, bool &ok);
	time_t Search(const std::wstring& param, time_t start, time_t end, int direction, SZARP_PROBE_TYPE probe, bool &ok, std::wstring& error);
	/**Add user defined param to object*/
	void AddExtraParam(const std::wstring& prefix, TParam *param);
#ifndef NO_LUA
#if LUA_PARAM_OPTIMISE
	void AddLuaOptParamReference(TParam* refered, TParam* referring);
#endif
#endif
	/**Remove user param from object, cache associated with param is destroyed*/
	void RemoveExtraParam(const std::wstring& prefix, TParam *param);
	/**Add configuration to object*/
	bool AddBase(const std::wstring& szbase_dir, const std::wstring &prefix);
	/**Methods for retrieveing data from szbase
	* @param param name of param
	* @param time time of value to retrieve
	* @param probe_type type of probe to retrieve
	* @param custom_length if probe type is PERIOT_T_OTHER, this value denotes number of 10 minute values to average over
	* @param isFixed output parameter, denotes is this probe vlaue is fixed i.e. can be cached
	* @param ok flag indicate if there was an error in retrieveing this value
	* @param error if ok is set to false, this output params holds error string*/
	double GetValue(const std::wstring& param, time_t time, SZARP_PROBE_TYPE probe_type, int custom_length, bool *isFixed, bool &ok, std::wstring &error);
	double GetValue(const std::basic_string<unsigned char>& param, time_t time, SZARP_PROBE_TYPE probe_type, int custom_length, bool *isFixed, bool &ok, std::wstring &error);
	double GetValue(std::pair<szb_buffer_t*, TParam*>& bp, time_t time, SZARP_PROBE_TYPE probe_type, int custom_length, bool *isFixed, bool &ok, std::wstring &error);
#ifndef NO_LUA
	/**Compiles lua formula*/
	bool CompileLuaFormula(const std::wstring& formula, std::wstring& error);
#endif
	void AddParamMonitor(SzbParamObserver *observer, const std::vector<TParam*>& params);
	/**Static methods for initizaling global object*/
	static void Init(const std::wstring& szarp_dir, bool write_cache = false);
	static void Init(const std::wstring& szarp_dir, void (*callback)(std::wstring, std::wstring), bool write_cache, int memory_cache_size = 0);
	/**Destroy all object contents*/
	static void Destroy();
	/**Get buffer for give prefix*/
	szb_buffer_t* GetBuffer(const std::wstring& prefix, bool add_if_not_present = true);
	static Szbase* GetObject();
	std::wstring GetCacheDir(const std::wstring& prefix);
	/**Find param with given global name*/
	bool FindParam(const std::basic_string<unsigned char>& param, std::pair<szb_buffer_t*, TParam*>& bp);
	/**Find param with given wchar_r version*/
	bool FindParam(const std::wstring& param, std::pair<szb_buffer_t*, TParam*>& bp);
	/**Clear whole cache for given prefix*/
	void ClearCacheDir(const std::wstring& prefix);
	/**Clears param from cache*/
	void ClearParamFromCache(const std::wstring& prefix, TParam* param);
	/**Remove all parameters from configuration of prefix from object
	@param prefix prefix to remove
	@param poison_cache if true, cache for this prefix will be removed*/
	void RemoveConfig(const std::wstring& prefix, bool poison_cache);
	const long& GetQueryId() { return m_current_query; }

	/**sets prober address for given prefix*/
	void SetProberAddress(std::wstring prefix, std::wstring address, std::wstring port);
	/**retrives prober address for given prefix*/
	bool GetProberAddress(std::wstring prefix, std::wstring& address, std::wstring& port);

	/**advances @see m_current_query id*/
	void NextQuery();

	/**@return maximum amount of time that  serach operation is allowed to last*/
	int GetMaximumSearchTime() const;

	/**sets maximum amount of time that serach operation is allowed to last*/
	void SetMaximumSerachTime(int search_time);
};

#ifndef NO_LUA
/*Global object wrapping lua interpreter used internally by szabse*/
class Lua {
	static lua_State* lua;
	Lua();
	
public:
	static void Init();
	static lua_State* GetInterpreter();
	static std::stack<bool> fixed;
};

int lua_szbase_hoursum(lua_State *lua);

#endif

#endif

