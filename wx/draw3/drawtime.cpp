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

#include <wx/wx.h>

#include "ids.h"
#include "drawtime.h"

const size_t TimeIndex::default_units_count[PERIOD_T_LAST] = { 10, 12, 31, 7, 24, 30, 40 };
/*for year, month, week, day, season*/
const int TimeIndex::PeriodMult[PERIOD_T_LAST] = {1, 1, 1, 3, 6, 6, 1};

DTime::DTime(const PeriodType& period, const wxDateTime& time) : m_period(period), m_time(time)
{
}

const wxDateTime& DTime::GetTime() const {
	return m_time;
}


const PeriodType& DTime::GetPeriod() const {
	return m_period;
}

DTime& DTime::AdjustToPeriodStart() {

	if (m_time.IsValid() == false)
		return *this;

	switch (m_period) {
		case PERIOD_T_DECADE:
			m_time.SetMonth(wxDateTime::Jan);
		case PERIOD_T_YEAR:
			m_time.SetDay(1);
		case PERIOD_T_MONTH:
		case PERIOD_T_WEEK:
			m_time.SetHour(0);
			m_time.SetMinute(0);
			m_time.SetSecond(0);
			break;
		case PERIOD_T_30MINUTE:
			m_time.SetSecond(0);
			break;
		case PERIOD_T_DAY: {
			wxDateTime tmp = m_time;
			m_time.SetMinute(0);
			m_time.SetSecond(0);

			//yet another workaround for bug in wxDateTime
			if (tmp.IsDST() && !m_time.IsDST())
				m_time -= wxTimeSpan::Hours(1);
			else if (!tmp.IsDST() && m_time.IsDST())
				m_time += wxTimeSpan::Hours(1);

			break;
		}
		case PERIOD_T_SEASON :
			m_time.SetToWeekDayInSameWeek(wxDateTime::Mon);
			break;
		case PERIOD_T_OTHER:
		default:
			break;
		}

	return *this;
}

DTime& DTime::SetPeriod(PeriodType pt) {
	m_period = pt;
	return AdjustToPeriod();
}

DTime& DTime::AdjustToPeriod() {

	if (m_time.IsValid() == false)
		return *this;
    
	switch (m_period) {
		case PERIOD_T_DECADE :
			m_time = wxDateTime(1, wxDateTime::Jan, m_time.GetYear());
			break;
		case PERIOD_T_YEAR :
			m_time = wxDateTime(1, m_time.GetMonth(), m_time.GetYear());
			break;
		case PERIOD_T_MONTH :
			m_time = wxDateTime(m_time.GetDay(), m_time.GetMonth(), m_time.GetYear());
			break;
		case PERIOD_T_WEEK :
			m_time.SetHour((m_time.GetHour()/8) * 8);
			m_time.SetMinute(0);
			m_time.SetSecond(0);
			m_time.SetMillisecond(0);
			break;
		case PERIOD_T_DAY : {
			wxDateTime tmp = m_time;
			m_time.SetMinute(m_time.GetMinute() / 10 * 10);
			m_time.SetSecond(0);

			//yet another workaround for bug in wxDateTime
			if (tmp.IsDST() && !m_time.IsDST())
				m_time -= wxTimeSpan::Hours(1);
			else if (!tmp.IsDST() && m_time.IsDST())
				m_time += wxTimeSpan::Hours(1);

			break;

		}
		case PERIOD_T_30MINUTE : {
			wxDateTime tmp = m_time;
			m_time.SetSecond(m_time.GetSecond() / 10 * 10);

			//yet another workaround for bug in wxDateTime
			if (tmp.IsDST() && !m_time.IsDST())
				m_time -= wxTimeSpan::Hours(1);
			else if (!tmp.IsDST() && m_time.IsDST())
				m_time += wxTimeSpan::Hours(1);

			break;

		}
		case PERIOD_T_SEASON :
			m_time.SetToWeekDayInSameWeek(wxDateTime::Mon);
			m_time.SetHour(0);
			m_time.SetMinute(0);
			m_time.SetSecond(0);
			m_time.SetMillisecond(0);
			break;
		case PERIOD_T_OTHER:
		case PERIOD_T_LAST:
			assert(false);
	}

	return *this;
}


DTime& DTime::operator=(const DTime& time) {
	if (this != &time) {
		m_time = time.m_time;
		m_period = time.m_period;
	}

	return *this;
}

DTime DTime::operator+(const wxTimeSpan& span) const {

	if (!m_time.IsValid() || m_period == PERIOD_T_OTHER)
		return DTime(m_period, wxInvalidDateTime);
		
	switch (m_period) {
		//in following periods we are not interested in time resolution
		case PERIOD_T_DECADE:
		case PERIOD_T_YEAR:
		case PERIOD_T_MONTH: 
			return DTime(m_period, m_time);
			break;
		//in this case will just follow UTC
		case PERIOD_T_30MINUTE: 
		case PERIOD_T_DAY: 
			return DTime(m_period, m_time + span);
			break;
		case PERIOD_T_WEEK:
		case PERIOD_T_SEASON: {
			wxDateTime t = m_time + span;					   
			if (t.IsDST() != m_time.IsDST())
				t += ( m_time.IsDST() ? 1 : -1 ) * wxTimeSpan::Hour();
			return DTime(m_period, t);
			break;
		}
		default:
			assert(false);
			break;
	}

}

DTime DTime::operator-(const wxTimeSpan& span) const {
	return operator+(span.Negate());
}

DTime DTime::operator+(const wxDateSpan& span) const {
	//note - this if is neccessary because result of addition of empty
	//wxDateSpan to 02:00 CET at the daylight saving time boundary 
	//magically becomes 02:00 CEST...
	if (span.GetYears() || span.GetMonths() || span.GetWeeks() || span.GetDays())
		return DTime(m_period, m_time + span);
	else
		return DTime(m_period, m_time);
}

DTime DTime::operator-(const wxDateSpan& span) const {
	if (span.GetYears() || span.GetMonths() || span.GetWeeks() || span.GetDays())
		return DTime(m_period, m_time - span);
	else
		return DTime(m_period, m_time);
}

bool DTime::operator==(const DTime& t) const {
	return m_time == t.m_time;
}

bool DTime::operator!=(const DTime& t) const {
	return m_time != t.m_time;
}

bool DTime::operator<=(const DTime& t) const {
	return m_time <= t.m_time;
}

bool DTime::operator<(const DTime& t) const {
	return m_time < t.m_time;
}


bool DTime::operator>(const DTime& t) const {
	return m_time > t.m_time;
}

bool DTime::operator>=(const DTime& t) const {
	return m_time >= t.m_time;
}

bool DTime::IsBetween(const DTime& t1, const DTime& t2) const {
	if (t1.IsValid() && t2.IsValid())
		return t1 <= *this && t2 > *this;
	else
		return false;
}

bool DTime::IsValid() const {
	return m_time.IsValid();
}

int DTime::GetDistance(const DTime &t) const {
	assert(IsValid());
	assert(t.IsValid());

	if (GetPeriod() != t.GetPeriod()) {
		wxLogError(_T("our period: %d, his period: %d"), GetPeriod(), t.GetPeriod());
		assert(false);
	}

	const wxDateTime& _t0 = GetTime();
	const wxDateTime& _t = t.GetTime();

	int ret = -1;

	switch (GetPeriod()) {
		case PERIOD_T_DECADE:
			ret = _t.GetYear() - _t0.GetYear();
			break;
		case PERIOD_T_YEAR:
			ret = _t.GetYear() * 12 + _t.GetMonth()
				- _t0.GetYear() * 12 - _t0.GetMonth();
			break;
		case PERIOD_T_MONTH: 
			ret = (_t - _t0).GetHours();
			//acount for daylight saving time
			switch (ret % 24) {
				case 1:
					ret -= 1;
					break;
				case 23:
					ret += 1;
					break;
				case -1:
					ret += 1;
					break;
				case -23:
					ret -= 1;
					break;
			}
			ret /= 24;
			break;
		case PERIOD_T_DAY:
			ret = (_t - _t0).GetMinutes() / 10;
			break;
		case PERIOD_T_30MINUTE:
			ret = ((_t - _t0).GetSeconds() / 10).ToLong();
			break;
		case PERIOD_T_WEEK:
			ret = (_t - _t0).GetHours() / 8;
			if (_t0.IsDST() != _t.IsDST()) {
				if (_t0 < _t && _t.IsDST())
					ret += 1;
				else if (_t < _t0 && _t0.IsDST())
					ret -= 1;
			}
			break;
		case PERIOD_T_SEASON: 
			ret = (_t - _t0).GetDays() / 7;
			if (_t0.IsDST() != _t.IsDST()) {
				if (_t0 < _t && _t.IsDST())
					ret += 1;
				else if (_t < _t0 && _t0.IsDST())
					ret -= 1;
			}
			break;
		default:
			assert(false);
	}

	return ret;
}

wxString DTime::Format(const wxChar* format) const {
	return m_time.Format(format);
}

DTime::operator const wxDateTime&() {
	return m_time;
}

DTime TimeIndex::GetTimeOfIndex(int i) const {
	return m_time + i * m_dateres + i * m_timeres;
}

TimeIndex::TimeIndex(const PeriodType& pt) : m_time(pt) {
	m_number_of_values = default_units_count[pt];
	UpdatePeriods();
}

TimeIndex::TimeIndex(const PeriodType& pt, size_t max_values) : m_time(pt) {
	m_number_of_values = max_values;
	UpdatePeriods();
}

const DTime& TimeIndex::GetStartTime() const {
	return m_time;
}

int TimeIndex::GetIndex(const DTime& time, int *dist) const {
	int i, d;
	if (!m_time.IsValid() || !time.IsValid()) {
		i = -1, d = 0;
	} else {
		i = d = m_time.GetDistance(time);

		if (d < 0 || d >= (int)m_number_of_values)
			i = -1;
	}
	if (dist)
		*dist = d;

	return i;
}

DTime TimeIndex::GetLastTime() const {
	return GetTimeOfIndex(m_number_of_values - 1);
}

DTime TimeIndex::GetFirstNotDisplayedTime() const {
	return GetTimeOfIndex(m_number_of_values);
}

void TimeIndex::SetNumberOfValues(size_t values_count) {
	m_number_of_values = values_count;
	UpdatePeriods();
}

void TimeIndex::UpdatePeriods() {

	const PeriodType& p = m_time.GetPeriod();

	switch (p) {
		case PERIOD_T_DECADE :
			m_timeres = wxTimeSpan(0, 0, 0, 0);
			m_dateres = wxDateSpan::Year();
			m_timeperiod = wxTimeSpan(0, 0, 0, 0);
			m_dateperiod = wxDateSpan::Year() * (m_number_of_values / TimeIndex::PeriodMult[p]);
			break;
		case PERIOD_T_YEAR :
			m_timeres = wxTimeSpan(0, 0, 0, 0);
			m_dateres = wxDateSpan::Month();
			m_timeperiod = wxTimeSpan(0, 0, 0, 0);
			m_dateperiod = wxDateSpan::Month() * (m_number_of_values / TimeIndex::PeriodMult[p]);
			break;
		case PERIOD_T_MONTH :
			m_timeres = wxTimeSpan(0, 0, 0, 0);
			m_dateres = wxDateSpan::Day();
			m_timeperiod = wxTimeSpan(0, 0, 0, 0);
			m_dateperiod = m_number_of_values == default_units_count[PERIOD_T_MONTH] ?
					wxDateSpan::Month() : wxDateSpan::Day() * (m_number_of_values / TimeIndex::PeriodMult[p]);
			break;
		case PERIOD_T_WEEK :
			m_timeres = wxTimeSpan(8, 0, 0, 0);
			m_dateres = wxDateSpan(0);
			m_timeperiod = wxTimeSpan(0, 0, 0, 0);
			m_dateperiod = wxDateSpan::Day() * (m_number_of_values / TimeIndex::PeriodMult[p]);
			break;
		case PERIOD_T_DAY :
			m_timeres = wxTimeSpan(0, 10, 0, 0);
			m_dateres = wxDateSpan(0);
			m_timeperiod = wxTimeSpan(1, 0, 0, 0) * (m_number_of_values / TimeIndex::PeriodMult[p]);
			m_dateperiod = wxDateSpan(0);
			break;
		case PERIOD_T_30MINUTE :
			m_timeres = wxTimeSpan(0, 0, 10, 0);
			m_dateres = wxDateSpan(0);
			m_timeperiod = wxTimeSpan(0, 1, 0, 0) * (m_number_of_values / TimeIndex::PeriodMult[p]);
			m_dateperiod = wxDateSpan(0);
			break;
		case PERIOD_T_SEASON :
			m_timeres = wxTimeSpan(24 * 7, 0, 0, 0);
			m_dateres = wxDateSpan(0);
			m_timeperiod = wxTimeSpan(0, 0, 0, 0);
			m_dateperiod = wxDateSpan::Week() * (m_number_of_values / TimeIndex::PeriodMult[p]);
			break;
		default:
			m_timeres = wxTimeSpan(0, 0, 0, 0);
			m_dateres = wxDateSpan(0);
			m_timeperiod = wxTimeSpan(0, 0, 0, 0);
			m_dateperiod = wxDateSpan(0);
			break;
	}
	
	m_time.AdjustToPeriodStart();
}

DTime TimeIndex::AdjustToPeriodSpan(const DTime &time) const {
	wxDateTime dt = time.GetTime();

	if (m_number_of_values == default_units_count[time.GetPeriod()] * PeriodMult[time.GetPeriod()]) {
		switch (time.GetPeriod()) {
			case PERIOD_T_DECADE :
			case PERIOD_T_YEAR :
				dt.SetMonth(wxDateTime::Jan);
			case PERIOD_T_MONTH :
				dt.SetDay(1);
				dt.SetHour(0);
				break;
			case PERIOD_T_WEEK:
			case PERIOD_T_SEASON :
				dt.SetToWeekDayInSameWeek(wxDateTime::Mon);
			case PERIOD_T_DAY :
				dt.SetHour(0);
				dt.SetMinute(0);
				break;
			case PERIOD_T_30MINUTE :
				dt.SetMinute(dt.GetMinute() / 30 * 30);
				dt.SetSecond(0);
				break;
			case PERIOD_T_OTHER:
			case PERIOD_T_LAST:
				assert(false);
		}
		return DTime(time.GetPeriod(), dt);
	} else {
		DTime ret(time);
		return ret.AdjustToPeriodStart();
	}
}

void TimeIndex::SetStartTime(const DTime &time) {
	m_time = time;
}

void TimeIndex::SetStartTime(const DTime &time, size_t number_of_values) {
	m_number_of_values = number_of_values;
	m_time = AdjustToPeriodSpan(time);

	UpdatePeriods();
}

const wxTimeSpan& TimeIndex::GetTimeRes() const {
	return m_timeres;
}

const wxDateSpan& TimeIndex::GetDateRes() const {
	return m_dateres;
}

const wxTimeSpan& TimeIndex::GetTimePeriod() const {
	return m_timeperiod;
}

const wxDateSpan& TimeIndex::GetDatePeriod() const {
	return m_dateperiod;
}

