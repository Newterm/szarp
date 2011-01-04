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

#include "szbase/szbbase.h"

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
		SET_PROBER_ADDRESS
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

	/**Type of query*/
	QueryType type;

	/**DrawInfo this query refers to*/
	DrawInfo *draw_info;
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
	};

	~DatabaseQuery();

};

/**Query execution thread*/
class QueryExecutor : public wxThread {
	/**Queue with pending queries*/
	DatabaseQueryQueue *queue;
	/**Object receiving responses*/
	wxEvtHandler *response_receiver;
	/**Szbase object*/
	Szbase *szbase;

	boost::mutex m_mutex;

	SzbCancelHandle *cancelHandle;

	/**Peforms a query for a parametr values
	 * @param base buffer that operation shall be performed upon */
	void ExecuteDataQuery(szb_buffer_t* szb, TParam *p, DatabaseQuery *q);

	/**Peforms a query for a data
	 * @param base buffer that operation shall be performed upon
	 * @param p ipk param to get data from
	 * @param sd output param @see DatabaseQuery::SearchData*/
	void ExecuteSearchQuery(szb_buffer_t *szb, TParam *p, DatabaseQuery::SearchData &sd);

public:
	void StopSearch();

	QueryExecutor(DatabaseQueryQueue *_queue, wxEvtHandler *_response_receiver, Szbase *_szbase);
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
