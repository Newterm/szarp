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
#include "sz4_iks.h"

#include <cmath>

#include <algorithm>
#include <iostream>
#include <limits>


Draw3Base::Draw3Base(wxEvtHandler *response_receiver) : m_response_receiver(response_receiver)
{}

void Draw3Base::SetProberAddress(const std::wstring& prefix,
			const std::wstring& address,
			const std::wstring& port)
{
}

void Draw3Base::ExtractParameters(DatabaseQuery *query)
{
	DatabaseQuery::ExtractionParameters &pars = query->extraction_parameters;
	boost::shared_ptr<SzbExtractor> extr(CreateExtractor());
	if (!extr)
		return;
	extr->SetPeriod(PeriodToProbeType(pars.pt),
			pars.start_time,
			szb_move_time(pars.end_time, 1, PeriodToProbeType(pars.pt), 0),
			0);

	extr->SetNoDataString(L"NO_DATA");
	extr->SetEmpty(0);

	FILE* output = fopen((const char*) SC::S2U(*pars.file_name).c_str(), "w");
	if (output == NULL)
		return;

	std::vector<SzbExtractor::Param> params;
	for (size_t i = 0; i < pars.params->size(); i++) {
		std::wstring param = pars.params->at(i);
		std::wstring prefix = pars.prefixes->at(i);
		params.push_back(SzbExtractor::Param(param, prefix, SzbExtractor::TYPE_AVERAGE));
	}

	extr->SetParams(params);
	extr->ExtractToCSV(output, L";");
	fclose(output);
}

void Draw3Base::RemoveConfig(DatabaseQuery* query) {
	RemoveConfig(query->reload_prefix.prefix, true);
	DatabaseResponse r(query);
	wxPostEvent(m_response_receiver, r);
}

void Draw3Base::RegisterObserver(DatabaseQuery *query) { }


void Draw3Base::SetProberAddress(DatabaseQuery* query) {
	SetProberAddress(query->prefix, query->prober_address.address, query->prober_address.port);

	free(query->prober_address.address);
	free(query->prober_address.port);

	delete query;
}

void Draw3Base::CompileLuaFormula(DatabaseQuery* query) {
	std::wstring error;
	bool ret;

	ret = CompileLuaFormula(query->compile_formula.formula, error);
	query->compile_formula.ok = ret;
	if (ret == false)
		query->compile_formula.error = wcsdup(error.c_str());
		
	DatabaseResponse dr(query);
	wxPostEvent(m_response_receiver, dr);
}

void Draw3Base::NotifyAboutConfigurationChanges(DatabaseQuery* query) {
	NotifyAboutConfigurationChanges();
	delete query;
}

void Draw3Base::AddExtraParam(DatabaseQuery *query) {
	AddExtraParam(query->defined_param.prefix, query->defined_param.p);
	free(query->defined_param.prefix);
	delete query;
}

void Draw3Base::RemoveExtraParam(DatabaseQuery* query) {
	RemoveExtraParam(query->defined_param.prefix, query->defined_param.p);
}


namespace {

void sz4_nanonsecond_to_pair(const sz4::nanosecond_time_t& time, time_t &second, time_t& nanosecond) {
	if (sz4::time_trait<sz4::nanosecond_time_t>::is_valid(time)) {
		second = time.second;
		nanosecond = time.nanosecond;
	} else {
		second = -1;
		nanosecond = -1;
	}
}

sz4::nanosecond_time_t pair_to_sz4_nanosecond(time_t second, time_t nanosecond) {
	if (second == (time_t) -1 && nanosecond == (time_t) -1)
		return sz4::time_trait<sz4::nanosecond_time_t>::invalid_value;
	else
		return sz4::nanosecond_time_t(second, nanosecond);
}

}

DatabaseQuery* CreateDataQueryPrivate(DrawInfo* di, TParam *param, PeriodType pt, int draw_no);

std::tr1::tuple<TSzarpConfig*, szb_buffer_t*> SzbaseBase::GetConfigAndBuffer(TParam *param) {
	if (param) {
		return std::tr1::tuple<TSzarpConfig*, szb_buffer_t*>(
			param->GetSzarpConfig(),
			szbase->GetBuffer(param->GetSzarpConfig()->GetPrefix())
		);
	} else {
		return std::tr1::tuple<TSzarpConfig*, szb_buffer_t*>(NULL, NULL);
	}
}

void SzbaseBase::maybeSetCancelHandle(TParam* p) {
#ifndef NO_LUA
	if (p->GetType() == TParam::P_LUA && p->GetFormulaType() == TParam::LUA_AV) {
		boost::mutex::scoped_lock datalock(m_mutex);
		cancelHandle = new SzbCancelHandle();
		cancelHandle->SetTimeout(60);
	}
#endif
}

void SzbaseBase::releaseCancelHandle(TParam* p) {

#ifndef NO_LUA
	if (p->GetType() == TParam::P_LUA && p->GetFormulaType() == TParam::LUA_AV) {
		boost::mutex::scoped_lock datalock(m_mutex);
		delete cancelHandle;
		cancelHandle = NULL;
	}
#endif

}

SzbaseBase::SzbaseBase(wxEvtHandler* response_receiver, const std::wstring& data_path, void (*conf_changed_cb)(std::wstring, std::wstring), int cache_size) : Draw3Base(response_receiver) {
	Szbase::Init(data_path, conf_changed_cb, true, cache_size);
	szbase = Szbase::GetObject();
}

SzbaseBase::~SzbaseBase() {
	Szbase::Destroy();
}

void SzbaseBase::RemoveConfig(const std::wstring& prefix, bool poison_cache) {
	szbase->RemoveConfig(prefix, poison_cache);
}

bool SzbaseBase::CompileLuaFormula(const std::wstring& formula, std::wstring& error) {
	return szbase->CompileLuaFormula(formula, error);
}

void SzbaseBase::AddExtraParam(const std::wstring& prefix, TParam *param) {
	szbase->AddExtraParam(prefix, param);
}

void SzbaseBase::RemoveExtraParam(const std::wstring& prefix, TParam *param) {
	szbase->RemoveExtraParam(prefix, param);
}

void SzbaseBase::NotifyAboutConfigurationChanges() {
	szbase->NotifyAboutConfigurationChanges();
}

void SzbaseBase::SetProberAddress(const std::wstring& prefix,
			const std::wstring& address,
			const std::wstring& port)
{
	szbase->SetProberAddress(prefix, address, port);
}

SzbExtractor* SzbaseBase::CreateExtractor() {
	IPKContainer* ipk = IPKContainer::GetObject();
	assert(ipk);

	return new SzbExtractor(ipk, szbase);
}

void SzbaseBase::SearchData(DatabaseQuery* query) {
	TParam *p = query->param;
	TSzarpConfig *cfg = NULL;
	szb_buffer_t *szb = NULL;

	DatabaseQuery::SearchData& sd = query->search_data;

	std::tr1::tie(cfg, szb) = GetConfigAndBuffer(p);
			
	if (szb == NULL) {
		sd.ok = true;
		sd.response_second = -1;
		sd.response_nanosecond = -1;

		DatabaseResponse dr(query);
		wxPostEvent(m_response_receiver, dr);
		return;
	}

	maybeSetCancelHandle(p);

	szbase->NextQuery();

	sd.response_second = 
		szb_search(szb, 
			p, 
			sd.start_second, 
			sd.end_second, 
			sd.direction, 
			PeriodToProbeType(sd.period_type), cancelHandle, *sd.search_condition);
	if (sd.response_second == (time_t) -1)
		sd.response_nanosecond = -1;
	else
		sd.response_nanosecond = 0;

	if (szb->last_err != SZBE_OK) {
		sd.ok = false;
		sd.error = szb->last_err;
		sd.error_str = wcsdup(szb->last_err_string.c_str());

		szb->last_err = SZBE_OK;
		szb->last_err_string = std::wstring();
	} else
		sd.ok = true;


	releaseCancelHandle(p);

	DatabaseResponse dr(query);
	wxPostEvent(m_response_receiver, dr);

}

void SzbaseBase::GetData(DatabaseQuery* query) {
	TParam *p = query->param;
	TSzarpConfig *cfg = NULL;
	szb_buffer_t *szb = NULL;

	std::tr1::tie(cfg, szb) = GetConfigAndBuffer(p);

	DatabaseQuery::ValueData &vd = query->value_data;

	if (vd.vv->size() == 0)
		return;

	SZARP_PROBE_TYPE pt = PeriodToProbeType(vd.period_type);

	int year = -1, month = -1;

	std::vector<DatabaseQuery::ValueData::V>::iterator i = vd.vv->begin();

	DatabaseQuery *rq = NULL;

	szbase->NextQuery();

	while (i != vd.vv->end()) {

		int new_year, new_month;
		szb_time2my(i->time_second, &new_year, &new_month);

		if (new_year != year || new_month != month) {
			if (rq) {
				DatabaseResponse dr(rq);
				wxPostEvent(m_response_receiver, dr);
				rq = NULL;
			}

			year = new_year;
			month = new_month;

		}

		if (rq == NULL) {
			rq = CreateDataQueryPrivate(query->draw_info, query->param, vd.period_type, query->draw_no);
			rq->inquirer_id = query->inquirer_id;
		}

		time_t end = szb_move_time(i->time_second, 1, pt, i->custom_length);
		if (p && szb) {
			bool fixed;
			i->response = szb_get_avg(szb, 
					p, 
					i->time_second,
					end,
					&i->sum,
					&i->count,
					pt,
					&fixed,
					&i->first_val,
					&i->last_val);
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
		wxPostEvent(m_response_receiver, dr);
	}

}

void SzbaseBase::ResetBuffer(DatabaseQuery* query) {
	TParam *p = query->param;
	TSzarpConfig *cfg = NULL;
	szb_buffer_t *szb = NULL;

	std::tr1::tie(cfg, szb) = GetConfigAndBuffer(p);

	if (szb)
		szb_reset_buffer(szb);
}

void SzbaseBase::ClearCache(DatabaseQuery* query) {
	TParam *p = query->param;
	TSzarpConfig *cfg = NULL;
	szb_buffer_t *szb = NULL;

	std::tr1::tie(cfg, szb) = GetConfigAndBuffer(p);

	if (cfg)
		szbase->ClearCacheDir(cfg->GetPrefix().c_str());
}

void SzbaseBase::StopSearch() {
	boost::mutex::scoped_lock datalock(m_mutex);
	if(cancelHandle != NULL) {
		cancelHandle->SetStopFlag();
	}
}

namespace {

template<class time_type>
void wsum_to_value(DatabaseQuery::ValueData::V& v,
		const sz4::weighted_sum<double, time_type>& wsum,
		SZARP_PROBE_TYPE pt,
		TParam::DataType dt,
		int prec) {
	typedef sz4::weighted_sum<double, time_type> wsum_type;

	typename wsum_type::sum_type sum;
	typename wsum_type::time_diff_type weight;

	sum = wsum.sum(weight);

	double scale;
	switch (pt) {
		case PT_HALFSEC: 
		case PT_MSEC10:
			scale = 10 * 60.;
			break;
		default:
			scale = 10 * 1000000000. * 60;
			break;
	}

	v.response = sz4::scale_value(wsum.avg(), dt, prec);
	v.sum = sz4::scale_value(double(sum), dt, prec) / scale;

	if (weight && v.count)
		v.count = v.count * double(weight) / (double(weight) + double(wsum.no_data_weight()));
	else
		v.count = 0;
}

template<class T> struct pair_to_sz4_type {};

template<> struct pair_to_sz4_type<sz4::nanosecond_time_t> {
	sz4::nanosecond_time_t operator()(time_t second, time_t nanosecond) const {
		if (second == (time_t) -1 && nanosecond == (time_t) -1)
			return sz4::time_trait<sz4::nanosecond_time_t>::invalid_value;
		else
			return sz4::nanosecond_time_t(second, nanosecond);
	}
};

template<> struct pair_to_sz4_type<sz4::second_time_t> {
	sz4::second_time_t operator()(time_t second, time_t nanosecond) const {
		return second;
	}
};
	
}


Sz4Base::Sz4Base(wxEvtHandler* response_receiver, const std::wstring& data_dir, IPKContainer* ipk_conatiner)
	: Draw3Base(response_receiver), data_dir(data_dir), ipk_container(ipk_conatiner) {

	base = new sz4::base(data_dir, ipk_conatiner);

}

Sz4Base::~Sz4Base() {
	delete base;
}

void Sz4Base::RemoveConfig(const std::wstring& prefix, bool poison_cache) {
	delete base;

	base = new sz4::base(data_dir, ipk_container);
}

bool Sz4Base::CompileLuaFormula(const std::wstring& formula, std::wstring& error) {
	lua_State* lua = base->get_lua_interpreter().lua();
	bool r = lua::compile_lua_formula(lua, (const char*)SC::S2U(formula).c_str());
	if (r == false)
		error = SC::lua_error2szarp(lua_tostring(lua, -1));
	lua_pop(lua, 1);
	return true;
}

void Sz4Base::AddExtraParam(const std::wstring& prefix, TParam *param) {
}

void Sz4Base::RemoveExtraParam(const std::wstring& prefix, TParam *param) {
	base->remove_param(param);
}

void Sz4Base::NotifyAboutConfigurationChanges() {

}

SzbExtractor* Sz4Base::CreateExtractor() {
	IPKContainer* ipk = IPKContainer::GetObject();
	assert(ipk);

	return new SzbExtractor(ipk, base);
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
#ifndef MINGW32
		return !isnanf(v);
#else
		return !std::isnan(v);
#endif
	}

	bool operator()(const double& v) const {
		return !std::isnan(v);
	}
};


void Sz4Base::SearchData(DatabaseQuery* query) {
	TParam* p = query->param;
	DatabaseQuery::SearchData& sd = query->search_data;

	if (p == NULL) {
		sd.ok = true;
		sd.response_second = -1;
		sd.response_nanosecond = -1;

		DatabaseResponse dr(query);
		wxPostEvent(m_response_receiver, dr);
		return;
	}

	sz4::nanosecond_time_t response;
	if (sd.direction > 0) {
		response = base->search_data_right(
			p,
			pair_to_sz4_nanosecond(sd.start_second, sd.start_nanosecond),
			pair_to_sz4_nanosecond(sd.end_second, sd.end_nanosecond),
			PeriodToProbeType(sd.period_type),
			no_data_search_condition()
		);
	} else {
		sz4::nanosecond_time_t end = pair_to_sz4_nanosecond(sd.end_second, sd.end_nanosecond);
		if (!sz4::time_trait<sz4::nanosecond_time_t>::is_valid(end)) {
			end.second = 0;
			end.nanosecond = 0;
		}
		response = base->search_data_left(
			p,
			pair_to_sz4_nanosecond(sd.start_second, sd.start_nanosecond),
			end,
			PeriodToProbeType(sd.period_type),
			no_data_search_condition());
	}
	sz4_nanonsecond_to_pair(response, sd.response_second, sd.response_nanosecond);
	sd.ok = true;

	DatabaseResponse dr(query);
	wxPostEvent(m_response_receiver, dr);
}

template<class time_type> void Sz4Base::GetValue(DatabaseQuery::ValueData::V& v,
		time_t second, time_t nanosecond, TParam* p, SZARP_PROBE_TYPE pt) {
	if (!p) {
		v.count = 0;
		v.response = nan("");
		return;
	}

	time_type time = pair_to_sz4_type<time_type>()(second, nanosecond);

	typedef sz4::weighted_sum<double, time_type> wsum_type;

	time_type end_time = szb_move_time(time, 1, pt, v.custom_length);
	wsum_type wsum;
	base->get_weighted_sum(p, time, end_time, pt, wsum);

	wsum_to_value(v, wsum, pt, p->GetDataType(), p->GetPrec());
}

void Sz4Base::GetData(DatabaseQuery* query) {
	DatabaseQuery::ValueData &vd = query->value_data;
	TParam* p = query->param;

	SZARP_PROBE_TYPE pt = PeriodToProbeType(vd.period_type);

	std::vector<DatabaseQuery::ValueData::V>::iterator i = vd.vv->begin();
	while (i != vd.vv->end()) {
		DatabaseQuery *rq = CreateDataQueryPrivate(query->draw_info, query->param, vd.period_type, query->draw_no);
		rq->inquirer_id = query->inquirer_id;
		try {
			if (pt == PT_HALFSEC || pt == PT_MSEC10)
				GetValue<sz4::nanosecond_time_t>(*i, i->time_second, i->time_nanosecond, p, pt);
			else
				GetValue<sz4::second_time_t>(*i, i->time_second, i->time_nanosecond, p, pt);

			i->ok = true;

		} catch (sz4::exception &e) {
			i->response = nan("");
			i->error = 1;
			i->error_str = wcsdup(SC::U2S((const unsigned char*)(e.what())).c_str());
			i->count = 0;
			i->ok = false;

		}
		rq->value_data.vv->push_back(*i);

		DatabaseResponse dr(rq);
		wxPostEvent(m_response_receiver, dr);
		i++;
	}	
}

void Sz4Base::ResetBuffer(DatabaseQuery* query) {

}

void Sz4Base::ClearCache(DatabaseQuery* query) {

}

void Sz4Base::StopSearch() {

}

void Sz4Base::RegisterObserver(DatabaseQuery *query) {
	base->deregister_observer(
		query->observer_registration_parameters.observer,
		*query->observer_registration_parameters.params_to_deregister);
	base->register_observer(
		query->observer_registration_parameters.observer,
		*query->observer_registration_parameters.params_to_register);
}


Sz4ApiBase::Sz4ApiBase(wxEvtHandler* response_receiver,
			const std::wstring& address, const std::wstring& port,
			IPKContainer *ipk_container) 
			: Draw3Base(response_receiver)
			, ipk_container(ipk_container) {

	std::tie(connection_mgr, base, io) =
		build_iks_client(ipk_container, address, port);

	io_thread = start_connection_manager(connection_mgr);
}

void Sz4ApiBase::RemoveConfig(const std::wstring& prefix,
			bool poison_cache) 
{
}

bool Sz4ApiBase::CompileLuaFormula(const std::wstring& formula, std::wstring& error) {
	error = L"not implemented";
	return false;
}

void Sz4ApiBase::AddExtraParam(const std::wstring& prefix, TParam *param) {
	///XXX:
}

void Sz4ApiBase::RemoveExtraParam(const std::wstring& prefix, TParam *param) {
	///XXX:
}


void Sz4ApiBase::NotifyAboutConfigurationChanges() {
}

void Sz4ApiBase::SetProberAddress(const std::wstring& prefix,
			const std::wstring& address,
			const std::wstring& port) {
}


SzbExtractor* Sz4ApiBase::CreateExtractor() {
	///XXX:
	return NULL;
}


void Sz4ApiBase::SearchData(DatabaseQuery* query) {
	TParam* p = query->param;
	DatabaseQuery::SearchData& sd = query->search_data;

	if (p == NULL) {
		sd.ok = true;
		sd.response_second = -1;
		sd.response_nanosecond = -1;

		DatabaseResponse dr(query);
		wxPostEvent(m_response_receiver, dr);
		return;
	}

	auto cb = [query, this] (const boost::system::error_code& ec, const sz4::nanosecond_time_t& t) {
		DatabaseQuery::SearchData& sd = query->search_data;
		if (!ec) {
			sz4_nanonsecond_to_pair(t, sd.response_second, sd.response_nanosecond);
			sd.ok = true;
		} else {
			sd.ok = false;
			sd.error_str = wcsdup(SC::L2S(ec.message()).c_str());
		}

		DatabaseResponse dr(query);
		wxPostEvent(m_response_receiver, dr);
	};

	if (sd.direction > 0) {
		base->search_data_right<sz4::nanosecond_time_t>(
			ParamInfoFromParam(p),
			pair_to_sz4_nanosecond(sd.start_second, sd.start_nanosecond),
			pair_to_sz4_nanosecond(sd.end_second, sd.end_nanosecond),
			PeriodToProbeType(sd.period_type),
			cb
		);
	} else {
		sz4::nanosecond_time_t end = pair_to_sz4_nanosecond(sd.end_second, sd.end_nanosecond);
		if (!sz4::time_trait<sz4::nanosecond_time_t>::is_valid(end)) {
			end.second = 0;
			end.nanosecond = 0;
		}
		base->search_data_left<sz4::nanosecond_time_t>(
			ParamInfoFromParam(p),
			pair_to_sz4_nanosecond(sd.start_second, sd.start_nanosecond),
			end,
			PeriodToProbeType(sd.period_type),
			cb
		);
	}
}

sz4::param_info Sz4ApiBase::ParamInfoFromParam(TParam* p) {
	return sz4::param_info(p->GetSzarpConfig()->GetPrefix(), p->GetName());
}

namespace {

void dummy_prepare_sz4_param(TParam* param) {
	if (param->GetSz4Type() != TParam::SZ4_NONE)
		return;

	if (param->GetType() == TParam::P_REAL) {
		param->SetSz4Type(TParam::SZ4_REAL);
		return;
	}

	if (param->IsDefinable())
		param->PrepareDefinable();

	if (param->GetType() == TParam::P_COMBINED) {
		param->SetSz4Type(TParam::SZ4_COMBINED);
		param->SetDataType(TParam::INT);
		return;
	}

	if (param->GetType() == TParam::P_DEFINABLE) {
		param->SetSz4Type(TParam::SZ4_DEFINABLE);
		param->SetDataType(TParam::DOUBLE);
		return;
	}

	if (param->GetType() == TParam::P_LUA) {
		param->SetSz4Type(TParam::SZ4_LUA);
		param->SetDataType(TParam::DOUBLE);
	}

}


}


template<class time_type> void Sz4ApiBase::DoGetData(DatabaseQuery* query) {
	auto query_ptr = std::shared_ptr<DatabaseQuery>(query);

	DatabaseQuery::ValueData &vd = query->value_data;
	SZARP_PROBE_TYPE pt = PeriodToProbeType(vd.period_type);

	dummy_prepare_sz4_param(query->param);

	TParam::DataType dt = query->param->GetDataType();
	int prec = query->param->GetPrec();

	for (auto v : *vd.vv) {
		time_type time = pair_to_sz4_type<time_type>()(v.time_second, v.time_nanosecond);
		base->get_weighted_sum<double, time_type>(
			ParamInfoFromParam(query->param),
			time, szb_move_time(time, 1, pt), pt,
			[query_ptr, time, this, v, dt, prec]
			(const boost::system::error_code& ec, const std::vector<sz4::weighted_sum<double, time_type>> & sums) {
				auto& vd = query_ptr->value_data;
				auto *rq = CreateDataQueryPrivate(query_ptr->draw_info, query_ptr->param,
							vd.period_type, query_ptr->draw_no);
				rq->inquirer_id = query_ptr->inquirer_id;
				rq->value_data.vv->push_back(v);
				auto &nv = rq->value_data.vv->back();

				if (!ec) {
					wsum_to_value(nv, sums.at(0),
							PeriodToProbeType(query_ptr->value_data.period_type),
							dt, prec);
					nv.ok = true;
				} else {
					nv.error = 1;
					nv.error_str = wcsdup(SC::L2S(ec.message()).c_str());
					nv.count = 0;
					nv.response = nan("");
					nv.ok = false;
				}

				DatabaseResponse r(rq);
				wxPostEvent(m_response_receiver, r);
		});
	}
}

void Sz4ApiBase::GetData(DatabaseQuery* query) {
	DatabaseQuery::ValueData &vd = query->value_data;
	SZARP_PROBE_TYPE pt = PeriodToProbeType(vd.period_type);
	if (pt == PT_HALFSEC || pt == PT_MSEC10)
		DoGetData<sz4::nanosecond_time_t>(query);
	else
		DoGetData<sz4::second_time_t>(query);
}

void Sz4ApiBase::ResetBuffer(DatabaseQuery* query) {

}


void Sz4ApiBase::ClearCache(DatabaseQuery* query) {

}
	
void Sz4ApiBase::StopSearch() {

}

Sz4ApiBase::ObserverWrapper::ObserverWrapper(TParam* param, sz4::param_observer* obs)
			: obs(obs), param(param) {
	prefix = param->GetSzarpConfig()->GetPrefix();
}

void Sz4ApiBase::ObserverWrapper::operator()() {
	obs->param_data_changed(param);
}

void Sz4ApiBase::RegisterObserver(DatabaseQuery* query) {
	auto observer = query->observer_registration_parameters.observer;
	auto& to_dereg = *query->observer_registration_parameters.params_to_deregister;
	auto& to_reg = *query->observer_registration_parameters.params_to_deregister;

	for (auto& p : to_dereg) {
		auto pi = ParamInfoFromParam(p);
		auto i = observers.find(std::make_pair(observer, pi));
		if (i == observers.end())
			continue;

		base->deregister_observer(
			i->second,
			std::vector<sz4::param_info>(1, pi),
			[] (const boost::system::error_code&) { /*XXX*/ });

		observers.erase(i);
	}

	for (auto& p : to_reg) {
		auto pi = ParamInfoFromParam(p);
		auto ow = std::make_shared<ObserverWrapper>(p, observer);
		observers[std::make_pair(observer, pi)] = ow;

		base->register_observer(
			ow,
			std::vector<sz4::param_info>(1 , pi),
			[] (const boost::system::error_code&) { /*XXX*/ });
	}

}

Sz4ApiBase::~Sz4ApiBase() {}

QueryExecutor::QueryExecutor(DatabaseQueryQueue *_queue, wxEvtHandler *_response_receiver, Draw3Base *_base) :
	wxThread(wxTHREAD_JOINABLE), queue(_queue), response_receiver(_response_receiver), base(_base)
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

	while ((q = queue->GetQuery())) {
		switch (q->type) {
			case DatabaseQuery::STARTING_CONFIG_RELOAD:
				base->RemoveConfig(q);
				break;
			
			case DatabaseQuery::COMPILE_FORMULA:
				base->CompileLuaFormula(q);
				break;

			case DatabaseQuery::ADD_PARAM:
				base->AddExtraParam(q);
				break;

			case DatabaseQuery::REMOVE_PARAM:
				base->RemoveExtraParam(q);
				break;

			case DatabaseQuery::CHECK_CONFIGURATIONS_CHANGE:
				base->NotifyAboutConfigurationChanges(q);
				break;

			case DatabaseQuery::SET_PROBER_ADDRESS:
				base->SetProberAddress(q);
				break;

			case DatabaseQuery::EXTRACT_PARAM_VALUES:
				base->ExtractParameters(q);
				break;

			case DatabaseQuery::SEARCH_DATA:
				base->SearchData(q);
				break;

			case DatabaseQuery::GET_DATA:
				base->GetData(q);
				break;

			case DatabaseQuery::RESET_BUFFER:
				base->ResetBuffer(q);
				break;

			case DatabaseQuery::CLEAR_CACHE:
				base->ClearCache(q);
				break;

			case DatabaseQuery::REGISTER_OBSERVER:
				base->RegisterObserver(q);
				break;
		}
	
	}

	delete base;
	
	return NULL;
}


void QueryExecutor::StopSearch() {
	base->StopSearch();
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

