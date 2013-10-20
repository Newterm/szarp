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
#ifndef __DATABASE_H__
#define __DATABASE_H__
#ifndef WX_PRECOMP
#include <wx/wx.h>
#include <wx/thread.h>
#else
#include <wx/wxprec.h>
#endif

#include <list>
#include <ids.h>
#include <boost/thread/thread.hpp>
#include <tr1/tuple>

#include "szbase/szbbase.h"
#include "sz4/base.h"


/**Query to the database*/
struct DatabaseQuery {

	/**Type of query*/
	enum QueryType {
		/*Search query*/
		SEARCH_DATA,
		/*Data retrieval query*/
		GET_DATA,
		/*Cleaning*/
		CLEANER,
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
	};

	/**Parameters of a query for data*/
	struct ValueData {
		/**Type of period we are asking for*/
		PeriodType period_type;
		struct V {
			/**Start time of data*/
			time_t time;
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
		};
		std::vector<V> *vv;
		bool refresh;
	};

	/**Parameters of a search query*/
	struct SearchData {
		/**Range of search*/
		time_t start, end;
		/**Direction of search*/
		int direction;
		/**Response*/
		time_t response;
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
public:
	virtual void RemoveConfig(const std::wstring& prefix,
			bool poison_cache) = 0;

	virtual bool CompileLuaFormula(const std::wstring& formula, std::wstring& error) = 0;

	virtual void AddExtraParam(const std::wstring& prefix, TParam *param) = 0;

	virtual void RemoveExtraParam(const std::wstring& prefix, TParam *param) = 0;

	virtual void NotifyAboutConfigurationChanges() = 0;

	virtual void SetProberAddress(const std::wstring& prefix,
			const std::wstring& address,
			const std::wstring& port) = 0 ;

	virtual void ExtractParameters(DatabaseQuery::ExtractionParameters &pars) = 0;

	virtual void SearchData(DatabaseQuery* query) = 0;

	virtual void GetData(DatabaseQuery* query, wxEvtHandler *response_receiver) = 0;

	virtual void ResetBuffer(DatabaseQuery* query) = 0;

	virtual void ClearCache(DatabaseQuery* query) = 0;

	virtual void StopSearch() = 0;

	virtual void RegisterObserver(sz4::param_observer* observer, const std::vector<TParam*>& params) = 0;

	virtual void DeregisterObserver(sz4::param_observer* observer, const std::vector<TParam*>& params) = 0;

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
	SzbaseBase(const std::wstring& data_path, void (*conf_changed_cb)(std::wstring, std::wstring), int cache_size);

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

	void ExtractParameters(DatabaseQuery::ExtractionParameters &pars) ;

	void SearchData(DatabaseQuery* query);

	void GetData(DatabaseQuery* query, wxEvtHandler *response_receiver);

	void ResetBuffer(DatabaseQuery* query);

	void ClearCache(DatabaseQuery* query);
	
	void StopSearch();

	void RegisterObserver(sz4::param_observer* observer, const std::vector<TParam*>& params) {};

	void DeregisterObserver(sz4::param_observer* observer, const std::vector<TParam*>& params) {};
};

class Sz4Base : public Draw3Base {
	sz4::base *base;

public:
	Sz4Base(const std::wstring& data_dir, IPKContainer* ipk_conatiner);

	~Sz4Base();	

	void RemoveConfig(const std::wstring& prefix,
			bool poison_cache) ;

	bool CompileLuaFormula(const std::wstring& formula, std::wstring& error) ;

	void AddExtraParam(const std::wstring& prefix, TParam *param) ;

	void RemoveExtraParam(const std::wstring& prefix, TParam *param) ;

	void NotifyAboutConfigurationChanges() ;

	void SetProberAddress(const std::wstring& prefix,
			const std::wstring& address,
			const std::wstring& port)  ;

	void ExtractParameters(DatabaseQuery::ExtractionParameters &pars) ;

	void SearchData(DatabaseQuery* query);

	void GetData(DatabaseQuery* query, wxEvtHandler *response_receiver);

	void ResetBuffer(DatabaseQuery* query);

	void ClearCache(DatabaseQuery* query);
	
	void StopSearch();

	void RegisterObserver(sz4::param_observer* observer, const std::vector<TParam*>& params);

	void DeregisterObserver(sz4::param_observer* observer, const std::vector<TParam*>& params);
};

/**Query execution thread*/
class QueryExecutor : public wxThread {
	/**Queue with pending queries*/
	DatabaseQueryQueue *queue;
	/**Object receiving responses*/
	wxEvtHandler *response_receiver;
	/**base object*/
	Draw3Base *base;

	/**Peforms a query for a parametr values
	 * @param base buffer that operation shall be performed upon */
	void ExecuteDataQuery(szb_buffer_t* szb, TParam *p, DatabaseQuery *q);

	/**Peforms a query for a data
	 * @param base buffer that operation shall be performed upon
	 * @param p ipk param to get data from
	 * @param sd output param @see DatabaseQuery::SearchData*/
	void ExecuteSearchQuery(szb_buffer_t *szb, TParam *p, DatabaseQuery::SearchData &sd);

	void ExecuteExtractParametersQuery(DatabaseQuery::ExtractionParameters &extract_parameters);
public:
	void StopSearch();

	QueryExecutor(DatabaseQueryQueue *_queue, wxEvtHandler *_response_receiver, Draw3Base *_base);
	/**Thread entry point. Executes queries*/
	virtual void* Entry();
};

typedef std::list<DatabaseQuery*>::iterator QueryCollectionIterator;

/**Queue holding pending queries*/
class DatabaseQueryQueue {

	/**Queue entry*/
	struct QueueEntry {
		/**@see DatabaseQuery*/
		DatabaseQuery *query;
		/**ranking of this entry, The greater the sooner the query will be executed*/
		double ranking;
	};

	/**Compares queries rangings*/
	static bool QueryCmp(const QueueEntry& q1, const QueueEntry& q2);

	/**Point to see @PanelManager*/
	DatabaseManager* database_manager;

	/**The queue implementation*/
	std::list<QueueEntry> queue;

	/**Semaphore counting number of elements in the queue*/
	wxSemaphore semaphore;

	/**Mutex guarding access to a queue*/
	wxMutex mutex;

	/**Number of elements in the queue preventing queries prioritisation*/
	int cant_prioritise_entries;

	/**Calculates ranking of the query*/
	double FindQueryRanking(DatabaseQuery* q);

public:
	DatabaseQueryQueue();

	/**Sets @see DatabaseManager object*/
	void SetDatabaseManager(DatabaseManager *manager);

	/**Recalculates priorities of entires in the queue*/
	void ShufflePriorities();

	/**Adds query to a queue*/
	void Add(DatabaseQuery *q);

	/**Adds query to a queue*/
	void Add(std::list<DatabaseQuery*> &qlist);

	/**Retrieves entry from the queue*/
	DatabaseQuery* GetQuery();

	/** Removes old queries form queue */
	void CleanOld(DatabaseQuery* q);

};


DatabaseQuery* CreateDataQuery(DrawInfo* di, PeriodType pt, int draw_no = -1);

void AddTimeToDataQuery(DatabaseQuery *q, time_t time);

SZARP_PROBE_TYPE PeriodToProbeType(PeriodType period);

#endif
