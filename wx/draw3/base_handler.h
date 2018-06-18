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
#ifndef __BASE_HANDLER__
#define __BASE_HANDLER__
#ifndef WX_PRECOMP
#include <wx/wx.h>
#include <wx/thread.h>
#else
#include <wx/wxprec.h>
#endif

#include <mutex>
#include <list>
#include <future>
#include <map>
#include <ids.h>
#include <boost/thread/thread.hpp>
#include <tr1/tuple>
#include <unordered_map>

#include "szbase/szbbase.h"

class SzbExtractor;


/**Query to the database*/
struct DatabaseQuery {

	/**Type of query*/
	enum QueryType {
		/*Search query*/
		SEARCH_DATA,
		/*Data retrieval query*/
		GET_DATA,
		/**Reset buffer*/
		RESET_BUFFER,
		/**Clear cache*/
		CLEAR_CACHE,
		/**Synchronizing message, informs queue that config is to be reloaded*/
		STARTING_CONFIG_RELOAD,
		/**Performs formula compilation, returns compilation status*/
		COMPILE_FORMULA,
		/**Adds param to configuration*/
		ADD_PARAM,
		/**Removes param from configuration*/
		REMOVE_PARAM,
		/**Checks if any configuration has changed*/
		CHECK_CONFIGURATIONS_CHANGE,
		/**Set addresses of probers servers*/
		SET_PROBER_ADDRESS,
		/**Set addresses of probers servers*/
		EXTRACT_PARAM_VALUES,
		/**Set addresses of probers servers*/
		REGISTER_OBSERVER,
		/**Add new base prefix to handler */
		ADD_BASE_PREFIX,
	};

	/**Parameters of a query for data*/
	struct ValueData {
		/**Type of period we are asking for*/
		PeriodType period_type;
		struct V {
			/**Start time of data*/
			time_t time_second;
			time_t time_nanosecond;
			/**Custom porobe length*/
			int custom_length;
			/**Response from a database*/
			double response;
			/**First val*/
			double first_val;
			/**Last val*/
			double last_val;
			/**Sum of probes*/
			double sum;
			/**Number of no no-data probes*/
			int count;
			/** False if error ocurred during data retrieval*/
			bool ok;
			/** error no*/	
			int error;
			/** Error string*/
			wchar_t *error_str;
			/** Fixed value, i.e. it's not gonna change in the future, 
			    no need to ask for it */
			bool fixed;
		};
		std::vector<V> *vv;
		bool refresh;
	};

	/**Parameters of a search query*/
	struct SearchData {
		/**Range of search*/
		time_t start_second, start_nanosecond;
		time_t end_second, end_nanosecond;
		/**Direction of search*/
		int direction;
		/**Response*/
		time_t response_second;
		time_t response_nanosecond;
		/**Type of probe we searching for (needed for LUA params)*/
		PeriodType period_type;
		/** False if error ocurred during data search*/
		bool ok;
		/** error no*/	
		int error;
		/** Error string*/
		wchar_t *error_str;
		/** Search condtion*/
		szb_search_condition *search_condition;
	};

	/**Name of configuration prefix to be reloaded*/
	struct ConfigReloadPrefix {
		wchar_t * prefix;
	};

	struct CompileFormula {
		wchar_t* formula;
		wchar_t* error;
		bool ok;
	};

	struct DefinedParamData {
		TParam *p;
		wchar_t *prefix;
	};

	struct ProberAddress {
		wchar_t *address;
		wchar_t *port;
	};

	struct ExtractionParameters {
		time_t start_time;
		time_t end_time;
		PeriodType pt;
		std::vector<std::wstring>* params;
		std::vector<std::wstring>* prefixes;
		std::wstring* file_name;
	};

	struct ObserverRegistrationParameters {
		sz4::param_observer* observer;
		std::vector<TParam*>* params_to_register;
		std::vector<TParam*>* params_to_deregister;
	};

	/**Type of query*/
	QueryType type;

	/**DrawInfo this query refers to*/
	DrawInfo *draw_info;
	/**IPK param this query refers to*/
	TParam *param;
	/** prefix of query*/
	std::wstring prefix;

	size_t q_id;

	/**Id of panel asking for data*/
	InquirerId inquirer_id;
	/**Number of draw in panel*/
	int draw_no;

	union {
		ValueData value_data;
		SearchData search_data;
		ConfigReloadPrefix reload_prefix;
		CompileFormula compile_formula;
		DefinedParamData defined_param;
		ProberAddress prober_address;
		ExtractionParameters extraction_parameters;
		ObserverRegistrationParameters observer_registration_parameters;
	};

	~DatabaseQuery();

};


class Draw3Base {
protected:
	wxEvtHandler *m_response_receiver;

	virtual void RemoveConfig(const std::wstring& prefix, bool poison_cache) = 0;

	virtual void NotifyAboutConfigurationChanges() = 0;

	virtual bool CompileLuaFormula(const std::wstring& formula, std::wstring& error) = 0;

	virtual void AddExtraParam(const std::wstring& prefix, TParam *param) = 0;

	virtual void SetProberAddress(const std::wstring& prefix,
			const std::wstring& address,
			const std::wstring& port);

	virtual void RemoveExtraParam(const std::wstring& prefix, TParam *param) = 0;
public:
	using ptr = std::shared_ptr<Draw3Base>;

	Draw3Base(wxEvtHandler* response_receiver);

	virtual void RemoveConfig(DatabaseQuery *query);

	virtual void CompileLuaFormula(DatabaseQuery *query);

	virtual void AddExtraParam(DatabaseQuery *query);

	virtual void RemoveExtraParam(DatabaseQuery *query);

	virtual void NotifyAboutConfigurationChanges(DatabaseQuery *query);

	virtual void SetProberAddress(DatabaseQuery* query);

	void ExtractParameters(DatabaseQuery *query);

	virtual SzbExtractor* CreateExtractor() = 0;

	virtual void SearchData(DatabaseQuery* query) = 0;

	virtual void GetData(DatabaseQuery* query) = 0;

	virtual void ResetBuffer(DatabaseQuery* query) = 0;

	virtual void ClearCache(DatabaseQuery* query) = 0;

	virtual void StopSearch() = 0;

	virtual void RegisterObserver(DatabaseQuery* query);

	virtual ~Draw3Base() {}
};

class SzbaseBase : public Draw3Base {
	Szbase *szbase;

	boost::mutex m_mutex;

	SzbCancelHandle *cancelHandle;

	std::tr1::tuple<TSzarpConfig*, szb_buffer_t*> GetConfigAndBuffer(TParam *param);
	
	void maybeSetCancelHandle(TParam* param);
	void releaseCancelHandle(TParam* param);
public:
	SzbaseBase(wxEvtHandler* response_receiver, const std::wstring& data_path, void (*conf_changed_cb)(std::wstring, std::wstring), int cache_size);

	~SzbaseBase();	

	void RemoveConfig(const std::wstring& prefix,
			bool poison_cache) ;

	bool CompileLuaFormula(const std::wstring& formula, std::wstring& error) ;

	void AddExtraParam(const std::wstring& prefix, TParam *param) ;

	void RemoveExtraParam(const std::wstring& prefix, TParam *param) ;

	void NotifyAboutConfigurationChanges() ;

	void SetProberAddress(const std::wstring& prefix,
			const std::wstring& address,
			const std::wstring& port)  ;

	SzbExtractor* CreateExtractor();

	void SearchData(DatabaseQuery* query);

	void GetData(DatabaseQuery* query);

	void ResetBuffer(DatabaseQuery* query);

	void ClearCache(DatabaseQuery* query);
	
	void StopSearch();

};

class Sz4Base : public Draw3Base {
	sz4::base *base;
	std::wstring data_dir;
	IPKContainer* ipk_container;

	template<class time_type> void GetValue(DatabaseQuery::ValueData::V& v,
			time_t second, time_t nanosecond, TParam* p, SZARP_PROBE_TYPE pt);
public:
	Sz4Base(wxEvtHandler* response_receiver, const std::wstring& data_path, IPKContainer* ipk_container);

	~Sz4Base();	

	void RemoveConfig(const std::wstring& prefix,
			bool poison_cache) ;

	bool CompileLuaFormula(const std::wstring& formula, std::wstring& error) ;

	void AddExtraParam(const std::wstring& prefix, TParam *param) ;

	void RemoveExtraParam(const std::wstring& prefix, TParam *param) ;

	void NotifyAboutConfigurationChanges() ;

	SzbExtractor* CreateExtractor();

	void SearchData(DatabaseQuery* query);

	void GetData(DatabaseQuery* query);

	void ResetBuffer(DatabaseQuery* query);

	void ClearCache(DatabaseQuery* query);
	
	void StopSearch();

	virtual void RegisterObserver(DatabaseQuery* query);
};

namespace sz4 {
	class iks;
	class connection_mgr;
}

namespace boost { namespace asio {
	class io_service;
}}


class Sz4ApiBase : public Draw3Base {
private:
	class ObserverWrapper : public sz4::param_observer_ {
		sz4::param_observer* obs;
		TParam *param;
		std::wstring prefix;
		int version;
	public:
		ObserverWrapper(TParam* param, sz4::param_observer* obs);
		virtual void operator()();
	};

	std::shared_ptr<boost::asio::io_service> io;
	std::shared_ptr<sz4::connection_mgr> connection_mgr;
	std::shared_ptr<sz4::iks> base;
	std::unique_ptr<sz4::lua_interpreter<sz4::iks>> m_interpreter;
	std::mutex lua_mutex;
	IPKContainer* ipk_container;
	boost::thread io_thread;

	std::future<void> connection_cv;

	std::map<std::pair<sz4::param_observer*, sz4::param_info>, std::shared_ptr<ObserverWrapper>> observers;

	sz4::param_info ParamInfoFromParam(TParam* p);

	template<class time_type> void DoGetData(DatabaseQuery *query);
public:

	Sz4ApiBase(wxEvtHandler* response_receiver,
			const std::wstring& address, const std::wstring& port,
			IPKContainer *ipk_conatiner);

	~Sz4ApiBase();	

	bool BlockUntilConnected(const unsigned int timeout_s = 10);

	void RemoveConfig(const std::wstring& prefix,
			bool poison_cache) ;

	bool CompileLuaFormula(const std::wstring& formula, std::wstring& error) ;

	void AddExtraParam(const std::wstring& prefix, TParam *param) ;

	void RemoveExtraParam(const std::wstring& prefix, TParam *param) ;

	void NotifyAboutConfigurationChanges() ;

	void SetProberAddress(const std::wstring& prefix,
			const std::wstring& address,
			const std::wstring& port)  ;

	SzbExtractor* CreateExtractor();

	void SearchData(DatabaseQuery* query);

	void GetData(DatabaseQuery* query);

	void ResetBuffer(DatabaseQuery* query);

	void ClearCache(DatabaseQuery* query);
	
	void StopSearch();

	void RegisterObserver(DatabaseQuery* query);
};

class BaseHandler {
public:
	using ptr = std::shared_ptr<BaseHandler>;
	virtual void SetCurrentPrefix(const wxString& prefix) = 0;
	virtual wxString GetCurrentPrefix() const = 0;
	virtual Draw3Base::ptr GetBaseHandler(const std::wstring& prefix) = 0;
	virtual void AddBasePrefix(const wxString&) = 0;
	virtual ~BaseHandler() {}
};

class SzbaseHandler : public BaseHandler {
public:
	SzbaseHandler(wxEvtHandler *rr) : response_receiver(rr) {}
	void SetupHandlers(const wxString& szarp_dir, const wxString& szarp_data_dir, int cache_size);

	void SetDefaultBaseHandler(Draw3Base::ptr d)
	{ default_base_handler = d; }

	void SetCurrentPrefix(const wxString& prefix) override
	{ current_prefix = prefix; }

	wxString GetCurrentPrefix() const override
	{ return current_prefix; }

	Draw3Base::ptr GetBaseHandler(const std::wstring& prefix) override;

	Draw3Base::ptr GetIksHandlerFromBase(const wxString& prefix);
	Draw3Base::ptr GetIksHandler(const wxString& iks_server, const wxString& iks_port);

	Draw3Base::ptr GetSz4Handler()
	{ return sz4_base_handler; }
	Draw3Base::ptr GetSz3Handler()
	{ return sz3_base_handler; }

	void AddBasePrefix(const wxString& prefix) override;
	void AddBaseHandler(const wxString& prefix, Draw3Base::ptr handler)
	{ base_handlers[prefix] = handler; }

	void UseIksServerOnly()
	{ use_iks = true; }

private:
	void ConfigLibpar(const wxString&);

	std::recursive_mutex libpar_mutex;
	Draw3Base::ptr default_base_handler{nullptr};
	Draw3Base::ptr sz4_base_handler{nullptr};
	Draw3Base::ptr sz3_base_handler{nullptr};

	wxString base_path;
	wxString current_prefix;

	wxEvtHandler *response_receiver{nullptr};
	std::map<wxString, Draw3Base::ptr> base_handlers;
	bool use_iks{false};
};

#endif //__BASE_HANDLER__
