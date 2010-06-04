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
#include <wx/config.h>
#include "kopervfetch.h"

#include <string>
#include <algorithm>
#include <sstream>
#include <assert.h>
#include "szbase/szbdefines.h"
#include "cconv.h"
#include <limits>

using std::vector;
using std::pair;
using std::string;

template<typename T,typename V> bool is_set(T s, V v) {
	bool result = false;
	if ((1 << s) & v)
		result = true;
	return result;
}

vector<string> split_string(const std::string& str) {
	vector<string> r;
	std::istringstream ss(str);
	string s;
	while (ss >> s)
		r.push_back(s);
	return r;

}

template<typename T> int first_bit_set(T val) {
	int ret = 0;
	T maxval = std::numeric_limits<T>::max();
	T bitmask = maxval - (maxval >> 1);

	for (;!(bitmask & val) && bitmask;bitmask>>=1);
	for (;bitmask;bitmask>>=1) ret += 1;

	return ret;

}

class TimeFinder {
	static bool FindMin(const TimeSpec& s, struct tm* result, const struct tm* t) {
		ExecTimeType sm = s.minute;
		int val;
		while ((val = first_bit_set(sm) - 1) >= 0) { 
			if (val * 10 <= t->tm_min) {
				result->tm_min = val * 10;
				result->tm_sec = 0;
				return true;
			}
			sm &= ~(((ExecTimeType)1) << val);
		}
		return false;
	}

	static bool FindHour(TimeSpec s, struct tm* result, const struct tm* t) {
		ExecTimeType sh = s.hour;
		int val;
		while ((val = first_bit_set(sh) - 1) >= 0) { 
			if (val <= t->tm_hour) {
				result->tm_hour = val;
				if (FindMin(s, result, t))
					return true;
			}
			sh &= ~(((ExecTimeType)1) << val);
		}
		return false;
	}

	static bool FindDay(const TimeSpec s, struct tm* result, const struct tm* t) {
		ExecTimeType sd = s.day;
		int val;
		while ((val = first_bit_set(sd)) > 0) { 
			if (val < szb_daysinmonth(result->tm_year + 1900, result->tm_mon + 1) 
				&& (val <= t->tm_mday)) {
				result->tm_mday = val;
				if (FindHour(s, result, t))
					return true;
			}
			sd &= ~(((ExecTimeType)1) << (val - 1));
		}
		return false;
	}

	static bool FindMonth(const TimeSpec& s, struct tm* result, const struct tm* t) {
		ExecTimeType sm = s.month;
		int val;
		while ((val = first_bit_set(sm) - 1) >= 0) {
			if (val <= t->tm_mon) {
				result->tm_mon = val;
				if (FindDay(s, result, t))
					return true;
			}
			sm &= ~(((ExecTimeType)1) << val);
		}
		return false;
	}

	static bool FindPreviousYear(const TimeSpec& s, struct tm* result, const tm* t) {
		struct tm py;

		result->tm_year = py.tm_year = t->tm_year - 1;
		py.tm_mon = 11;
		py.tm_mday = 31;
		py.tm_hour = 23;
		py.tm_min = 50;
		py.tm_isdst = 0;

		return FindMonth(s, result, &py);
	}
public:
	static bool FindStartTime(const TimeSpec& s, struct tm* result, time_t t) {
		struct tm* pct;
#if __WXGTK__
		struct tm ct;
		assert(
			pct = localtime_r(&t, &ct)
		);
#else
		assert(
			pct = localtime(&t)
		);
#endif

		result->tm_year = pct->tm_year;
		result->tm_isdst = -1;

		if (FindMonth(s, result, pct))
			return true;

		return FindPreviousYear(s, result, pct);
	}

};

KoperValueFetcher::KoperValueFetcher() :
	m_szbbuf(NULL),
	m_config(NULL),
	m_param(NULL),
	m_init(false)
{}

KoperValueFetcher::~KoperValueFetcher() {
	if (m_szbbuf)
		szb_free_buffer(m_szbbuf);
	delete m_config;
}

bool KoperValueFetcher::ParseTime(TimeSpec *s, const std::string& t) {
	vector<string> v = split_string(t);
	if (v.size() != 4) {
		sz_log(0, "Invalid time specification, wrong numer of componenets");
		return false;

	}

	if (parse_cron_time(v[0].c_str(), &s->minute, 5, 0, 1)) {
		sz_log(0, "Wrong minute specification");
		goto error;
	}

	if (parse_cron_time(v[1].c_str(), &s->hour, 23, 0)) {
		sz_log(0, "Wrong hour specification");
		goto error;
	}

	if (parse_cron_time(v[2].c_str(), &s->day, 31, 1)) {
		sz_log(0, "Wrong day specification");
		goto error;
	}

	if (parse_cron_time(v[3].c_str(), &s->month, 12, 1)) {
		sz_log(0, "Wrong month specification %s", v[3].c_str());
		goto error;
	}
	

	return true;
error:

	return false;
}

bool KoperValueFetcher::Setup(vector<string>& ts, time_t ct) {


	for (size_t i = 0; i < ts.size(); ++i) {
		ValueInfo v;

		if (!ParseTime(&v.ts, ts[i]))
			return false;

		v.val = SZB_NODATA;
		v.ival = SZB_NODATA;
		v.tendency = NONE;
		v.lft = -1;

		m_vt.push_back(v);
	}

	time_t ld = szb_search(m_szbbuf, m_param, ct, -1, -1);

	time_t pld = (time_t) -1;

	if (ld != (time_t) -1) {
		pld = szb_search(m_szbbuf, m_param, ld - SZBASE_DATA_SPAN, -1, -1);
		if (pld != (time_t) -1)
			AdjustToTime(pld);
		AdjustToTime(ld);
	}

	return true;

}

bool KoperValueFetcher::SetTime(time_t ct) {

	ct = szb_round_time(ct, PT_MIN10, 0);
	for (size_t i = 0; i < m_vt.size(); ++i) {
		bool r = TimeFinder::FindStartTime(m_vt[i].ts, &m_vt[i].t, ct);
		if (!r) {
			sz_log(0, "Unable to find a start time for a given time specification");
			assert(false);
		}
	}
	return true;

}

SZBASE_TYPE KoperValueFetcher::GetHoursum(time_t st, time_t en) {
	assert (st <= en);

	SZBASE_TYPE ret = 0, sum = 0;

	en = szb_move_time(en, 1, PT_MIN10, 1);

	ret = szb_get_avg(m_szbbuf, m_param, st, en, &sum);

	if (IS_SZB_NODATA(ret))
		return nan("");

	return -sum / 6;

}

void KoperValueFetcher::AdjustToTime(time_t t) {
	bool r = SetTime(t);
	assert(r);

	t = szb_round_time(t, PT_MIN10, 0);

	Szbase::GetObject()->NextQuery();

	time_t ld = szb_search(m_szbbuf, m_param, t, -1, -1);
	ld = szb_round_time(ld , PT_MIN10, 0);

	
	if (ld == (time_t) -1) 
		return;

	for (size_t i = 0; i < m_vt.size(); ++i) {
		ValueInfo& vi = m_vt[i];

		time_t st = mktime(&vi.t);
		assert(st != (time_t) -1);

		if (st > ld) {
			vi.val = SZB_NODATA;
			vi.ival = SZB_NODATA;
			vi.tendency = NONE;
			continue;
		}

		if (vi.lft >= ld)
			continue;

		SZBASE_TYPE ival, val;

		ival = -szb_get_data(m_szbbuf, m_param, ld);

		ival = roundf(ival * pow10(m_param->GetPrec())) / pow10(m_param->GetPrec());

		val = GetHoursum(st, ld);

		if (!IS_SZB_NODATA(ival) 
			&& !IS_SZB_NODATA(vi.ival)) {

			if (ival > vi.ival)
				vi.tendency = INCREASING;
			else if (ival < vi.ival)
				vi.tendency = DECREASING;	
			else 
				vi.tendency = NONE;	
		}
		
		vi.val = val;
		vi.ival = ival;
		vi.lft = ld;
	}

}

int KoperValueFetcher::GetValuePrec() {
	return m_param->GetPrec();
}

double KoperValueFetcher::Value(size_t index) {
	return m_vt.at(index).val;
}

const struct tm& KoperValueFetcher::Time(size_t index) {
	return m_vt.at(index).t;
}

Tendency KoperValueFetcher::GetTendency() {
	Tendency t = NONE;

	for (size_t i = 0; i < m_vt.size(); ++i) {
		t = m_vt[i].tendency;
		if (t != NONE)
			break;
	}

	return t;
}

bool KoperValueFetcher::Init(const std::string& szarp_dir, 
		const std::string& prefix, 
		const std::string& param_name, 
		const std::string& date1,
		const std::string& date2) {
	assert(!m_init);

	vector<string> dv;

	std::string szbase_dir;

	IPKContainer::Init(SC::A2S(szarp_dir), SC::A2S(szarp_dir), L"", new NullMutex());
	m_config = IPKContainer::GetObject()->GetConfig(SC::A2S(prefix));
	if (m_config == NULL)  {
		sz_log(0, "Failed to load configuration %s", prefix.c_str());
		goto fail2;
	}

	for (m_param = m_config->GetFirstParam(); m_param; m_param = m_config->GetNextParam(m_param)) 
		if (SC::A2S(param_name) == m_param->GetName())
			break;

	if (!m_param) {
		sz_log(0, "Param %s not found in configuration %s", param_name.c_str(), prefix.c_str());
		goto fail2;
	}
			
	Szbase::Init(SC::A2S(szarp_dir));
	szbase_dir = string(szarp_dir) + prefix + "/szbase";
	m_szbbuf = szb_create_buffer(Szbase::GetObject(), SC::A2S(szbase_dir), 500, m_config);

	if (!m_szbbuf) {
		sz_log(0, "Failed to init database from dir %s", szbase_dir.c_str());
		goto fail2;
	}

	dv.push_back(date1);
	dv.push_back(date2);
	if (!Setup(dv, time(NULL))) {
		sz_log(0, "Failed to find initial time for params caluclation%s", szbase_dir.c_str());
		goto fail1;
	}

	m_init = true;

	return true;

fail1:	
	szb_free_buffer(m_szbbuf);
	m_szbbuf = NULL;
fail2:
	delete m_config;
	m_config = NULL;
	
	return false;

}
