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

#include "classes.h"
#include "database.h"
#include "cfgmgr.h"
#include "dbmgr.h"
#include "conversion.h"
#include "szbextr/extr.h"

#include <cmath>

#include <algorithm>
#include <iostream>
#include <limits>

#include <sz4/base.h>

sz4::base* sz4_base;

DatabaseQuery* CreateDataQueryPrivate(DrawInfo* di, TParam *param, PeriodType pt, int draw_no);

void SzbaseBase::RemoveConfig(const std::wstring& prefix, bool poison_cache) {
	TParam *p = q->defined_param.p;
	wchar_t *prefix = q->defined_param.prefix;
	m_szbase->RemoveExtraParam(prefix, p);
}

bool SzbaseBase::CompileLuaFormula(const std::wstring& formula, std::wstring& error) {

}



QueryExecutor::QueryExecutor(DatabaseQueryQueue *_queue, wxEvtHandler *_response_receiver, Szbase *_szbase) :
	wxThread(wxTHREAD_JOINABLE), queue(_queue), response_receiver(_response_receiver), szbase(_szbase), cancelHandle(NULL)
{ 
	sz4_base = new sz4::base(L"/opt/szarp", IPKContainer::GetObject());
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
		default:
			pt = PT_CUSTOM;
			assert (0);
	}

	return pt;
}

void* QueryExecutor::Entry() {
	DatabaseQuery *q = NULL;

	while ((q = queue->GetQuery())) {

		bool post_response = false;

		if (q->type == DatabaseQuery::STARTING_CONFIG_RELOAD) {
			szbase->RemoveConfig(q->reload_prefix.prefix, true);
			post_response = true;
		} else if (q->type == DatabaseQuery::COMPILE_FORMULA) {
			std::wstring error;
			bool ret;
#ifndef NO_LUA
			ret = szbase->CompileLuaFormula(q->compile_formula.formula, error);
			q->compile_formula.ok = ret;
			if (ret == false)
				q->compile_formula.error = wcsdup(error.c_str());
				
#else
			q->compile_formula.ok = false;
			q->compile_formula.error = strdup(error.c_str());
#endif
			post_response = true;

#ifndef NO_LUA
		} else if (q->type == DatabaseQuery::ADD_PARAM) {
			TParam *p = q->defined_param.p;
			wchar_t *prefix = q->defined_param.prefix;

			szbase->AddExtraParam(prefix, p);

			post_response = false;

			free(prefix);
		} else if (q->type == DatabaseQuery::REMOVE_PARAM) {
			post_response = true;
#endif
		} else if (q->type == DatabaseQuery::CHECK_CONFIGURATIONS_CHANGE) {
			szbase->NotifyAboutConfigurationChanges();
			post_response = false;
		} else if (q->type == DatabaseQuery::SET_PROBER_ADDRESS) {
			szbase->SetProberAddress(q->prefix,
					q->prober_address.address,
					q->prober_address.port);
			free(q->prober_address.address);
			free(q->prober_address.port);
			post_response = false;
		} else if (q->type == DatabaseQuery::EXTRACT_PARAM_VALUES) {
			ExecuteExtractParametersQuery(q->extraction_parameters);
			post_response = false;
		} else {
			TParam *p = NULL; 
			TSzarpConfig *cfg = NULL;
			szb_buffer_t *szb = NULL;
			
			if (q->param) {
				p = q->param;
				cfg = p->GetSzarpConfig();
				szb = szbase->GetBuffer(cfg->GetPrefix());
			}

			switch (q->type) {
				case DatabaseQuery::SEARCH_DATA: 
					szbase->NextQuery();
					ExecuteSearchQuery(szb, p, q->search_data);
					post_response = true;
					break;
				case DatabaseQuery::GET_DATA: 
					szbase->NextQuery();
					ExecuteDataQuery(szb, p, q);
					post_response = false;
					break;
				case DatabaseQuery::RESET_BUFFER:
					if (szb)
						szb_reset_buffer(szb);
					break;
				case DatabaseQuery::CLEAR_CACHE:
					if (cfg)
						szbase->ClearCacheDir(cfg->GetPrefix().c_str());
					break;
				default:
					break;
			}

		}
	
		if (post_response) {
			DatabaseResponse r(q);
			wxPostEvent(response_receiver, r);
		} else
			delete q;
	
	}
	
	delete base;
	Szbase::Destroy();

	return NULL;
}

void QueryExecutor::ExecuteDataQuery(szb_buffer_t* szb, TParam* p, DatabaseQuery* q) {
	DatabaseQuery::ValueData &vd = q->value_data;

	if (vd.vv->size() == 0)
		return;

	SZARP_PROBE_TYPE pt = PeriodToProbeType(vd.period_type);

	int year = -1, month = -1;

	std::vector<DatabaseQuery::ValueData::V>::iterator i = vd.vv->begin();

	DatabaseQuery *rq = NULL;

	while (i != vd.vv->end()) {

		int new_year, new_month;
		szb_time2my(i->time, &new_year, &new_month);

		if (new_year != year || new_month != month) {
			if (rq) {
				DatabaseResponse dr(rq);
				wxPostEvent(response_receiver, dr);
				rq = NULL;
			}

			year = new_year;
			month = new_month;

		}

		if (rq == NULL) {
			rq = CreateDataQueryPrivate(q->draw_info, q->param, vd.period_type, q->draw_no);
			rq->inquirer_id = q->inquirer_id;
		}

		time_t end = szb_move_time(i->time, 1, pt, i->custom_length);
		if (p && szb) {
			sz4::weighted_sum<double, sz4::second_time_t> sum;
			sz4_base->get_weighted_sum(p, sz4::second_time_t(i->time), sz4::second_time_t(end), pt, sum);
			i->response = sum.sum() / sum.weight() / pow10(p->GetPrec());
			i->sum = sum.sum() / pow10(p->GetPrec());
			i->count = sum.weight() / (sum.weight() + sum.no_data_weight()) * 100;
#if 0
			bool fixed;
			i->response = szb_get_avg(szb, 
					p, 
					i->time,
					end,
					&i->sum,
					&i->count,
					pt,
					&fixed,
					&i->first_val,
					&i->last_val);
#endif
			if (szb->last_err != SZBE_OK) {
				i->ok = false;
				i->error = szb->last_err;
				i->error_str = wcsdup(szb->last_err_string.c_str());

				szb->last_err = SZBE_OK;
				szb->last_err_string = std::wstring();
			} else {
				i->ok = true;
			}

		} else {
			i->response = nan("");
			i->ok = true;
		}

		rq->value_data.vv->push_back(*i);

		++i;
	}

	if (rq) {
		DatabaseResponse dr(rq);
		wxPostEvent(response_receiver, dr);
	}

}

void QueryExecutor::StopSearch() {
	boost::mutex::scoped_lock datalock(m_mutex);
	if(cancelHandle != NULL) {
		cancelHandle->SetStopFlag();
	}
}

class no_data_search_condition : public sz4::search_condition {
public:
	bool operator()(const short& v) const {
		return v != std::numeric_limits<short>::min();
	}

	bool operator()(const int& v) const {
		return v != std::numeric_limits<int>::min();

	}

	bool operator()(const float& v) const {
		return isnanf(v);
	}

	bool operator()(const double& v) const {
		return isnan(v);
	}
};

void QueryExecutor::ExecuteSearchQuery(szb_buffer_t* szb, TParam *p, DatabaseQuery::SearchData& sd) {
	if (szb == NULL) {
		sd.ok = true;
		sd.response = -1;
		return;
	}

#ifndef NO_LUA
	if (p->GetType() == TParam::P_LUA && p->GetFormulaType() == TParam::LUA_AV) {
		boost::mutex::scoped_lock datalock(m_mutex);
		cancelHandle = new SzbCancelHandle();
		cancelHandle->SetTimeout(60);
	}
#endif

#if 0
	sd.response = 
		szb_search(szb, 
			p, 
			sd.start, 
			sd.end, 
			sd.direction, 
			PeriodToProbeType(sd.period_type), cancelHandle, *sd.search_condition);
#endif
	if (sd.direction > 0)
		sd.response = sz4_base->search_data_right(p, sz4::second_time_t(sd.start), sz4::second_time_t(sd.end), PeriodToProbeType(sd.period_type), no_data_search_condition());
	else {
		if (sd.end == -1)
			sd.end = 0;
		sd.response = sz4_base->search_data_left(p, sz4::second_time_t(sd.start), sz4::second_time_t(sd.end), PeriodToProbeType(sd.period_type), no_data_search_condition());
	}

	if (szb->last_err != SZBE_OK) {
		sd.ok = false;
		sd.error = szb->last_err;
		sd.error_str = wcsdup(szb->last_err_string.c_str());

		szb->last_err = SZBE_OK;
		szb->last_err_string = std::wstring();
	} else
		sd.ok = true;

#ifndef NO_LUA
	if (p->GetType() == TParam::P_LUA && p->GetFormulaType() == TParam::LUA_AV) {
		boost::mutex::scoped_lock datalock(m_mutex);
		delete cancelHandle;
		cancelHandle = NULL;
	}
#endif

}

void QueryExecutor::ExecuteExtractParametersQuery(DatabaseQuery::ExtractionParameters &pars) {
	IPKContainer* ipk = IPKContainer::GetObject();
	assert(ipk);

	SzbExtractor extr(szbase);
	extr.SetPeriod(PeriodToProbeType(pars.pt),
			pars.start_time,
			szb_move_time(pars.end_time, 1, PeriodToProbeType(pars.pt), 0),
			0);

	extr.SetNoDataString(L"NO_DATA");
	extr.SetEmpty(0);

	FILE* output = fopen((const char*) SC::S2U(*pars.file_name).c_str(), "w");
	if (output == NULL)
		return;

	std::vector<SzbExtractor::Param> params;
	for (size_t i = 0; i < pars.params->size(); i++) {
		std::wstring param = pars.params->at(i);
		std::wstring prefix = pars.prefixes->at(i);
		szb_buffer_t* buffer = szbase->GetBuffer(prefix);
		params.push_back(SzbExtractor::Param(param, prefix, buffer, SzbExtractor::TYPE_AVERAGE));
	}

	extr.SetParams(params);
	extr.ExtractToCSV(output, L";");
	fclose(output);
}

DatabaseQuery::~DatabaseQuery() {
	if (type == GET_DATA) {
		delete value_data.vv;
	} else if (type == EXTRACT_PARAM_VALUES) {
		delete extraction_parameters.params;
		delete extraction_parameters.prefixes;
		delete extraction_parameters.file_name;
	}
}

DatabaseQueryQueue::DatabaseQueryQueue() : database_manager(NULL), cant_prioritise_entries(0)
{}

bool DatabaseQueryQueue::QueryCmp(const QueueEntry& q1, const QueueEntry& q2) { 
		return q1.ranking > q2.ranking;
}

double DatabaseQueryQueue::FindQueryRanking(DatabaseQuery* q) {

	double ranking = 0;

	if (q == NULL)
		return 100.f;

	if (q->type == DatabaseQuery::CLEAR_CACHE)
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
		return -100.f;

	if (q->type == DatabaseQuery::CHECK_CONFIGURATIONS_CHANGE)
		return -200.f;

	if (q->type == DatabaseQuery::EXTRACT_PARAM_VALUES)
		return -200.f;


	InquirerId current_inquirer = database_manager->GetCurrentInquirerId();
	DrawInfo *current_draw_info = database_manager->GetCurrentDrawInfoForInquirer(q->inquirer_id);
	time_t t = database_manager->GetCurrentDateForInquirer(q->inquirer_id);

	if (q->type == DatabaseQuery::SEARCH_DATA)
		ranking = 1.f;
	else {
		DatabaseQuery::ValueData& vd = q->value_data;
		std::vector<DatabaseQuery::ValueData::V> &vv = *vd.vv;

		double d;

		time_t& t1 = vv[0].time;
		time_t& t2 = vv[vv.size() - 1].time;
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
	time_t start = query->search_data.start;
	time_t end = query->search_data.end;

	assert(start<end);
	
	if(queue.size()>100)
	{
		wxMutexLocker lock(mutex);
		
		std::list<QueueEntry>::iterator i=queue.begin();
		
		for(i=queue.begin();i!=queue.end();i++)
		{
			if((i->query->search_data.start < start)||(i->query->search_data.end>end))
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


void AddTimeToDataQuery(DatabaseQuery *q, time_t time) {
	DatabaseQuery::ValueData::V v;
	v.time = time;
	v.custom_length = 0;
	q->value_data.vv->push_back(v);
}

