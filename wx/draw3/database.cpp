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

#include "cconv.h"

#include "classes.h"
#include "szarp_config.h"
#include "database.h"
#include "cfgmgr.h"
#include "dbmgr.h"
#include "conversion.h"
#include "szbextr/extr.h"
#include "sz4/api.h"
#include "sz4/util.h"
#include "sz4/exceptions.h"
#include "szarp_base_common/lua_utils.h"
#include "ikssetup.h"
#include "sz4_iks_param_info.h"
#include "sz4_iks.h"

#include <cmath>

#include <algorithm>
#include <limits>

QueryExecutor::QueryExecutor(DatabaseQueryQueue *_queue, wxEvtHandler *_response_receiver, BaseHandler::ptr _base_handler) :
	wxThread(wxTHREAD_JOINABLE), queue(_queue), response_receiver(_response_receiver), base_handler(_base_handler)
{ 
}

SZARP_PROBE_TYPE PeriodToProbeType(PeriodType period) {
	SZARP_PROBE_TYPE pt;
	switch (period) {
		case PERIOD_T_DECADE:
			pt = PT_YEAR;
			break;
		case PERIOD_T_YEAR:
			pt = PT_MONTH;
			break;
		case PERIOD_T_MONTH:
			pt = PT_DAY;
			break;
		case PERIOD_T_WEEK:
			pt = PT_HOUR8;
			break;
		case PERIOD_T_SEASON:
			pt = PT_WEEK;
			break;
		case PERIOD_T_DAY:
			pt = PT_MIN10;
			break;
		case PERIOD_T_30MINUTE:
			pt = PT_SEC10;
			break;
		case PERIOD_T_5MINUTE:
			pt = PT_SEC;
			break;
		case PERIOD_T_MINUTE:
			pt = PT_HALFSEC;
			break;
		case PERIOD_T_30SEC:
			pt = PT_MSEC10;
			break;
		default:
			pt = PT_CUSTOM;
			assert (0);
	}

	return pt;
}

void ToNanosecondTime(const wxDateTime& time, time_t& second, time_t& nanosecond) {
	if (time.IsValid()) {
		second = time.GetTicks();
		nanosecond = time.GetMillisecond() * 1000000;
	} else {	
		second = -1;
		nanosecond = -1;
	}
}

wxDateTime ToWxDateTime(time_t second, time_t nanosecond) {
	if (second == (time_t) -1 && nanosecond == (time_t) -1)
		return wxInvalidDateTime;
	else {
		wxDateTime t((time_t)second);
		t.SetMillisecond(nanosecond / 1000000);
		return t;
	}
}

void* QueryExecutor::Entry() {
	DatabaseQuery *q = NULL;
	std::wstring last_prefix;
	while ((q = queue->GetQuery())) {
		if(q->type == DatabaseQuery::ADD_BASE_PREFIX) {
			base_handler->AddBasePrefix(wxString(q->prefix));
			delete q;
			continue;
		}
		if( q->type == DatabaseQuery::REGISTER_OBSERVER ) {
			q->prefix = last_prefix;
		}
		last_prefix = q->prefix;
		auto base = base_handler->GetBaseHandler(q->prefix);
		if(!base) {
			delete q;
			continue;
		}
		switch (q->type) {
			case DatabaseQuery::STARTING_CONFIG_RELOAD:
				base->RemoveConfig(q);
				break;

			case DatabaseQuery::COMPILE_FORMULA:
				base->CompileLuaFormula(q);
				break;

			case DatabaseQuery::ADD_PARAM:
				base = base_handler->GetBaseHandler(q->defined_param.prefix);
				base->AddExtraParam(q);
				break;

			case DatabaseQuery::REMOVE_PARAM:
				base->RemoveExtraParam(q);
				delete q;
				break;

			case DatabaseQuery::CHECK_CONFIGURATIONS_CHANGE:
				base->NotifyAboutConfigurationChanges(q);
				break;

			case DatabaseQuery::SET_PROBER_ADDRESS:
				base->SetProberAddress(q);
				break;

			case DatabaseQuery::EXTRACT_PARAM_VALUES:
				base->ExtractParameters(q);
				delete q;
				break;

			case DatabaseQuery::SEARCH_DATA:
				base->SearchData(q);
				break;

			case DatabaseQuery::GET_DATA:
				base->GetData(q);
				break;

			case DatabaseQuery::RESET_BUFFER:
				base->ResetBuffer(q);
				delete q;
				break;

			case DatabaseQuery::CLEAR_CACHE:
				base->ClearCache(q);
				delete q;
				break;

			case DatabaseQuery::REGISTER_OBSERVER:
				base->RegisterObserver(q);
				break;
		}
	}

	return NULL;
}


DatabaseQuery::~DatabaseQuery() {
	if (type == GET_DATA) {
		delete value_data.vv;
	} else if (type == EXTRACT_PARAM_VALUES) {
		delete extraction_parameters.params;
		delete extraction_parameters.prefixes;
		delete extraction_parameters.file_name;
	} else if (type == REGISTER_OBSERVER) {
		delete observer_registration_parameters.params_to_register;
		delete observer_registration_parameters.params_to_deregister;
	}
}

DatabaseQueryQueue::DatabaseQueryQueue() : database_manager(NULL), cant_prioritise_entries(0)
{}

bool DatabaseQueryQueue::QueryCmp(const QueueEntry& q1, const QueueEntry& q2) { 
		return q1.ranking > q2.ranking;
}

//ranking is in ascending order, so the lower value means higher rank
double DatabaseQueryQueue::FindQueryRanking(DatabaseQuery* q) {

	double ranking = 0;

	if (q == NULL)
		return 100.f;

	if (q->type == DatabaseQuery::CLEAR_CACHE)
		return 150.f;

	if (q->type == DatabaseQuery::REGISTER_OBSERVER)
		return 150.f;

	if (q->type == DatabaseQuery::SET_PROBER_ADDRESS)
		return 150.f;

	if (q->type == DatabaseQuery::RESET_BUFFER)
		return 100.f;

	if (q->type == DatabaseQuery::COMPILE_FORMULA)
		return 100.f;

	if (q->type == DatabaseQuery::REMOVE_PARAM)
		return nan("");

	if (q->type == DatabaseQuery::ADD_PARAM)
		return nan("");

	if (q->type == DatabaseQuery::STARTING_CONFIG_RELOAD)
		return nan("");

	if (q->type == DatabaseQuery::CHECK_CONFIGURATIONS_CHANGE)
		return -200.f;

	if (q->type == DatabaseQuery::EXTRACT_PARAM_VALUES)
		return -200.f;

	if (q->type == DatabaseQuery::ADD_BASE_PREFIX)
		return -250.f;

	InquirerId current_inquirer = database_manager->GetCurrentInquirerId();
	DrawInfo *current_draw_info = database_manager->GetCurrentDrawInfoForInquirer(q->inquirer_id);
	time_t t = database_manager->GetCurrentDateForInquirer(q->inquirer_id);

	if (q->type == DatabaseQuery::SEARCH_DATA)
		ranking = 1.f;
	else {
		DatabaseQuery::ValueData& vd = q->value_data;
		std::vector<DatabaseQuery::ValueData::V> &vv = *vd.vv;

		double d;

		time_t t1 = vv[0].time_second;
		time_t t2 = vv[vv.size() - 1].time_second;
		if (t1 <= t && t <= t2)
			d = 0;
		else
			d = std::min(abs(t - t1), abs(t - t2));

		ranking = 1.f / (2.f + d);

	}

	if (current_inquirer == q->inquirer_id)
		ranking += 2.f;

	if (q->draw_info == current_draw_info)
		ranking += 1.f;

	return ranking;

}

void DatabaseQueryQueue::ShufflePriorities() {

	wxMutexLocker lock(mutex);

	if (cant_prioritise_entries)
		return;

	for (std::list<QueueEntry>::iterator i = queue.begin();
		i != queue.end();
		++i) {

		QueueEntry& e = *i;
		DatabaseQuery* q = e.query;
		e.ranking = FindQueryRanking(q);

	}

	queue.sort(QueryCmp);

}
void DatabaseQueryQueue::CleanOld(DatabaseQuery *query) {
	time_t start = query->search_data.start_second;
	time_t end = query->search_data.end_second;

	assert(start<end);

	if(queue.size()>100)
	{
		wxMutexLocker lock(mutex);

		std::list<QueueEntry>::iterator i=queue.begin();

		for(i=queue.begin();i!=queue.end();i++)
		{
			if((i->query->search_data.start_second < start)||(i->query->search_data.end_second>end))
			{
				wxSemaError err = semaphore.TryWait();
				assert(err == wxSEMA_NO_ERROR);
				queue.erase(i++);
			}
		}
	}
}

void DatabaseQueryQueue::Add(DatabaseQuery *query) {
	QueueEntry entry;
	entry.query = query;
	entry.ranking = FindQueryRanking(query);
	wxMutexLocker lock(mutex);
	if (std::isnan(entry.ranking))
		cant_prioritise_entries += 1;
	std::list<QueueEntry>::iterator i;
	if (cant_prioritise_entries)
		i = queue.end();
	else
       		i = std::upper_bound(queue.begin(), queue.end(), entry, QueryCmp);
	queue.insert(i, entry);
	semaphore.Post();
}

void DatabaseQueryQueue::Add(std::list<DatabaseQuery*> &qlist){

	std::list<QueueEntry> querylist;

	bool needsort = false;

	for(std::list<DatabaseQuery*>::iterator i = qlist.begin(); i != qlist.end(); i++){
		QueueEntry entry;
		entry.query = *i;
		entry.ranking = FindQueryRanking(*i);
		if(!needsort && !querylist.empty() && !QueryCmp(querylist.back(), entry)) {
			needsort = true;
		}
		querylist.push_back(entry);
	}

	if (needsort) {
		querylist.sort(QueryCmp);
	}

	wxMutexLocker lock(mutex);
	unsigned int size = querylist.size();
	queue.merge(querylist, QueryCmp);
	for (unsigned int x = 0; x < size; x++)
		semaphore.Post();
}

DatabaseQuery* DatabaseQueryQueue::GetQuery() {
	semaphore.Wait();
	wxMutexLocker lock(mutex);

	QueueEntry& qe = queue.front();
	if (std::isnan(qe.ranking))
		cant_prioritise_entries -= 1;

	DatabaseQuery* dq = qe.query;
	queue.pop_front();
	return dq;
}

void DatabaseQueryQueue::SetDatabaseManager(DatabaseManager *manager) {
	database_manager = manager;
}

DatabaseQuery* CreateDataQueryPrivate(DrawInfo* di, TParam *param, PeriodType pt, int draw_no) {

	DatabaseQuery *q = new DatabaseQuery();
	q->type = DatabaseQuery::GET_DATA;
	q->draw_info = di;
	q->param = param;
	q->draw_no = draw_no;
	q->value_data.period_type = pt;
	q->value_data.vv = new std::vector<DatabaseQuery::ValueData::V>();

	return q;
}

DatabaseQuery* CreateDataQuery(DrawInfo* di, PeriodType pt, int draw_no) {
	TParam *p;
	if (di->IsValid())
		p = di->GetParam()->GetIPKParam();
	else
		p = NULL;

	return CreateDataQueryPrivate(di, p, pt, draw_no);
}


DatabaseQuery::ValueData::V& AddTimeToDataQuery(DatabaseQuery *q, const wxDateTime& time) {
	DatabaseQuery::ValueData::V v;
	ToNanosecondTime(time, v.time_second, v.time_nanosecond);
	v.custom_length = 0;
	q->value_data.vv->push_back(v);
	return q->value_data.vv->back();
}

