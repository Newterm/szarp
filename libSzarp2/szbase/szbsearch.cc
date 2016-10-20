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

#include "config.h"

#include "liblog.h"
#include "szbbase.h"
#include "szbsearch.h"
#include "szbase/szbbuf.h"
#include "proberconnection.h"

time_t 
szb_real_search_probe(szb_buffer_t * buffer, TParam * param, time_t start, time_t end, int direction, SzbCancelHandle * c_handle, const szb_search_condition& condition) {
	if (buffer->PrepareConnection() == false)
		return -1;
	time_t t = buffer->prober_connection->Search(start, end, direction, param->GetSzbaseName());
	if (t == (time_t) -1)
		if (buffer->prober_connection->IsError())
			buffer->prober_connection->ClearError();
	return t;
}

time_t 
szb_combined_search_probe(szb_buffer_t * buffer, TParam * param, time_t start, time_t end, int direction, SzbCancelHandle * c_handle, const szb_search_condition& condition) {
	if (buffer->PrepareConnection() == false)
		return -1;
	TParam ** p_cache = param->GetFormulaCache();
	time_t msw_ret = szb_real_search_probe(buffer, p_cache[0], start, end, direction, c_handle, condition);
	if (msw_ret == (time_t) -1)
		return msw_ret;

	time_t lsw_ret = szb_real_search_probe(buffer, p_cache[1], start, end, direction, c_handle, condition);
	if (lsw_ret == (time_t) -1)
		return lsw_ret;

	if (direction < 0)
		return std::min(lsw_ret, msw_ret);
	else
		return std::max(lsw_ret, msw_ret);
}

class search_timeout_check {
	szb_buffer_t *buffer;
	time_t end_time;
public:
	search_timeout_check(szb_buffer_t *_buffer) : buffer(_buffer), end_time(time_t(-1)) {}
	bool timeout() { 
		if (end_time == time_t(-1)) {
			end_time = time(NULL) + buffer->szbase->GetMaximumSearchTime();
			return false;
		}
		if (end_time > time(NULL))
			return false;
		buffer->last_err = SZBE_SEARCH_TIMEOUT;
		buffer->last_err_string = L"Searching for value timed out";
		return true;
	}
};

time_t search_in_probe_range(szb_buffer_t* buffer, TParam* param, time_t start, time_t end, int direction, const szb_search_condition& condition) {
	if (param->IsConst())
		return start;

	sz_log(10, "Search for value in probes, param: %ls, start: %d, end: %d, direction: %d", param->GetName().c_str(), int(start), int(end), direction);
	if (direction == 0)
		return IS_SZB_NODATA(szb_get_probe(buffer, param, start, PT_SEC10)) ? -1 : start;

	search_timeout_check timeout_check(buffer);
	szb_block_t *block = NULL;
	for (time_t t = start; direction > 0 ? t <= end : t >= end; t += SZBASE_PROBE_SPAN * direction) {
		if (block == NULL || block->GetStartTime() > t || block->GetEndTime() < t) {
			time_t b_start = szb_round_to_probe_block_start(t);
			sz_log(10, "Getting new block at %d", int(b_start));
			block = szb_get_block(buffer, 
						param,
						b_start,
						SEC10_BLOCK);
			if (buffer->last_err != SZBE_OK)
				return -1;
			if (timeout_check.timeout())
				return -1;
		}

		int index = (t - block->GetStartTime()) / SZBASE_PROBE_SPAN;
		if (condition(block->GetData()[index])) {
			sz_log(10, "Data found at %d", int(t));
			return t;
		}
	}
	sz_log(10, "Not data found");
	return -1;
}

bool adjust_search_boundaries(time_t& start, time_t& end, time_t first_date, time_t last_date, int direction) {
	if (first_date == -1 && last_date == -1)
		return false;

	if (direction <= 0 && start < first_date)
		return false;

	if (direction >= 0 && start > last_date)
		return false;

	if (direction > 0 && start < first_date)
		start = first_date;

	if (direction < 0 && start > last_date)
		start = last_date;

	if (end == -1) {
		if (direction < 0)
			end = first_date;
		else
			end = last_date;
	} else {
		if (direction < 0 && end < first_date)
			end = first_date;

		if (direction > 0 && end > last_date)
			end = last_date;
	}

	return true;
}

time_t 
szb_definable_search_probe(szb_buffer_t * buffer, TParam * param, time_t start, time_t end, int direction, SzbCancelHandle * c_handle, const szb_search_condition& condtion) {
	time_t first_date;
	time_t last_date;

	if (buffer->prober_connection->GetRange(first_date, last_date) == false)
		return -1;

	if (adjust_search_boundaries(start, end, first_date, last_date, direction) == false)
		return -1;

	return search_in_probe_range(buffer, param, start, end, direction, condtion);
}

time_t 
szb_lua_search_probe(szb_buffer_t * buffer, TParam * param, time_t start, time_t end, int direction, SzbCancelHandle * c_handle, const szb_search_condition& condtion) {
	time_t first_date, last_date;
	if (!szb_lua_search_first_last_date(buffer, param, PT_SEC10, first_date, last_date))
		return -1;

	if (adjust_search_boundaries(start, end, first_date, last_date, direction) == false)
		return -1;

	return search_in_probe_range(buffer, param, start, end, direction, condtion);
}

time_t search_in_data_left(szb_buffer_t* buffer, TParam* param, time_t start, time_t end, const szb_search_condition& condtion) {
	int year = -1, month = -1;
	szb_datablock_t* block = NULL;
	const SZBASE_TYPE* data = NULL;
	search_timeout_check timeout_check(buffer);
	for (time_t t = start; t >= end; t -= SZBASE_DATA_SPAN) {
		int new_year, new_month;
		szb_time2my(t, &new_year, &new_month);
		if (block == NULL || new_month != month || new_year != year) {
			block = szb_get_datablock(buffer, param, new_year, new_month);
			if (buffer->last_err != SZBE_OK)
				return -1;
			if (timeout_check.timeout())
				return -1;
			if (block != NULL)
				data = block->GetData();
			if (NULL == block || block->GetFirstDataProbeIdx() < 0) {
				t = probe2time(0, new_year, new_month);
				continue;
			}
			year = new_year;
			month = new_month;
		}
		/* check for data at index */
		int probe_n = szb_probeind(t);
		assert(!IS_SZB_NODATA(data[block->GetFirstDataProbeIdx()]));
		assert(!IS_SZB_NODATA(data[block->GetLastDataProbeIdx()]));
		if (probe_n > block->GetLastDataProbeIdx()) { //If index is after last data jump to last data in next iteration
			t = probe2time(block->GetLastDataProbeIdx() + 1, new_year, new_month);
			continue;
		}
		if (probe_n < block->GetFirstDataProbeIdx()) { //If index is before first data jump to the previous block in next iteration
			t = probe2time(0, new_year, new_month);
			continue;
		}
		if (condtion(data[probe_n]))
			return t;
	}
	return -1;
}

time_t search_in_data_right(szb_buffer_t* buffer, TParam* param, time_t start, time_t end, const szb_search_condition& condtion) {
	int year = -1, month = -1;
	szb_datablock_t* block = NULL;
	search_timeout_check timeout_check(buffer);
	const SZBASE_TYPE* data = NULL;
	for (time_t t = start; t <= end; t += SZBASE_DATA_SPAN) {
		int new_year, new_month;
		szb_time2my(t, &new_year, &new_month);
		if (block == NULL || new_month != month || new_year != year) {
			block = szb_get_datablock(buffer, param, new_year, new_month);
			if (buffer->last_err != SZBE_OK)
				return -1;
			if (timeout_check.timeout())
				return -1;
			if (NULL != block)
				data = block->GetData();
			if (NULL == block || block->GetFirstDataProbeIdx() < 0) {
				t = probe2time(szb_probecnt(new_year, new_month) - 1, new_year, new_month);
				continue;
			}
			data = block->GetData();
			year = new_year;
			month = new_month;
		}
		int probe_n = szb_probeind(t);
		assert(probe_n >= 0);
			
		assert(!IS_SZB_NODATA(data[block->GetFirstDataProbeIdx()]));
		assert(!IS_SZB_NODATA(data[block->GetLastDataProbeIdx()]));
		
		if (probe_n > block->GetLastDataProbeIdx()) { //If index is after last data jump to the next block
			t = probe2time(block->max_probes - 1, new_year, new_month);
			continue;
		}
		if (probe_n < block->GetFirstDataProbeIdx()) { //If index is before first data jump first data in next iteration
			t = probe2time(block->GetFirstDataProbeIdx() - 1, new_year, new_month);
			continue;
		}
		if (condtion(data[probe_n]))
			return t;
	}
	return -1;
}

time_t search_in_data_range(szb_buffer_t* buffer, TParam* param, time_t start, time_t end, int direction, const szb_search_condition& condtion) {
	switch (direction) {
		case 0:
			return condtion(szb_get_probe(buffer, param, start, PT_MIN10)) ? start : -1;
		case -1:
			return search_in_data_left(buffer, param, start, end, condtion);
		case 1:
			return search_in_data_right(buffer, param, start, end, condtion);
		default:
			assert(false);
			return -1;
	}
}

time_t szb_real_search_data(szb_buffer_t * buffer, TParam * param, time_t start, time_t end, int direction, const szb_search_condition& condtion) {
	time_t first_date = szb_search_first(buffer, param);
	time_t last_date = szb_search_last(buffer, param);

	if (adjust_search_boundaries(start, end, first_date, last_date, direction) == false)
		return -1;

	return search_in_data_range(buffer, param, start, end, direction, condtion);
}

time_t szb_combined_search_data(szb_buffer_t * buffer, TParam * param, time_t start, time_t end, int direction, const szb_search_condition& condtion) {
	assert(param->GetNumParsInFormula() == 2);
	TParam ** cache = param->GetFormulaCache();
	time_t first_date = szb_search_first(buffer, cache[0]);
	time_t last_date = szb_search_last(buffer, cache[0]);
	if (adjust_search_boundaries(start, end, first_date, last_date, direction) == false)
		return -1;
	return search_in_data_range(buffer, param, start, end, direction, condtion);
}

time_t
szb_definable_search_data(szb_buffer_t * buffer, TParam * param, time_t start, time_t end, int direction, const szb_search_condition& condtion) {
	time_t first_date = buffer->first_av_date; 
	time_t last_date = szb_search_last(buffer, param);
	if (adjust_search_boundaries(start, end, first_date, last_date, direction) == false)
		return -1;
	return search_in_data_range(buffer, param, start, end, direction, condtion);
}


time_t szb_lua_search_data(szb_buffer_t * buffer, TParam * param , time_t start, time_t end, int direction, const szb_search_condition& condtion) {
	time_t first_date, last_date;
	if (!szb_lua_search_first_last_date(buffer, param, PT_MIN10, first_date, last_date))
		return -1;
	if (adjust_search_boundaries(start, end, first_date, last_date, direction) == false)
		return -1;
	return search_in_data_range(buffer, param, start, end, direction, condtion);
}


template<class CMP> time_t do_szb_search(szb_buffer_t* buffer, TParam* param, SZARP_PROBE_TYPE probe_type, time_t start_time, time_t end_time, int direction, const szb_search_condition& condtion) {
	CMP cmp;
	search_timeout_check timeout_check(buffer);
	for (time_t t = start_time; cmp(t, end_time); t = szb_move_time(t, direction, probe_type, 0)) {
		double val = szb_get_probe(buffer, param, t, probe_type, 0);
		if (buffer->last_err != SZBE_OK)
			return -1;
		if (timeout_check.timeout())
			return -1;
		if (condtion(val))
			return t;
	}
	return -1;
}

bool szb_lua_search_first_last_date(szb_buffer_t* buffer, TParam* param, SZARP_PROBE_TYPE probe_type, time_t& first_date, time_t& last_date) {
	switch (probe_type) {
		case PT_SEC10:
			if (buffer->PrepareConnection() == false)
				return false;
			if (buffer->prober_connection->GetRange(first_date, last_date) == false)
				return false;
			if (param->GetLuaStartDateTime() > 0 && param->GetLuaStartDateTime() > first_date)
				first_date = param->GetLuaStartDateTime();
			break;
		default:
			if (param->GetLuaStartDateTime() > 0)
				first_date = param->GetLuaStartDateTime();
			else
				first_date = buffer->first_av_date;
			first_date = szb_round_time(first_date, PT_MIN10, 0);
			last_date = szb_search_last(buffer, param);
			break;
	}
	first_date += param->GetLuaStartOffset(); 
	last_date += param->GetLuaEndOffset();
	return true;
}

time_t szb_lua_search_by_value(szb_buffer_t* buffer, TParam* param, SZARP_PROBE_TYPE probe_type, time_t start_time, time_t end_time, int direction, const szb_search_condition& condtion) {
	time_t first_date;
	time_t last_date;

	if (!szb_lua_search_first_last_date(buffer, param, probe_type, first_date, last_date))
		return -1;

	if (adjust_search_boundaries(start_time, end_time, first_date, last_date, direction) == false)
		return -1;

	switch (direction) {
		case -1:
			return do_szb_search<std::greater_equal<time_t> >(buffer, param, probe_type, start_time, end_time, direction, condtion);
		case 1:
			return do_szb_search<std::less_equal<time_t> >(buffer, param, probe_type, start_time, end_time, direction, condtion);
		case 0:
			return condtion(szb_get_probe(buffer, param, start_time, probe_type)) ? start_time : -1;
		default:
			assert(false);
			return -1;
	}
}

