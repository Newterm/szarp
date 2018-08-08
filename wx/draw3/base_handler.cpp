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
#include "sz4/util.h"
#include "sz4/exceptions.h"
#include "szarp_base_common/lua_utils.h"
#include "ikssetup.h"
#include "sz4_iks_param_info.h"
#include "sz4_iks.h"
#include "base_handler.h"
#include "custom_assert.h"

#include <cmath>

#include <algorithm>
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

void Draw3Base::RegisterObserver(DatabaseQuery *query) {
	delete query;
}

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
	if (p->GetType() == ParamType::LUA && p->GetFormulaType() == FormulaType::LUA_AV) {
		boost::mutex::scoped_lock datalock(m_mutex);
		cancelHandle = new SzbCancelHandle();
		cancelHandle->SetTimeout(60);
	}
#endif
}

void SzbaseBase::releaseCancelHandle(TParam* p) {

#ifndef NO_LUA
	if (p->GetType() == ParamType::LUA && p->GetFormulaType() == FormulaType::LUA_AV) {
		boost::mutex::scoped_lock datalock(m_mutex);
		delete cancelHandle;
		cancelHandle = NULL;
	}
#endif

}

SzbaseBase::SzbaseBase(wxEvtHandler* response_receiver,
	   	const std::wstring& data_path, IPKContainer* _ipk_container,
	   	void (*conf_changed_cb)(std::wstring, std::wstring),
	   	int cache_size) : Draw3Base(response_receiver), ipk_container(_ipk_container) {
	Szbase::Init(data_path, conf_changed_cb, true, cache_size);
	szbase = Szbase::GetObject();

	ASSERT(ipk_container);
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
	return new SzbExtractor(ipk_container, szbase);
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
					&i->data_count,
					pt,
					&fixed,
					&i->first_val,
					&i->last_val);
			i->no_data_count -= i->data_count;
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

	delete query;
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

std::wstring SzbaseBase::GetType() const {
	return L"sz3";
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

	v.response = sz4::scale_value(wsum.avg(), dt, prec);

	double scale;
	switch (pt) {
		case PT_HALFSEC: 
		case PT_MSEC10:
			scale = 10 * 1000000000. * 60;
			break;
		default:
			scale = 10 * 60.;
			break;
	}

	v.sum = sz4::scale_value(double(sum), dt, prec) / scale;

	const double data_time_span = 10; // used to change no-data time weight to period-based missing probes
	v.data_count = double(weight) / data_time_span;
	v.no_data_count = double(wsum.no_data_weight()) / data_time_span;

	v.fixed = wsum.fixed();
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
	ASSERT(ipk_container);
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
	IPKContainer* ipk = ipk_container;
	return new SzbExtractor(ipk, base);
}


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

	sz4::nanosecond_time_t end = pair_to_sz4_nanosecond(sd.end_second, sd.end_nanosecond);
	sz4::nanosecond_time_t start = pair_to_sz4_nanosecond(sd.start_second, sd.start_nanosecond);

	if (sd.direction > 0) {
		response = base->search_data_right(
			p,
			start,
			end,
			PeriodToProbeType(sd.period_type),
			sz4::no_data_search_condition()
		);
	} else {
		if (!sz4::time_trait<sz4::nanosecond_time_t>::is_valid(end)) {
			end.second = 0;
			end.nanosecond = 0;
		}
		response = base->search_data_left(
			p,
			start,
			end,
			PeriodToProbeType(sd.period_type),
			sz4::no_data_search_condition());
	}
	sz4_nanonsecond_to_pair(response, sd.response_second, sd.response_nanosecond);
	sd.ok = true;

	DatabaseResponse dr(query);
	wxPostEvent(m_response_receiver, dr);
}

template<class time_type> void Sz4Base::GetValue(DatabaseQuery::ValueData::V& v,
		time_t second, time_t nanosecond, TParam* p, SZARP_PROBE_TYPE pt) {
	if (!p) {
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
			i->data_count = 0;
			i->ok = false;

		}
		rq->value_data.vv->push_back(*i);

		DatabaseResponse dr(rq);
		wxPostEvent(m_response_receiver, dr);
		i++;
	}

	delete query;
}

void Sz4Base::ResetBuffer(DatabaseQuery* query) {

}

void Sz4Base::ClearCache(DatabaseQuery* query) {

}

void Sz4Base::StopSearch() {

}

std::wstring Sz4Base::GetType() const {
	return L"sz4";
}

void Sz4Base::RegisterObserver(DatabaseQuery *query) {
	base->deregister_observer(
		query->observer_registration_parameters.observer,
		*query->observer_registration_parameters.params_to_deregister);
	base->register_observer(
		query->observer_registration_parameters.observer,
		*query->observer_registration_parameters.params_to_register);
	delete query;
}


Sz4ApiBase::Sz4ApiBase(wxEvtHandler* response_receiver,
			const std::wstring& address, const std::wstring& port,
			IPKContainer *ipk_container) 
			: Draw3Base(response_receiver)
			, ipk_container(ipk_container) {

	std::tie(connection_mgr, base, io) =
		build_iks_client(ipk_container, address, port, _T("User:Param:"));

	connection_cv = std::move(connection_mgr->connection_cv.get_future());

	io_thread = start_connection_manager(connection_mgr);
}

bool Sz4ApiBase::BlockUntilConnected(const unsigned int timeout_s) {
	if (!connection_cv.valid()) {
		throw DrawBaseException(_("Could not establish connection to iks server"));
	}

	const std::chrono::seconds connection_establishing_timeout = std::chrono::seconds(timeout_s);
	
	if (connection_cv.wait_for(connection_establishing_timeout) == std::future_status::ready) return true;
	return false;
}

void Sz4ApiBase::RemoveConfig(const std::wstring& prefix,
			bool poison_cache) 
{
	auto i = observers.begin();
	while (i != observers.end()) {
		sz4::param_info pi = i->first.second;
		if (pi.prefix() == prefix) {
			base->deregister_observer(
				i->second,
				{ pi },
				[] (const boost::system::error_code&) { /*XXX*/ });

			i = observers.erase(i);
		} else 
			std::advance(i, 1);
	}
}

bool Sz4ApiBase::CompileLuaFormula(const std::wstring& formula, std::wstring& error) {
	///XXX: implement
	return true;
}

void Sz4ApiBase::AddExtraParam(const std::wstring& prefix, TParam *param) {}

void Sz4ApiBase::RemoveExtraParam(const std::wstring& prefix, TParam *param) {}

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
			sd.response_second = sd.response_nanosecond = -1;
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
	if (param->GetSz4Type() != Sz4ParamType::NONE)
		return;

	if (param->GetType() == ParamType::REAL) {
		param->SetSz4Type(Sz4ParamType::REAL);
		return;
	}

	if (param->IsDefinable())
		param->PrepareDefinable();

	if (param->GetType() == ParamType::COMBINED) {
		param->SetSz4Type(Sz4ParamType::COMBINED);
		param->SetDataType(TParam::INT);
		return;
	}

	if (param->GetType() == ParamType::DEFINABLE) {
		param->SetSz4Type(Sz4ParamType::DEFINABLE);
		param->SetDataType(TParam::DOUBLE);
		return;
	}

	if (param->GetType() == ParamType::LUA) {
		param->SetSz4Type(Sz4ParamType::LUA);
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

	using V = DatabaseQuery::ValueData::V;
	auto minmax = std::minmax_element(vd.vv->begin(), vd.vv->end(),
		[](const V& a, const V& b){
			return a.time_second < b.time_second || (a.time_second == b.time_second && a.time_nanosecond < b.time_nanosecond);
		}
	);

	const auto& v = *minmax.first;
	auto t_start = sz4::nanosecond_time_t(v.time_second, v.time_nanosecond);
	auto t_end = szb_move_time(sz4::nanosecond_time_t(minmax.second->time_second, minmax.second->time_nanosecond), 1, pt);

	base->get_weighted_sum<double, time_type>(
		ParamInfoFromParam(query->param),
		t_start, t_end, pt,
		[query_ptr, t_start, pt, this, v, dt, prec]
		(const boost::system::error_code& ec, const std::vector<sz4::weighted_sum<double, time_type>> & sums) {

			sz4::nanosecond_time_t s_time = t_start;
			for (const auto& sum: sums) {
				auto *rq = CreateDataQueryPrivate(query_ptr->draw_info, query_ptr->param,
							query_ptr->value_data.period_type, query_ptr->draw_no);
				rq->inquirer_id = query_ptr->inquirer_id;
				rq->value_data.vv->push_back(v);
				auto &nv = rq->value_data.vv->back();
				rq->value_data.vv->back().time_second = s_time.second;
				rq->value_data.vv->back().time_nanosecond = s_time.nanosecond;

				if (!ec) {
					wsum_to_value(nv, sum,
							PeriodToProbeType(query_ptr->value_data.period_type),
							dt, prec);
					nv.ok = true;
				} else {
					nv.error = 1;
					nv.error_str = wcsdup(SC::L2S(ec.message()).c_str());
					nv.data_count = 0;
					nv.response = nan("");
					nv.ok = false;
				}

				s_time = szb_move_time(s_time, 1, pt);

				DatabaseResponse r(rq);
				wxPostEvent(m_response_receiver, r);
			} // for each sum
		} // callback
	); // get_weighted_sum
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

std::wstring Sz4ApiBase::GetType() const {
	return L"iks";
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
	auto& to_reg = *query->observer_registration_parameters.params_to_register;

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

	delete query;
}

Sz4ApiBase::~Sz4ApiBase() {}




Draw3Base::ptr SzbaseHandler::GetIksHandlerFromBase(const wxString& prefix)
{
#ifndef __WXGTK__
	return Draw3Base::ptr(nullptr);
#else
	const wxString DEFAULT_IKS_PORT = "9002";
	wxString iks_port(DEFAULT_IKS_PORT);
	wxString iks_server;

	char *iks_s = libpar_getpar("draw3", "iks_server", 0);
	if (iks_s) {
		iks_server = SC::L2S(iks_s);
		free(iks_s);
	} else {
		iks_server = prefix;
	}

	char *iks_p = libpar_getpar("draw3", "iks_server_port", 0);
	if (iks_p) {
		iks_port = SC::L2S(iks_p);
		free(iks_p);
	}

	return GetIksHandler(iks_server, iks_port);
#endif
}

Draw3Base::ptr SzbaseHandler::GetIksHandler(const wxString& iks_server, const wxString& iks_port)
{
	auto iks_base = new Sz4ApiBase(response_receiver, iks_server.wc_str(), iks_port.wc_str(), ipk_manager->GetIPKContainer());
	if ( !iks_base->BlockUntilConnected() ) {
		return std::shared_ptr<Draw3Base>(nullptr);
	}

	return std::shared_ptr<Draw3Base>(iks_base);
}

Draw3Base::ptr SzbaseHandler::GetBaseHandler(const std::wstring& prefix)
{
#ifndef __WXGTK__
	return default_base_handler;
#else
	std::lock_guard<std::recursive_mutex> guard(libpar_mutex);
	if(use_iks)
		return default_base_handler;

	wxString base_prefix(prefix);
	if(base_prefix.IsEmpty())
		base_prefix = current_prefix;

	if(base_handlers.count(base_prefix) == 0)
		AddBasePrefix(base_prefix);

	return base_handlers[base_prefix];
#endif
}

std::map<std::wstring, std::wstring> SzbaseHandler::GetBaseHandlers() const
{
	std::map<std::wstring, std::wstring> base_handlers_map;
	for (const auto& base_it: base_handlers)
	{
		base_handlers_map[base_it.first.ToStdWstring()] = base_it.second->GetType();
	}

	return base_handlers_map;
}

void SzbaseHandler::AddBasePrefix(const wxString& prefix)
{
#ifdef __WXGTK__
	if(use_iks)
		return;

	std::lock_guard<std::recursive_mutex> guard(libpar_mutex);
	if(base_handlers.count(prefix) && base_handlers[prefix] != nullptr)
	   return;

	ConfigLibpar(prefix);
	int num_of_tries = 5;

	const auto cfg_base_type = libpar_getpar("draw3", "base_type", 0);
	if (cfg_base_type) {
		const std::string base_type(cfg_base_type);
		if (base_type == "sz4" || base_type == "SZ4"|| base_type == "4") {
			base_handlers[prefix] = GetSz4Handler();
		} else if (base_type == "iks"|| base_type == "IKS"|| base_type == "i") {
			auto handler = GetIksHandlerFromBase(prefix);
			base_handlers[prefix] = handler;
			if( handler.get() == nullptr ) {
				wxCommandEvent evt(IKS_CONNECTION_FAILED, 1000);
				evt.SetString(prefix);
				wxPostEvent(response_receiver, evt);
			}
		} else {
			for( int i = 0; i < num_of_tries; ++i )
				if( ipk_manager->PreloadConfig(prefix.ToStdWstring()) )
					break;

			base_handlers[prefix] = GetSz3Handler();
		}
	}
	else {
		for( int i = 0; i < num_of_tries; ++i )
			if( ipk_manager->PreloadConfig(prefix.ToStdWstring()) )
				break;

		base_handlers[prefix] = GetSz3Handler();
	}
#endif
}

void SzbaseHandler::ConfigLibpar(const wxString& prefix)
{
#ifdef __WXGTK__
	std::string config_path = std::string(wxString(base_path + prefix + '/').mb_str());
	libpar_done();
	libpar_init_from_folder(config_path);
#endif
}

void SzbaseHandler::SetupHandlers(const wxString& szarp_dir, const wxString& szarp_data_dir, int cache_size)
{
	base_path = szarp_dir;
	auto sz4 = new Sz4Base(response_receiver,
				szarp_data_dir.wc_str(), ipk_manager->GetIPKContainer());
	sz4_base_handler = std::shared_ptr<Draw3Base>( sz4 );
	auto sz3 = new SzbaseBase(response_receiver, szarp_data_dir.wc_str(), ipk_manager->GetIPKContainer(),
			&ConfigurationFileChangeHandler::handle, cache_size);
	sz3_base_handler = std::shared_ptr<Draw3Base>( sz3 );
}
