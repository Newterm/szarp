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
#include <future>
#include <ids.h>
#include <boost/thread/thread.hpp>
#include <tr1/tuple>
#include <unordered_map>

#include "szbase/szbbase.h"
#include "sz4/base.h"
#include "sz4_iks_param_info.h"
#include "sz4_iks_param_observer.h"
#include "wx_exceptions.h"
#include "base_handler.h"


/**Query execution thread*/
class QueryExecutor : public wxThread {
	/**Queue with pending queries*/
	DatabaseQueryQueue *queue;
	/**Object receiving responses*/
	wxEvtHandler *response_receiver;
	/**base object*/
	BaseHandler::ptr base_handler;

public:
	QueryExecutor(DatabaseQueryQueue *_queue, wxEvtHandler *_response_receiver, BaseHandler::ptr _base_handler);
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


	/**Mutex + condition_variable guarding access to a queue*/
	std::mutex queue_mutex;
	std::condition_variable queue_cv;

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

	/** Removes old queries from queue */
	void CleanOld(DatabaseQuery* q);
};

DatabaseQuery* CreateDataQuery(DrawInfo* di, PeriodType pt, int draw_no = -1);

DatabaseQuery::ValueData::V& AddTimeToDataQuery(DatabaseQuery *q, const wxDateTime& time);

SZARP_PROBE_TYPE PeriodToProbeType(PeriodType period);

void ToNanosecondTime(const wxDateTime& time, time_t& second, time_t &nanosecond);

wxDateTime ToWxDateTime(time_t second, time_t nanosecond);

#endif
