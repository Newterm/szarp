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
#include "draw.h" 
#include "drawview.h"
#include "drawswdg.h"
#include "infowdg.h"
#include "cconv.h"

#include <algorithm>
#include <utility>

using std::min;
using std::max;
using std::vector;	

/*for year, month, week, day, season*/
const int Draw::PeriodMult[PERIOD_T_LAST] = {1, 1, 3, 6, 1};

void print_time(const wxDateTime &s) {
	wxLogWarning(_T("%s"), s.Format().c_str());
}

void Draw::PrintTime(const DTime& s) {
	wxLogWarning(_T("%s"), s.Format().c_str());
}

ValueInfo::ValueInfo() {
	state = EMPTY;
	val = SZB_NODATA;
	sum = SZB_NODATA;
	db_val = SZB_NODATA;
	max_probes = -1;
	count_probes = 0;
	n_count = 0;
	m_remark = false;
}

bool ValueInfo::IsData() const {
	return state == PRESENT && !IS_SZB_NODATA(val);
}

bool ValueInfo::MayHaveData() const {
	return (state == PRESENT && !IS_SZB_NODATA(val))
		|| state == PENDING
		|| state == NEIGHBOUR_WAIT;
}


Draw::Draw(DrawsWidget* draws_widget, TimeReference* reference, int number) : 
	m_state(STOP),
	m_draw_no(number),
	m_draws_widget(draws_widget), 
	m_draw_info(NULL),
	m_period(PERIOD_T_OTHER),
	m_selected(false),
	m_blocked(false),
	m_index(this->m_period, draws_widget->GetNumberOfValues()),
	m_proposed_time(m_period),
	m_suggested_start_time(m_period),
	m_current_time(m_period),
	m_stats_start_time(m_period),
	m_left_search_result(m_period),
	m_right_search_result(m_period),
	m_values(this),
	m_reference(reference),
	m_enabled(true),
	m_current_index(-1)
{
}

void Draw::RefreshData(bool auto_refresh)
{
	if (m_index.GetStartTime().IsValid() == false)
		return;

	if (!auto_refresh || m_values.m_filter) {
		m_values.clean();
		if (m_selected && m_state == DISPLAY) {
			m_proposed_time = m_current_time;
			EnterState(WAIT_DATA_NEAREST);
		}
	} else 
		for (size_t i = 0; i < m_values.len(); ++i) {
			ValueInfo &v = m_values.Get(i);

			if (v.state == ValueInfo::PRESENT && !v.IsData())
				m_values.ResetValue(i);
		}

	TriggerDataFetch();
}

void Draw::PrintTimeOfIndex(int i) {
	print_time(GetTimeOfIndex(i));
}

void Draw::SetPeriod(PeriodType period, const wxDateTime &current_time) {

    	assert(period != PERIOD_T_OTHER);
	assert(m_state == STOP);

	m_values.SetPeriod(period);

	for (size_t i = 0; i < m_values.len(); ++i)
		m_values.Get(i).state = ValueInfo::EMPTY;

	if (current_time.IsValid()) {
		DTime t(m_period, current_time);

		m_period = period;
		m_index.PeriodChanged(m_draws_widget->GetNumberOfValues());

		if (m_selected) {
			m_current_time = DTime(m_period);
			m_current_index = -1;
			m_proposed_time = SyncWithTimeReference(t);
			m_proposed_time.AdjustToPeriod();
		}
		DTime st = t;
		if (m_draws_widget->IsDefaultNumberOfValues())
			st.AdjustToPeriodSpan();
		SetStartTime(st);
	} else {
		m_period = period;

		m_index.PeriodChanged(m_draws_widget->GetNumberOfValues());
	}


	for (std::vector<DrawObserver*>::iterator i = m_observers.begin();
			i != m_observers.end();
			++i) 
		(*i)->PeriodChanged(this, period);
}

DTime Draw::SyncWithTimeReference(const DTime& time) {
	wxDateTime t = time.GetTime();

	if (t.IsValid() == false)
		return time;


	switch (m_period) {
		default:
			break;
		case PERIOD_T_DAY:
			t.SetMinute(m_reference->m_minute);
			t.SetHour(m_reference->m_hour);
			t.SetDay(m_reference->m_day);
			break;
		case PERIOD_T_WEEK:
			t.SetHour(m_reference->m_hour / 8 * 8);
			t.SetToWeekDayInSameWeek(m_reference->m_wday);
			break;
		case PERIOD_T_MONTH:
			t.SetDay(m_reference->m_day);
			break;
		case PERIOD_T_SEASON:
			//actually do nothing
			break;
	}

	if (!t.IsValid()) {
		DTime ret = time;
		ret.AdjustToPeriodSpan();
		return ret;
	}


	return DTime(m_period, t);
}

void Draw::Stop() {
	EnterState(STOP);
}

bool Draw::GoToTime(const DTime& time) {
	assert(m_period != PERIOD_T_OTHER);

	const DTime& start_time = m_index.GetStartTime();

	if (!start_time.IsValid()) {
		AdjustStartTime(time);
		return true;
	}

	DTime t = time;

	t.AdjustToPeriodStart();

	if (DataFitsCurrentInterval(t))
		return false;

	if (t < start_time) 
		if (t + m_index.GetDatePeriod() + m_index.GetTimePeriod() < start_time)
			AdjustStartTime(time);
		else
			SetStartTime(t);
	else 
		if (t > start_time 
				+ 2 * m_index.GetDatePeriod() 
				+ 2 * m_index.GetTimePeriod())
			AdjustStartTime(t);
		else {
			t = t - m_index.GetTimePeriod() - m_index.GetDatePeriod();
			t = t + PeriodMult[m_period] * m_index.GetTimeRes() + PeriodMult[m_period] * m_index.GetDateRes() ;

			if (t == m_index.GetStartTime())
				assert(false);
			SetStartTime(t);
		}

	return true;
}

bool Draw::DataFitsCurrentInterval(const DTime& time) {

	bool ret;

	const DTime& first_time = m_index.GetStartTime();
	DTime last_time = m_index.GetLastTime(m_draws_widget->GetNumberOfValues());


	if (first_time.IsValid() && last_time.IsValid()) {
		wxLogInfo(_T("Data fits current interval s:%s e:%s d:%s"), 
				first_time.Format().c_str(), 
				last_time.Format().c_str(), 
				time.Format().c_str());
		ret = time.IsBetween(first_time, last_time);
	} else
		ret = false;

	return ret;

}


void Draw::EnterState(STATE state) {
	if (state == m_state)
		return;

	wxLogVerbose(_T("Entering state: %d"), state);

#if 0
	if (m_state == (SEARCH_RIGHT | SEARCH_LEFT | SEARCH_BOTH |
		SEARCH_BOTH_PREFER_RIGHT | SEARCH_BOTH_PREFER_LEFT)) {
		wxGetApp().GetQueryExecutor()->StopSearch();
	}
#endif

	switch (state) {
		case STOP:
			if (m_selected && wxIsBusy())
				wxEndBusyCursor();
			m_state = STOP;
			break;
		case DISPLAY:
			if (m_selected && wxIsBusy())
				wxEndBusyCursor();

			m_state = DISPLAY;
			break;
		case SEARCH_RIGHT:
		case SEARCH_LEFT:
		case SEARCH_BOTH:
		case SEARCH_BOTH_PREFER_RIGHT:
		case SEARCH_BOTH_PREFER_LEFT:
			if (m_selected && !wxIsBusy())
				wxBeginBusyCursor();

			m_got_left_search_response = false;
			m_got_right_search_response = false;
			m_state = state;
			TriggerSearch();
			break;
		case WAIT_DATA_NEAREST:
		case WAIT_DATA_LEFT:
		case WAIT_DATA_RIGHT:
			if (m_selected && !wxIsBusy())
				wxBeginBusyCursor();

			m_state = state;
			CheckData();
			break;
	}

	wxLogVerbose(_T("Entered state: %d"), m_state);

}

void Draw::TriggerSearch() {
	switch (m_state) {
		case SEARCH_LEFT:
			SendSearchQuery((m_proposed_time + m_index.GetTimeRes() + m_index.GetDateRes()).GetTime() - wxTimeSpan::Minutes(10),
					wxInvalidDateTime, -1); 
			break;
		case SEARCH_RIGHT:
			SendSearchQuery(m_proposed_time.GetTime(), wxInvalidDateTime, 1);
			break;
		case SEARCH_BOTH_PREFER_LEFT:
			SendSearchQuery((m_proposed_time.GetTime() + m_index.GetTimeRes() + m_index.GetDateRes()) - wxTimeSpan::Minutes(10), 
					wxInvalidDateTime,
					-1);
			SendSearchQuery(m_proposed_time.GetTime(), 
					m_index.GetStartTime().GetTime() - wxTimeSpan::Minutes(10),
					1);
			break;
		case SEARCH_BOTH_PREFER_RIGHT:
			SendSearchQuery(m_proposed_time.GetTime(), 
					m_index.GetLastTime(m_draws_widget->GetNumberOfValues()).GetTime() + m_index.GetTimeRes() + m_index.GetDateRes(),
					-1);
			SendSearchQuery(m_proposed_time.GetTime(), 
					wxInvalidDateTime, 
					1);
			break;
		case SEARCH_BOTH:
			SendSearchQuery(m_proposed_time.GetTime(), 
					wxInvalidDateTime,
					-1);
			SendSearchQuery(m_proposed_time.GetTime(), 
					wxInvalidDateTime, 
					1);
			break;
		default:
			assert(false);
	}

}

void Draw::SetCurrentDispTime(const DTime& time) {
	if (!m_selected)
		return;

	if (!m_current_time.IsValid() && !time.IsValid())
		return;

	if (m_current_time.IsValid() 
		&& time.IsValid() 
		&& m_current_time == time)
		return;

	int pi = m_current_index;

	m_current_time = time;
	m_current_index = GetIndex(m_current_time);

	int d = 0;

	if (IsDoubleCursor() && m_current_time.IsValid()) {

		assert(m_stats_start_time.IsValid());

		int dt = m_stats_start_time.GetDistance(m_current_time);
		if (dt) {
			m_values.DragStats(dt);
			m_draws_widget->NotifyDragStats(dt);
			m_stats_start_time = m_current_time;
		}

		d = dt;
	}

	for (std::vector<DrawObserver*>::iterator i = m_observers.begin();
			i != m_observers.end();
			++i)
		(*i)->CurrentProbeChanged(this, pi, m_current_index, d);

	if (m_current_time.IsValid())
		UpdateTimeReference(m_current_time);
}

void Draw::StartSelectedDraw() {

	if (DataFitsCurrentInterval(m_proposed_time)) {
		int i = m_index.GetIndex(m_proposed_time, m_draws_widget->GetNumberOfValues());

		assert(i >= 0);

		if (m_values.at(i).state == ValueInfo::EMPTY)
			TriggerDataFetch();

		if (m_values.at(i).IsData()) {
			SetCurrentDispTime(m_proposed_time);
			EnterState(DISPLAY);
		} else {
			EnterState(WAIT_DATA_NEAREST);
		}
	} else {
		EnterState(SEARCH_BOTH);
	}


}

void Draw::MoveToTime(const wxDateTime& time) {
	if (m_selected)
		return;

	Draw* sd = m_draws_widget->GetSelectedDraw();
	bool selected_blocked = false;

	if (sd && sd->GetBlocked())
		selected_blocked = true;

	if (!m_index.GetStartTime().IsValid()
			|| !(m_blocked || selected_blocked))
		SetStartTime(DTime(m_period, time));
}

void Draw::Start() {

	if (m_selected) {
		StartSelectedDraw();
	} else {
		EnterState(DISPLAY);
		TriggerDataFetch();
	}

	assert(m_state != STOP);
}

int Draw::GetIndex(const DTime& time, int* dist) const {

	return m_index.GetIndex(time, m_draws_widget->GetNumberOfValues(), dist);
}


void Draw::SetStartTime(const DTime& time) {

	const DTime& start_time = m_index.GetStartTime();

	if (start_time.IsValid()) {
		if (start_time == time)
			return;

		int d = start_time.GetDistance(time);
		m_values.MoveView(d);

	} else {
		for (size_t i = 0; i < m_values.len(); ++i)
			m_values.Get(i).state = ValueInfo::EMPTY;

	}

	m_index.SetStartTime(time);

	if (m_selected)
		wxLogInfo(_T("Start time set to: %s"), time.Format().c_str());

	if (m_state != STOP)
		TriggerDataFetch();
	else {
		wxLogInfo(_T("Start time set to: %s but we are in stopped state"), time.Format().c_str());
	}
  
	for (std::vector<DrawObserver*>::iterator i = m_observers.begin();
			i != m_observers.end();
			++i) 
		(*i)->ScreenMoved(this, time.GetTime());

}

void Draw::AdjustStartTime(const DTime& time) {
	assert(m_period != PERIOD_T_OTHER);

	DTime start = time;
	if (m_draws_widget->IsDefaultNumberOfValues())
		start.AdjustToPeriodSpan();
	else
		start.AdjustToPeriodStart();

	SetStartTime(start);
		
}

void Draw::UpdateTimeReference(const DTime &time) {
	const wxDateTime& wxt = time.GetTime();
	switch (m_period) {
		case PERIOD_T_DAY:
			m_reference->m_minute = wxt.GetMinute();
			m_reference->m_hour = wxt.GetHour();
		case PERIOD_T_WEEK:
			m_reference->m_wday = wxt.GetWeekDay();
			m_reference->m_hour = wxt.GetHour() / 8 * 8 + m_reference->m_hour % 8;
		case PERIOD_T_MONTH:
		case PERIOD_T_SEASON:
			m_reference->m_day = wxt.GetDay();
		case PERIOD_T_YEAR:
			break;
		default:
			assert(false);
	}
}

const wxDateTime& DTime::GetTime() const {
	return m_time;
}

void DTime::AdjustToPeriodSpan() {
	AdjustToPeriod();
	switch (m_period) {
		case PERIOD_T_YEAR :
			m_time.SetMonth(wxDateTime::Jan);
		case PERIOD_T_MONTH :
			m_time.SetDay(1);
			m_time.SetHour(0);
			break;
		case PERIOD_T_WEEK:
		case PERIOD_T_SEASON :
			m_time.SetToWeekDayInSameWeek(wxDateTime::Mon);
		case PERIOD_T_DAY :
			m_time.SetHour(0);
			m_time.SetMinute(0);
			break;
		case PERIOD_T_OTHER:
		case PERIOD_T_LAST:
			assert(false);
	}
}

DTime Draw::GetDTimeOfIndex(int i) const {
	return m_index.GetTimeOfIndex(i);
}

wxDateTime Draw::GetTimeOfIndex(int i) const {
	return m_index.GetTimeOfIndex(i).GetTime();
}

void Draw::Deselect() {
	switch (m_state) {
		case STOP:
		case DISPLAY:
			break;
		case SEARCH_LEFT:
		case SEARCH_RIGHT:
		case SEARCH_BOTH:
		case SEARCH_BOTH_PREFER_RIGHT:
		case SEARCH_BOTH_PREFER_LEFT:
		case WAIT_DATA_NEAREST:
		case WAIT_DATA_LEFT:
		case WAIT_DATA_RIGHT:
			EnterState(DISPLAY);
			break;
	}

	SetCurrentDispTime(DTime(m_period));

	m_selected = false;

	for (std::vector<DrawObserver*>::iterator i = m_observers.begin();
			i != m_observers.end();
			++i) 
		(*i)->DrawDeselected(this);

}

void Draw::DatabaseResponse(DatabaseQuery *query) {
	assert(m_draw_info);

	if (m_draw_info != query->draw_info) {
		delete query;
		return;
	}

	switch (query->type) {
		case DatabaseQuery::SEARCH_DATA:
			HandleSearchResponse(query);
			break;
		case DatabaseQuery::GET_DATA:
			HandleDataResponse(query);
			break;
		default:
			delete query;
			break;
	}

}

void Draw::HandleSearchResponse(DatabaseQuery *query) {

	if (query->draw_info != m_draw_info) {
		delete query;
		return;
	}

	switch (m_state) {
		case STOP:
		case WAIT_DATA_NEAREST:
		case WAIT_DATA_LEFT:
		case WAIT_DATA_RIGHT:
		case DISPLAY:
			delete query;
			return;
		case SEARCH_LEFT:
		case SEARCH_RIGHT:
		case SEARCH_BOTH:
		case SEARCH_BOTH_PREFER_LEFT:
		case SEARCH_BOTH_PREFER_RIGHT:
			break;
	}

	DatabaseQuery::SearchData& sd = query->search_data;

	wxDateTime dt(sd.start);

	assert(dt.IsValid());

	wxDateTime res;
	if (sd.response != -1)
		res = wxDateTime(sd.response);
	else
		res = wxInvalidDateTime;

	switch (sd.direction) {
		case -1:
			m_got_left_search_response = true;
			m_left_search_result = DTime(m_period, res);
			m_left_search_result.AdjustToPeriod();
			break;
		case 1:
			m_got_right_search_response = true;
			m_right_search_result = DTime(m_period, res);
			m_right_search_result.AdjustToPeriod();
			break;
		default:
			assert(false);
	}

	bool screen_moved = false;
	switch (m_state) {
		case SEARCH_LEFT: 
			if (!m_got_left_search_response)
				break;
			if (m_left_search_result.IsValid()) {
				wxLogInfo(_T("Left search result:%s"), m_left_search_result.Format().c_str());

				m_proposed_time = m_left_search_result;

				screen_moved = GoToTime(m_proposed_time);

				//SetCurrentDispTime(DTime(m_period));
				SetCurrentDispTime(m_proposed_time);

				EnterState(WAIT_DATA_NEAREST);
			} else {
				wxBell();
				EnterState(DISPLAY);
			}
			break;
		case SEARCH_RIGHT:
			if (!m_got_right_search_response)
				break;
			if (m_right_search_result.IsValid()) {
				wxLogInfo(_T("Right search result:%s"), m_right_search_result.Format().c_str());

				m_proposed_time = m_right_search_result;

				wxLogInfo(_T("Right search result after adjustment:%s"), m_proposed_time.Format().c_str());

				screen_moved = GoToTime(m_proposed_time);

				//SetCurrentDispTime(DTime(m_period));
				SetCurrentDispTime(m_proposed_time);

				EnterState(WAIT_DATA_NEAREST);
			} else {
				wxBell();
				EnterState(DISPLAY);
			}
			break;
		case SEARCH_BOTH_PREFER_LEFT:
		case SEARCH_BOTH_PREFER_RIGHT:
		case SEARCH_BOTH: {
			if (!m_got_right_search_response || !m_got_left_search_response)
				break;

			if (!m_left_search_result.IsValid() && !m_right_search_result.IsValid()) {
				EnterState(DISPLAY);
				//search failed and no data is displayed so apparently param has
				//not any data
				if (!m_current_time.IsValid()) {
					m_param_has_data = false;
					m_draws_widget->ParamHasNoData();
					m_current_index = -1;
				} 
				break;
			}

			/*choose time that is closer to propsed time*/
			wxTimeSpan d1;
			if (m_left_search_result.IsValid()) 
				d1 = m_proposed_time.GetTime() - m_left_search_result.GetTime();

			wxTimeSpan d2;
			if (m_right_search_result.IsValid()) 
				d2 = m_right_search_result.GetTime() - m_proposed_time.GetTime();

			if (!m_left_search_result.IsValid())
				m_proposed_time = m_right_search_result;
			else if (!m_right_search_result.IsValid())
				m_proposed_time = m_left_search_result;
			else
				m_proposed_time = (d1 < d2) ? m_left_search_result : m_right_search_result;

			wxLogInfo(_T("Search both found data at time: %s"), m_proposed_time.Format().c_str());

			//check if we can set start time to desired value
			if (m_proposed_time.IsBetween(m_suggested_start_time, 
					m_suggested_start_time 
					+ m_index.GetTimePeriod()  
					+ m_index.GetDatePeriod() 
					- m_index.GetTimeRes()
					- m_index.GetDateRes())) {

				SetStartTime(m_suggested_start_time);
				screen_moved = true;
			} else {
				screen_moved = GoToTime(m_proposed_time);
			}

			//SetCurrentDispTime(DTime(m_period));
			SetCurrentDispTime(m_proposed_time);

			wxLogInfo(_T("Search both start time is: %s, end time:%s"), 
					m_index.GetStartTime().Format().c_str(), m_index.GetLastTime(m_draws_widget->GetNumberOfValues()).Format().c_str());

			EnterState(WAIT_DATA_NEAREST);
			break;
		}

		default:
			assert(false);
			break;
	}

	delete query;

	if (screen_moved)
		m_draws_widget->NotifyScreenMoved(m_index.GetStartTime().GetTime());

}

void Draw::HandleDataResponse(DatabaseQuery *query) {
	bool view_changed = false;

	m_values.AddData(query, view_changed);

	delete query;

	if (!view_changed)
		return;

	switch (m_state) {
		case STOP:
		case SEARCH_LEFT:
		case SEARCH_RIGHT:
		case SEARCH_BOTH:
		case SEARCH_BOTH_PREFER_RIGHT:
		case SEARCH_BOTH_PREFER_LEFT:
		case DISPLAY:
			break;
		case WAIT_DATA_NEAREST:
		case WAIT_DATA_LEFT:
		case WAIT_DATA_RIGHT:
			CheckData();
			break;
	}

}

void Draw::MoveCursorRight(int n) {
	assert(m_period != PERIOD_T_OTHER);

	if (m_state != DISPLAY)
		return;
	m_proposed_time = m_current_time + n * m_index.GetDateRes() + n * m_index.GetTimeRes();

	EnterState(WAIT_DATA_RIGHT);

}

void Draw::MoveCursorLeft(int n) {
	assert(m_period != PERIOD_T_OTHER);

	if (m_state != DISPLAY)
		return;

	m_proposed_time = m_current_time - n * m_index.GetTimeRes() - n * m_index.GetDateRes();

	EnterState(WAIT_DATA_LEFT);
}

void Draw::CheckData() {
	assert(m_period != PERIOD_T_OTHER);
	int proposed_time_index = GetIndex(m_proposed_time);

	int max_index = m_values.size();

	switch (m_state) {
		case WAIT_DATA_LEFT: {
			if (proposed_time_index < 0) {
				EnterState(SEARCH_LEFT);
				break;
			}
			int i;
			if (m_values.at(proposed_time_index).state == ValueInfo::EMPTY) {
				TriggerDataFetch();
				break;
			}

			for (i = proposed_time_index; i >= 0; --i) 
				if (m_values.at(i).MayHaveData())
					break;

			if (i < 0) 
				EnterState(SEARCH_LEFT);
			else if (m_values.at(i).IsData()) {
				m_proposed_time = GetDTimeOfIndex(i);
				EnterState(DISPLAY);
				SetCurrentDispTime(m_proposed_time);
			}
			break;
		}
		case WAIT_DATA_RIGHT: {
			if (proposed_time_index < 0) {
				EnterState(SEARCH_RIGHT);
				break;
			}
			if (m_values.at(proposed_time_index).state == ValueInfo::EMPTY) {
				TriggerDataFetch();
				break;
			}

			int i;
			for (i = proposed_time_index; i < max_index; ++i) 
				if (m_values.at(i).MayHaveData())
					break;

			if (i >= max_index) 
				EnterState(SEARCH_RIGHT);
			else if (m_values.at(i).IsData()) {
				m_proposed_time = GetDTimeOfIndex(i);
				SetCurrentDispTime(m_proposed_time);
				EnterState(DISPLAY);
			}
			break;
		}

		case WAIT_DATA_NEAREST: {
			if (proposed_time_index < 0) {
				EnterState(SEARCH_BOTH);
				break;
			}
			if (m_values.at(proposed_time_index).state == ValueInfo::EMPTY) {
				TriggerDataFetch();
				break;
			}

			int i,j;
			for (i = proposed_time_index, j = proposed_time_index;
				i >= 0 || j < max_index;
				--i, ++j) {

				if (i >= 0 && m_values.at(i).MayHaveData())
					break;

				if (j < max_index && m_values.at(j).MayHaveData())
					break;
			}

			if (i < 0 && j >= max_index) {
				EnterState(SEARCH_BOTH);
				break;
			}

			if (i >= 0 && m_values.at(i).IsData()) {
				m_proposed_time = GetDTimeOfIndex(i);
				SetCurrentDispTime(m_proposed_time);
				EnterState(DISPLAY);
			} else if (j < max_index && m_values.at(j).IsData()) {
				m_proposed_time = GetDTimeOfIndex(j);
				SetCurrentDispTime(m_proposed_time);
				EnterState(DISPLAY);
			} 

			break;
		}
		default:
			assert(false);

	}
}

void Draw::SendCleanQuery(const wxDateTime& start, const wxDateTime& end){
#if 0
	time_t t1 = start.IsValid() ? start.GetTicks() : -1;
	time_t t2 = end.IsValid() ? end.GetTicks() : -1;
	
	if(t1<t2)
	{
		assert(m_draw_info);
		TParam *param = m_draw_info->GetParam();
		assert(param);
	
		DatabaseQuery *q = new DatabaseQuery();
		q->type = DatabaseQuery::CLEANER;
		q->param = param;
		q->draw_no = 0;
		q->search_data.start = t1;
		q->search_data.end = t2;
		q->search_data.direction = 0;
		m_draws_widget->CleanDatabase(q);
	}
#endif

}

void Draw::SendSearchQuery(const wxDateTime& start, const wxDateTime& end, int direction) {
	time_t t1 = start.IsValid() ? start.GetTicks() : -1;
	time_t t2 = end.IsValid() ? end.GetTicks() : -1;

	wxString str(_T("Searching "));
	if (direction > 0)
		str << _T("right ");
	else
		str << _T("left ");

	if (start.IsValid())
		str << _T("start ") << start.Format() << _T(" ");

	if (end.IsValid())
		str << _T("end ") << end.Format() << _T(" ");

	wxLogInfo(_T("%s"), str.c_str());
	
	assert(m_draw_info);

	DatabaseQuery *q = new DatabaseQuery();
	q->type = DatabaseQuery::SEARCH_DATA;
	q->draw_info = m_draw_info;
	q->draw_no = m_draw_no;
	q->search_data.start = t1;
	q->search_data.end = t2;
	q->search_data.period_type = m_period;
	q->search_data.direction = direction;
	m_draws_widget->QueryDatabase(q);

}

void Draw::TriggerDataFetch() {
	assert(m_period != PERIOD_T_OTHER);

	if (!m_index.GetStartTime().IsValid())
		return;

	int d = m_values.m_view.Start();

	DatabaseQuery* q = CreateDataQuery(m_draw_info, m_period, m_draw_no);

	bool no_max_probes = m_draw_info->GetParam()->GetIPKParam()->GetFormulaType() == TParam::LUA_AV;
	for (size_t i = 0; i < m_values.len(); ++i) {
		ValueInfo &v = m_values.Get(i);

		if (v.state != ValueInfo::EMPTY) 
			continue;
		
		DTime pt = GetDTimeOfIndex(i - d);
		if (m_period == PERIOD_T_DAY)
			v.max_probes = 1;
		else if (no_max_probes)
			v.max_probes = 0;
		else {
			DTime ptn = pt + m_index.GetDateRes() + m_index.GetTimeRes();
			v.max_probes = (ptn.GetTime() - pt.GetTime()).GetMinutes() / 10;
		}

		v.state = ValueInfo::PENDING;
		
		AddTimeToDataQuery(q, pt.GetTime().GetTicks());
	}

	if (q->value_data.vv->size())
		m_draws_widget->QueryDatabase(q);
	else
		delete q;
}

void Draw::SetDraw(DrawInfo *info) {
	assert(m_state == STOP);
	m_param_has_data = true;
	m_draw_info = info;
	for (size_t i = 0; i < m_values.len(); ++i) {
		m_values.Get(i).state = ValueInfo::EMPTY;
		m_values.Get(i).n_count = 0;
	}
	m_values.RecalculateStats();

	if (m_selected)
		SetCurrentDispTime(DTime(m_period));
	for (std::vector<DrawObserver*>::iterator i = m_observers.begin();
			i != m_observers.end();
			++i) 
		(*i)->DrawInfoChanged(this);

}

int Draw::GetCurrentIndex() const {
	if (!m_selected)
		return -1;

	if (m_current_index == -1)
		return GetIndex(m_proposed_time);
	else
		return m_current_index;

}

const wxDateTime& Draw::GetCurrentTime() const {
	return m_current_time.IsValid() ? m_current_time.GetTime() : m_proposed_time.GetTime();
}

DrawInfo* Draw::GetDrawInfo() const {
	return m_draw_info;
}

void Draw::Select(const wxDateTime &time) {
	assert (m_state == STOP);
	m_proposed_time = DTime(m_period, time);
	m_proposed_time.AdjustToPeriod();

	bool ws = m_selected;
	m_selected = true;

	if (!ws) {
		for (std::vector<DrawObserver*>::iterator i = m_observers.begin();
				i != m_observers.end();
				++i) 
		(*i)->DrawSelected(this);
	}
}

void Draw::SetCurrentTime(const wxDateTime &time) {
	m_proposed_time = DTime(m_period, time);
	m_proposed_time.AdjustToPeriod();
	EnterState(WAIT_DATA_NEAREST);
}

void Draw::MoveCursorBegin() {
	assert(m_selected);

	if (m_state != DISPLAY)
		return;

	m_proposed_time = m_index.GetStartTime();

	EnterState(WAIT_DATA_NEAREST);
}

void Draw::MoveCursorEnd() {
	assert(m_selected);

	if (m_state != DISPLAY)
		return;

	m_proposed_time = m_index.GetLastTime(m_draws_widget->GetNumberOfValues());

	EnterState(WAIT_DATA_NEAREST);
}

void Draw::MoveScreenLeft() {
	assert(m_selected);

	if (m_state != DISPLAY)
		return;

	m_suggested_start_time = m_index.GetStartTime() - m_index.GetDatePeriod() - m_index.GetTimePeriod();

	m_proposed_time = m_current_time - m_index.GetDatePeriod() - m_index.GetTimePeriod();

	EnterState(SEARCH_BOTH_PREFER_LEFT);

}

void Draw::MoveScreenRight() {
	assert(m_selected);

	if (m_state != DISPLAY)
		return;
	
	m_suggested_start_time = m_index.GetStartTime() + m_index.GetDatePeriod() + m_index.GetTimePeriod();

	m_proposed_time = m_current_time + m_index.GetDatePeriod() + m_index.GetTimePeriod();

	EnterState(SEARCH_BOTH_PREFER_RIGHT);

}

const Draw::VT& Draw::GetValuesTable() const {
	return m_values;
}

const wxDateTime& Draw::GetDisplayedTime() const {
	return m_current_time.GetTime();
}

void Draw::AttachObserver(DrawObserver *observer) {
	m_observers.push_back(observer);
}

void Draw::DetachObserver(DrawObserver *observer) {

	vector<DrawObserver*>::iterator i;

	i = std::remove(m_observers.begin(),
		m_observers.end(), 
		observer);

	m_observers.erase(i, m_observers.end());
}

const PeriodType& Draw::GetPeriod() const {
	return m_period;
}

bool Draw::GetEnable() const {
	return m_enabled;
}

void Draw::SetEnable(bool enable) {
	if (enable == m_enabled)
		return;
	m_enabled = enable;
	for (std::vector<DrawObserver*>::iterator i = m_observers.begin();
			i != m_observers.end();
			++i) 
		(*i)->EnableChanged(this);

}

int Draw::GetDrawNo() const {
	return m_draw_no;
}

bool Draw::HasNoData() const {
	return !m_param_has_data;
}

bool Draw::GetBlocked() const {
	return m_blocked;
}

void Draw::SetBlocked(bool blocked) {
	m_blocked = blocked;
}

bool Draw::GetSelected() const {
	return m_selected;
}

void Draw::DoubleCursorSet(int idx) {
	if (m_selected)
		return;

	m_values.EnableDoubleCursor(idx);
	m_stats_start_time = GetDTimeOfIndex(idx);
}

void Draw::DoubleCursorStopped() {
	if (m_selected)
		return;

	m_values.DisableDoubleCursor();
}

void Draw::DragStats(int disp) {
	if (m_selected)
		return;
	
	m_stats_start_time = m_stats_start_time 
		+ m_index.GetTimeRes() * disp 
		+ m_index.GetDateRes() * disp;

	m_values.DragStats(disp);
}


bool Draw::IsDoubleCursor() {
	return m_values.m_double_cursor;
}

bool Draw::StartDoubleCursor() {
	assert(m_selected);

	if (m_state != DISPLAY)
		return false;

	m_values.EnableDoubleCursor(m_current_index);	
	m_stats_start_time = GetDTimeOfIndex(m_current_index);
	m_draws_widget->NotifyDoubleCursorEnabled(m_current_index);

	return true;

}

void Draw::StopDoubleCursor() {
	assert(m_selected);

	m_values.DisableDoubleCursor();
	m_draws_widget->NotifyDoubleCursorDisabled();

}

void Draw::SetFilter(int filter) {
	m_values.SetFilter(filter);
	for (std::vector<DrawObserver*>::iterator it = m_observers.begin();
			it != m_observers.end();
			++it)  {
		(*it)->FilterChanged(this);
	}
	TriggerDataFetch();

}

bool Draw::IsTheNewestValue() {

	Szbase* szbase = Szbase::GetObject();

	bool ok;
	std::wstring param_name(this->m_draw_info->GetBasePrefix().c_str());
	param_name += L":";
	param_name += m_draw_info->GetParamName().c_str();
	time_t newest = szbase->SearchLast(param_name, ok);

	if(!ok)
		return false;

	return szb_round_time(newest,PeriodToProbeType(m_period), 0) 
		== szb_round_time(m_current_time.GetTime().GetTicks(), PeriodToProbeType(m_period), 0);
}

void Draw::SetRemarksTimes(std::vector<wxDateTime>& times) {
	bool new_added = false;
	for (std::vector<wxDateTime>::iterator i = times.begin();
			i != times.end();
			i++) {

		DTime dt(m_period, *i);

		int idx = GetIndex(dt);
		if (idx >= 0) {
			ValueInfo& vi = m_values.at(idx);
			if (vi.m_remark == false) {
				vi.m_remark = true;
				new_added = true;
			}
		}
	}
	for (std::vector<DrawObserver*>::iterator i = m_observers.begin();
			i != m_observers.end();
			++i)
		(*i)->NewRemarks(this);
}

DTime::DTime(const PeriodType& period, const wxDateTime& time) : m_period(period), m_time(time)
{
}

const PeriodType& DTime::GetPeriod() const {
	return m_period;
}

void DTime::AdjustToPeriodStart() {

	if (m_time.IsValid() == false)
		return;

	switch (m_period) {
		case PERIOD_T_YEAR:
			m_time.SetDay(1);
		case PERIOD_T_MONTH:
		case PERIOD_T_WEEK:
			m_time.SetHour(0);
			m_time.SetMinute(0);
			break;
		case PERIOD_T_DAY: {
			wxDateTime tmp = m_time;
			m_time.SetMinute(0);

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
}

void DTime::AdjustToPeriod() {

	if (m_time.IsValid() == false)
		return;
    
	switch (m_period) {
		case PERIOD_T_YEAR :
			m_time = wxDateTime(1, m_time.GetMonth(), m_time.GetYear());
			break;
		case PERIOD_T_MONTH :
			m_time = wxDateTime(m_time.GetDay(), m_time.GetMonth(), m_time.GetYear());
			break;
		case PERIOD_T_WEEK :
			m_time = m_time;
			m_time.SetHour((m_time.GetHour()/8) * 8);
			m_time.SetMinute(0);
			m_time.SetSecond(0);
			m_time.SetMillisecond(0);
			break;
		case PERIOD_T_DAY : {
			wxDateTime tmp = m_time;
			m_time.SetMinute(m_time.GetMinute() / 10 * 10);

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
			break;
		case PERIOD_T_LAST:
			assert(false);
	}
}


DTime& DTime::operator=(const DTime& time) {
	if (this != &time) {
		m_time = time.m_time;
	}

	return *this;
}

DTime DTime::operator+(const wxTimeSpan& span) const {

	if (!m_time.IsValid() || m_period == PERIOD_T_OTHER)
		return DTime(m_period, wxInvalidDateTime);
		
	switch (m_period) {
		//in following periods we are not interested in time resolution
		case PERIOD_T_YEAR:
		case PERIOD_T_MONTH: 
			return DTime(m_period, m_time);
			break;
		//in this case will just follow UTC
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
	return t1 <= *this && t2 >= *this;
}

bool DTime::IsValid() const {
	return m_time.IsValid();
}

int DTime::GetDistance(const DTime &t) const {
	assert(IsValid());
	assert(t.IsValid());

	assert(GetPeriod() == t.GetPeriod());

	const wxDateTime& _t0 = GetTime();
	const wxDateTime& _t = t.GetTime();

	int ret = -1;

	switch (GetPeriod()) {
		case PERIOD_T_YEAR:
			ret = _t.GetYear() * 12 + _t.GetMonth()
				- _t0.GetYear() * 12 - _t0.GetMonth();
			break;
		case PERIOD_T_MONTH: 
			ret = (_t - _t0).GetDays();
			if (_t0.IsDST() != _t.IsDST()) {
				if (_t0 < _t && _t.IsDST())
					ret += 1;
				else if (_t < _t0 && _t0.IsDST())
					ret -= 1;
			}
			break;
		case PERIOD_T_DAY:
			ret = (_t - _t0).GetMinutes() / 10;
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

DTime TimeIndex::GetTimeOfIndex(int i) const {
	return m_time + i * m_dateres + i * m_timeres;
}

wxString DTime::Format(const wxChar* format) const {
	return m_time.Format(format);
}

TimeIndex::TimeIndex(const PeriodType& pt, size_t max_values) : m_time(pt) {
	PeriodChanged(max_values);
}

const DTime& TimeIndex::GetStartTime() const {
	return m_time;
}

int TimeIndex::GetIndex(const DTime& time, size_t max_idx, int *dist) const {
	int i, d;
	if (!m_time.IsValid() || !time.IsValid()) {
		i = -1, d = 0;
	} else {
		i = d = m_time.GetDistance(time);

		if (d < 0 || d >= (int)max_idx)
			i = -1;
	}
	if (dist)
		*dist = d;

	return i;
}

DTime TimeIndex::GetLastTime(size_t values_count) const {
	return GetTimeOfIndex(values_count - 1);
}

void TimeIndex::PeriodChanged(size_t values_count) {
	const PeriodType& p = m_time.GetPeriod();

	switch (p) {
		case PERIOD_T_YEAR :
			m_timeres = wxTimeSpan(0, 0, 0, 0);
			m_dateres = wxDateSpan::Month();
			m_timeperiod = wxTimeSpan(0, 0, 0, 0);
			m_dateperiod = wxDateSpan::Month() * (values_count / Draw::PeriodMult[p]);
			break;
		case PERIOD_T_MONTH :
			m_timeres = wxTimeSpan(0, 0, 0, 0);
			m_dateres = wxDateSpan::Day();
			m_timeperiod = wxTimeSpan(0, 0, 0, 0);
			m_dateperiod = values_count == 31 ? wxDateSpan::Month() : wxDateSpan::Day() * (values_count / Draw::PeriodMult[p]);
			break;
		case PERIOD_T_WEEK :
			m_timeres = wxTimeSpan(8, 0, 0, 0);
			m_dateres = wxDateSpan(0);
			m_timeperiod = wxTimeSpan(0, 0, 0, 0);
			m_dateperiod = wxDateSpan::Day() * (values_count / Draw::PeriodMult[p]);
			break;
		case PERIOD_T_DAY :
			m_timeres = wxTimeSpan(0, 10, 0, 0);
			m_dateres = wxDateSpan(0);
			m_timeperiod = wxTimeSpan(1, 0, 0, 0) * (values_count / Draw::PeriodMult[p]);
			m_dateperiod = wxDateSpan(0);
			break;
		case PERIOD_T_SEASON :
			m_timeres = wxTimeSpan(24 * 7, 0, 0, 0);
			m_dateres = wxDateSpan(0);
			m_timeperiod = wxTimeSpan(0, 0, 0, 0);
			m_dateperiod = wxDateSpan::Week() * (values_count / Draw::PeriodMult[p]);
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

void TimeIndex::SetStartTime(const DTime &time) {
	m_time = time;
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

ValuesTable::ValuesTable(Draw *draw) {
     m_draw = draw;
     m_double_cursor = false;
     m_filter = 0;
}

ValueInfo& ValuesTable::at(size_t i) {
	return m_values.at(i + m_view.Start());
}

const ValueInfo& ValuesTable::at(size_t i) const {
	return m_values.at(i + m_view.Start());
}

ValueInfo& ValuesTable::operator[](size_t i) {
	return m_values.at(i + m_view.Start());
}

const ValueInfo& ValuesTable::operator[](size_t i) const {
	return m_values.at(i + m_view.Start());
}

size_t ValuesTable::size() const {
	return m_draw->m_draws_widget->GetNumberOfValues();
}

size_t ValuesTable::len() const {
	return m_values.size();
}

void ValuesTable::ClearStats() {
	m_count = 0;
	m_probes_count = 0;
	m_data_probes_count = 0;

	m_sum = 
	m_max = 
	m_min = 
	m_hsum = 
	m_data_probes_ratio = 
		nan("");
}	

void ValuesTable::RecalculateStats() {
	ClearStats();

	size_t s,e;
	s = m_stats.Start(); 
	e = m_stats.End();

	for (size_t i = s; i <= e; ++i) 
		UpdateStats(i);
}

void ValuesTable::SetPeriod(PeriodType pt) {
	size_t s = m_draw->m_draws_widget->GetNumberOfValues();

	assert(m_double_cursor == false);
	m_values.clear();
	m_values.resize(s + 2 * m_filter);
	m_view.m_start = m_filter;
	m_view.m_end = s - 1 + m_filter;

	m_stats.m_start = m_filter;
	m_stats.m_end = s - 1 + m_filter;

	ClearStats();
}

Interval Interval::Union(const Interval &i) const {
	Interval ret;

	ret.m_start = std::min(Start(), i.Start());
	ret.m_end = std::max(End(), i.End());
	
	return ret;

}

void Interval::Drag(int d) {
	m_start += d;
}

void Interval::Move(int dr) {
	this->m_start += dr;
	this->m_end += dr;
}

int Interval::Start() const {
	return std::min(m_start, m_end);
}

int Interval::End() const {
	return std::max(m_start, m_end);
}

void ValuesTable::PopFront() {
	ValueInfo &vi = m_values.front();
	if (vi.state != ValueInfo::PRESENT && 
			vi.state != ValueInfo::NEIGHBOUR_WAIT) {
		m_values.pop_front();
		return;
	}

	for (int i = 1; i < m_filter && i < (int)m_values.size(); ++i) {
		ValueInfo& vi = m_values.at(i);
		vi.n_count--;
	}

	m_values.pop_front();
}

void ValuesTable::PopBack() {
	ValueInfo &vi = m_values.back();
	if (vi.state != ValueInfo::PRESENT && 
			vi.state != ValueInfo::NEIGHBOUR_WAIT) {
		m_values.pop_back();
		return;
	}

	for (int i = m_values.size() - 1; i > (int)m_values.size() - m_filter - 1 && i >= 0; --i) {
		ValueInfo & vi = m_values.at(i);
		vi.n_count--;
	}

	m_values.pop_back();
	
}

void ValuesTable::PushBack() {
	ValueInfo nvi;
	for (int i = m_values.size() - 1; i > (int)m_values.size() - m_filter - 1 && i >= 0; --i) {
		ValueInfo & vi = m_values.at(i);
		if (vi.state == ValueInfo::PRESENT 
			|| vi.state == ValueInfo::NEIGHBOUR_WAIT)
			nvi.n_count++;
	}

	m_values.push_back(nvi);

}

void ValuesTable::PushFront() {
	ValueInfo nvi;
	for (int i = 0; i < m_filter && i < (int)m_values.size(); ++i) {
		ValueInfo & vi = m_values.at(i);
		if (vi.state == ValueInfo::PRESENT 
			|| vi.state == ValueInfo::NEIGHBOUR_WAIT)
			nvi.n_count++;
	}

	m_values.push_front(nvi);
}

void ValuesTable::Move(const Interval &in) {
	if (in.End() < 0 || in.Start() >= (int)m_values.size()) {
		m_values.resize(0);
		m_values.resize(in.End() - in.Start() + 1);
		return;
	}

	while ((in.End() + 1) < (int)m_values.size())
		PopBack();

	for (int i = 0; i < in.Start(); ++i)
		PopFront();

	for (int i = 0; i > in.Start(); --i)
		PushFront();

	for (int i = m_values.size(); i <= in.End() - in.Start(); ++i) 
		PushBack();
}

void ValuesTable::UpdateStats(int idx) {
	const ValueInfo& v = m_values[idx];

	if (v.state != ValueInfo::PRESENT)
		return;

	m_probes_count += v.max_probes;
	m_data_probes_count += v.count_probes;
	if (m_probes_count)
		m_data_probes_ratio = double(m_data_probes_count) / m_probes_count;

	if (!v.IsData())
		return;

	const double& val = v.val;

	if (m_count) {
		m_max = std::max(val, m_max);
		m_min = std::min(val, m_min);
		m_sum += val;
		m_hsum += v.sum / m_draw->GetDrawInfo()->GetSumDivisor();
	} else {
		m_max = m_min = val;
		m_sum = val;
		m_hsum = v.sum / m_draw->GetDrawInfo()->GetSumDivisor();
	}

	m_count++;
}

void ValuesTable::CalculateProbeValue(int index) {
	double sum = 0.0;
	int count = 0;
	for (int i = - m_filter + index; i <= m_filter + index; ++i) {
		if (i < 0 || i >= (int)m_values.size())
			continue;
		ValueInfo &vi = m_values.at(i);

		if (!IS_SZB_NODATA(vi.db_val)) {
			sum += vi.db_val;
			count++;
		}
	}

	ValueInfo &vi = m_values.at(index);

	if (count) 
		vi.val = IS_SZB_NODATA(vi.db_val) ? SZB_NODATA : sum / count;
	else
		vi.val = SZB_NODATA;

	wxLogInfo(_T("Calcualted value of probe %d that is %f"), index, vi.val);
}


void ValuesTable::AddData(DatabaseQuery *q, bool &view_values_changed) {
	bool stats_updated = false;

	DatabaseQuery::ValueData& vd = q->value_data;
	PeriodType pt = vd.period_type;
	if (pt != m_draw->GetPeriod() || q->draw_info != m_draw->GetDrawInfo()) {
		return;
	}

	for (std::vector<DatabaseQuery::ValueData::V>::iterator i = q->value_data.vv->begin();
			i != q->value_data.vv->end();
			i++)
		InsertValue(*i, view_values_changed, stats_updated);

	if (stats_updated) 
		NotifyStatsChange();
}

void ValuesTable::InsertValue(DatabaseQuery::ValueData::V &v, bool &view_values_changed, bool& stats_updated) {
	DTime dt(m_draw->GetPeriod(), wxDateTime(v.time));

	int view_index = m_draw->m_index.GetStartTime().GetDistance(dt);
	int index = view_index + m_view.Start();

	if (index < 0 || index >= (int)m_values.size()) 
		return;

	if (m_values.at(index).IsData())
		return;

	ValueInfo& vi = m_values.at(index);

	vi.db_val = v.response;
	vi.sum = v.sum;
	vi.count_probes = v.count;

	DataInserted(index, stats_updated, view_values_changed);
}

void ValuesTable::DataInserted(int index, bool &stats_updated, bool &view_updated) {

	for (int i = index - m_filter ; i <= m_filter + index; ++i) {
		if (i < 0 || i >= (int)m_values.size())
			continue;

		ValueInfo& n = m_values.at(i);
		if (i == index) {
			if (n.n_count != 2 * m_filter) {
				n.state = ValueInfo::NEIGHBOUR_WAIT;
				continue;
			}
		} else {
			++n.n_count;
			if (n.state != ValueInfo::NEIGHBOUR_WAIT || n.n_count < 2 * m_filter)
				continue;
		}
	
		n.state = ValueInfo::PRESENT;

		CalculateProbeValue(i);

		if (i >= m_view.Start() && i <= m_view.End()) {
			for (std::vector<DrawObserver*>::iterator it = m_draw->m_observers.begin();
					it != m_draw->m_observers.end();
					++it) 
				(*it)->NewData(m_draw, i - m_view.Start());

			view_updated = true;
		}

		if (i >= m_stats.Start() && i <= m_stats.End()) {
			UpdateStats(i);
			stats_updated = true;
		}
	}

}

void ValuesTable::MoveView(int d) { 

	m_view.Move(d);
	
	if (!m_double_cursor)
		m_stats.Move(d);

	Interval in = m_view.Union(m_stats);
	Interval fin = in;
	fin.m_start = fin.Start() - m_filter;
	fin.m_end = fin.End() + m_filter;

	Move(fin);

	m_view.Move(-in.Start() + m_filter);
	m_stats.Move(-in.Start() + m_filter);

	if (!m_double_cursor) {
		RecalculateStats();
		NotifyStatsChange();
	}

}

void ValuesTable::DragStats(int d) {
	if (!m_double_cursor)
		return;

	m_stats.Drag(d);

	Interval in = m_view.Union(m_stats);
	Interval fin = in;
	fin.m_start -= m_filter;
	fin.m_end += m_filter;
	Move(fin);

	m_view.Move(-in.Start() + m_filter);
	m_stats.Move(-in.Start() + m_filter);

	RecalculateStats();

	NotifyStatsChange();

}

void ValuesTable::EnableDoubleCursor(int idx) {
	m_double_cursor = true;
	m_stats.m_start = m_stats.m_end = idx + m_filter;
	RecalculateStats();
	NotifyStatsChange();
}

void ValuesTable::DisableDoubleCursor() {
	for (int i = m_filter; i < m_view.m_start; i++)
		m_values.pop_front();

	m_view.m_end = m_filter + (m_view.m_end - m_view.m_start);
	m_view.m_start = m_filter;

	m_stats.m_start = m_view.m_start;
	m_stats.m_end = m_view.m_end;

	m_double_cursor = false;
	RecalculateStats();
	NotifyStatsChange();
}

ValueInfo& ValuesTable::Get(size_t i) {
	return m_values.at(i);
}

const ValueInfo& ValuesTable::Get(size_t i) const {
	return m_values.at(i);
}

void ValuesTable::NotifyStatsChange() {
	for (std::vector<DrawObserver*>::iterator it = m_draw->m_observers.begin();
			it != m_draw->m_observers.end();
			++it) 
		(*it)->StatsChanged(m_draw);
}

void ValuesTable::SetFilter(int f) {
	int df = f - m_filter;

	m_view.m_start += df;
	m_view.m_end += df;

	m_stats.m_start += df;
	m_stats.m_end += df;

	for (size_t i = 0; i < m_values.size(); ++i) 
		m_values[i].n_count = 0;

	if (df > 0) 
		while (df--) {
			m_values.push_back(ValueInfo());
			m_values.push_front(ValueInfo());
		}
	else 
		while (df++) {
			m_values.pop_back();
			m_values.pop_front();
		}

	m_filter = f;

	ClearStats();

	bool stats_updated, view_updated;
	for (size_t i = 0; i < m_values.size(); ++i) {
		ValueInfo& v = m_values.at(i);
		if (v.state != ValueInfo::EMPTY && v.state != ValueInfo::PENDING)
			DataInserted(i, stats_updated, view_updated);
	}

}

void ValuesTable::clean() {
	size_t size = m_values.size();
	m_values.resize(0);
	m_values.resize(size);
	RecalculateStats();
}

void ValuesTable::ResetValue(int i) {
	ValueInfo& v = m_values.at(i);
	m_values.at(i).state = ValueInfo::EMPTY;
	if (m_stats.Start() <= i && i <= m_stats.End()) {
		m_probes_count -= v.max_probes;
		if (m_probes_count)
			m_data_probes_ratio = double(m_data_probes_count) / m_probes_count;
		else
			m_data_probes_ratio = nan("");
	}
}
