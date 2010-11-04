#include "config.h"

#include <cmath>
#include <cfloat>
#include <functional>
#include <boost/variant.hpp>

#include "conversion.h"
#include "liblog.h"
#include "lua_syntax.h"
#include "szbbase.h"
#include "loptdatablock.h"
#include "loptcalculate.h"

#ifdef LUA_PARAM_OPTIMISE
LuaOptDatablock::LuaOptDatablock(szb_buffer_t * b, TParam * p, int y, int m) : LuaDatablock(b, p, y, m) {
	AllocateDataMemory();
}

void LuaOptDatablock::FinishInitialization() {

	exec_param = param->GetLuaExecParam();

	if (year < buffer->first_av_year)
		NOT_INITIALIZED;
	if (year == buffer->first_av_year && month < buffer->first_av_month)
		NOT_INITIALIZED;
	int year, month;
	time_t end_date = szb_search_last(buffer, param);
	szb_time2my(end_date, &year, &month);
	if (this->year > year || (this->year == year && this->month > month))
		NOT_INITIALIZED;
	if (end_date > GetEndTime())
		end_date = GetEndTime();
	int probes_to_compute = szb_probeind(end_date) + 1;
	for (int i = probes_to_compute; i < max_probes; i++)
		data[i] = SZB_NODATA;

	last_update_time = szb_round_time(buffer->GetMeanerDate(), PT_MIN10, 0);
	if (LoadFromCache()) {
		Refresh();
	} else {
		LuaExec::ExecutionEngine ee(buffer, param->GetLuaExecParam());
		CalculateValues(&ee, probes_to_compute);
	}
}

void LuaOptDatablock::CalculateValues(LuaExec::ExecutionEngine *ee, int end_probe) {
	time_t t = probe2time(fixed_probes_count, year, month);

	for (int i = fixed_probes_count; i < end_probe; i++, t += SZBASE_DATA_SPAN) {
		bool probe_fixed = true;
		ee->CalculateValue(t, PT_MIN10, data[i], probe_fixed);
		if (!std::isnan(data[i])) {
			last_data_probe_index = i;
			if (first_data_probe_index < 0)
				first_data_probe_index = i;
		}
		if (probe_fixed && fixed_probes_count == i)
			fixed_probes_count = i + 1;
		
	}
}

void LuaOptDatablock::Refresh() {
	if (fixed_probes_count == max_probes)
		return;

	bool refresh = false;
	for (std::map<szb_buffer_t*, time_t>::iterator i = exec_param->m_last_update_times.begin();
			i != exec_param->m_last_update_times.end();
			i++) {
		time_t meaner_time = szb_round_time(i->first->GetMeanerDate(), PT_MIN10, 0);
		time_t& update_time = i->second;
		if (i->first == buffer)
			last_update_time = meaner_time;
		if (update_time != meaner_time) {
			update_time = meaner_time;
			refresh = true;
		}
	}
	
	if (!refresh)
		return;

	time_t end_date = szb_search_last(buffer, param);
	if (end_date > GetEndTime())
		end_date = GetEndTime();

	int end_probe = szb_probeind(end_date) + 1;
	LuaExec::ExecutionEngine ee(buffer, param->GetLuaExecParam());
	CalculateValues(&ee, end_probe);
	last_update_time = szb_round_time(buffer->GetMeanerDate(), PT_MIN10, 0);
}

#endif

#if LUA_PARAM_OPTIMISE
LuaDatablock *create_lua_data_block(szb_buffer_t *b, TParam* p, int y, int m) {
	LuaExec::Param *ep = p->GetLuaExecParam();
	if (ep == NULL) {
		ep = LuaExec::optimize_lua_param(p);
		b->AddExecParam(p);
	}
	if (ep->m_optimized)
		return new LuaOptDatablock(b, p, y, m);
	else
		return new LuaNativeDatablock(b, p, y, m);


}
#else
LuaDatablock *create_lua_data_block(szb_buffer_t *b, TParam* p, int y, int m) {
	return new LuaNativeDatablock(b, p, y, m);
}
#endif
