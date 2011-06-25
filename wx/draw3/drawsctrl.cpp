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

/*
 * draw3 program
 * SZARP
 
 * $Id: draw.h 11 2009-07-15 09:16:00Z reksi0 $
 */

#include "classes.h"
#include "ids.h"
#include "dbinquirer.h"
#include "database.h"
#include "drawtime.h"
#include "drawobs.h"
#include "coobs.h"
#include "drawsctrl.h"
#include "remarks.h"
#include "drawobs.h"
#include "draw.h"
#include "drawswdg.h"
#include "cfgmgr.h"

static const size_t MAXIMUM_NUMBER_OF_INITIALLY_ENABLED_DRAWS = 12;

szb_nan_search_condition DrawsController::search_condition;

DrawsController::DrawsController(ConfigManager *config_manager, DatabaseManager *database_manager) :
	DBInquirer(database_manager), 
	m_state(STOP),
	m_time_reference(wxDateTime::Now()),
	m_config_manager(config_manager),
	m_got_left_search_response(false),
	m_got_right_search_response(false),
	m_left_search_result(PERIOD_T_OTHER),
	m_right_search_result(PERIOD_T_OTHER),
	m_suggested_start_time(PERIOD_T_OTHER),
	m_current_time(PERIOD_T_OTHER),
	m_time_to_go(PERIOD_T_OTHER),
	m_selected_draw(-1),
	m_period_type(PERIOD_T_OTHER),
	m_active_draws_count(0),
	m_draw_set(NULL),
	m_current_index(-1),
	m_double_cursor(false),
	m_follow_latest_data_mode(false),
	m_filter(0)
{
	m_config_manager->RegisterConfigObserver(this);

	m_units_count[PERIOD_T_DECADE] = TimeIndex::default_units_count[PERIOD_T_DECADE];
	m_units_count[PERIOD_T_YEAR] = TimeIndex::default_units_count[PERIOD_T_YEAR];
	m_units_count[PERIOD_T_MONTH] = TimeIndex::default_units_count[PERIOD_T_MONTH];
	m_units_count[PERIOD_T_WEEK] = TimeIndex::default_units_count[PERIOD_T_WEEK];
	m_units_count[PERIOD_T_DAY] = TimeIndex::default_units_count[PERIOD_T_DAY];
	m_units_count[PERIOD_T_30MINUTE] = TimeIndex::default_units_count[PERIOD_T_30MINUTE];
	m_units_count[PERIOD_T_SEASON] = TimeIndex::default_units_count[PERIOD_T_SEASON];

	m_period_type = PERIOD_T_OTHER;

	DatabaseQuery* q = new DatabaseQuery;
	q->type = DatabaseQuery::CHECK_CONFIGURATIONS_CHANGE;
	q->draw_info = NULL;
	QueryDatabase(q);
}

DrawsController::~DrawsController() {
	for (size_t i = 0; i < m_draws.size(); i++)
		delete m_draws[i];

	m_config_manager->DeregisterConfigObserver(this);
}

void DrawsController::DisableDisabledDraws() {
	bool any_disabled_draw_present = false;
	for (std::vector<Draw*>::iterator i = m_draws.begin(); i != m_draws.end(); i++) {
		DrawInfo *di = (*i)->GetDrawInfo();
		if (di != NULL &&
				m_disabled_draws.find(std::make_pair(di->GetSetName(), std::make_pair(di->GetName(), (*i)->GetDrawNo()))) 
					!= m_disabled_draws.end()) {
			if (m_selected_draw != int(std::distance(m_draws.begin(), i))) 
				(*i)->SetEnable(false);
			any_disabled_draw_present = true;
		} else {
			(*i)->SetEnable(true);
		}
	}
	
	if (!any_disabled_draw_present) for (size_t i = MAXIMUM_NUMBER_OF_INITIALLY_ENABLED_DRAWS; i < m_draws.size(); i++) {
		DrawInfo *di = m_draws[i]->GetDrawInfo();
		m_draws[i]->SetEnable(false);
		if (di == NULL)
			continue;
		m_disabled_draws[std::make_pair(di->GetSetName(), std::make_pair(di->GetName(), m_draws[i]->GetDrawNo()))] = true;
	}
}

void DrawsController::CheckAwaitedDataPresence() {
	assert(m_selected_draw >= 0);

	Draw* draw = m_draws[m_selected_draw];
	int current_index = draw->GetIndex(m_time_to_go);

	const Draw::VT& values = draw->GetValuesTable();
	int max_index = values.size();

	wxLogInfo(_T("Waiting for data at index: %d, time: %s, data at index is in state %d, value: %f"),
			current_index,
			m_time_to_go.Format().c_str(),
			values.at(current_index).state,
			values.at(current_index).val
			);

	int i = -1, j = max_index;

	switch (m_state) {
		case WAIT_DATA_LEFT:
			for (i = current_index; i >= 0; --i) 
				if (values.at(i).MayHaveData())
					break;
			if (i == -1) 
				EnterSearchState(SEARCH_LEFT, m_time_to_go, DTime());
			
			break;
		case WAIT_DATA_RIGHT:
			for (j = current_index; j < max_index; ++j) 
				if (values.at(j).MayHaveData())
					break;

			if (j == max_index)
				EnterSearchState(SEARCH_RIGHT, m_time_to_go, DTime());
			break;
		case WAIT_DATA_NEAREST:
			for (i = current_index, j = current_index;
					i >= 0 || j < max_index;
					--i, ++j) {

				if (i >= 0 && values.at(i).MayHaveData()) {
					if (values.at(i).IsData())
						wxLogInfo(_T("Stopping scan because value at index: %d has data"), i);
					else
						wxLogInfo(_T("Stopping scan because value at index: %d might have data"), i);
					break;
				}

				if (j < max_index && values.at(j).MayHaveData()) {
					if (values.at(j).IsData())
						wxLogInfo(_T("Stopping scan because value at index: %d has data"), j);
					else
						wxLogInfo(_T("Stopping scan because value at index: %d might have data"), j);
					break;
				}
			}

			if (i < 0 && j >= max_index)
				EnterSearchState(SEARCH_BOTH, m_time_to_go, DTime());

			break;
		default:
			assert(false);

	}

	if (i >= 0 && values.at(i).IsData()) {
		EnterDisplayState(draw->GetDTimeOfIndex(i));
	} else if (j < max_index && values.at(j).IsData()) {
		EnterDisplayState(draw->GetDTimeOfIndex(j));
	}

	BusyCursorSet();
}

void DrawsController::EnterWaitState(STATE state) {
	if (m_state == STOP) {
		EnterSearchState(SEARCH_BOTH, m_time_to_go, DTime());
		return;
	}

	m_state = state;

	const TimeIndex& index = m_draws[m_selected_draw]->GetTimeIndex();

	switch (state) {
		case WAIT_DATA_NEAREST:
		case WAIT_DATA_LEFT:
		case WAIT_DATA_RIGHT:
			m_state = state;

			wxLogInfo(_T("checking if data: %s is between %s and %s"), m_time_to_go.Format().c_str(),
					index.GetStartTime().Format().c_str(),
					index.GetFirstNotDisplayedTime().Format().c_str());
			if (m_time_to_go.IsBetween(index.GetStartTime(), index.GetFirstNotDisplayedTime()))
				CheckAwaitedDataPresence();
			else {
				STATE ns;
				switch (state) {
					case WAIT_DATA_NEAREST:
						ns = SEARCH_BOTH;
						break;
					case WAIT_DATA_LEFT:
						ns = SEARCH_LEFT;
						break;
					case WAIT_DATA_RIGHT:
						ns = SEARCH_RIGHT;
						break;
					default:
						assert(false);
				}
				EnterSearchState(ns, m_time_to_go, DTime());
			}
			break;
		default:
			assert(false);
	}

	BusyCursorSet();
}

void DrawsController::EnterSearchState(STATE state, DTime search_from, const DTime& suggested_start_time) {
	wxLogVerbose(_("DrawsController::EnterSearchState"));
	m_got_left_search_response = false;
	m_got_right_search_response = false;
	m_suggested_start_time = suggested_start_time;

	assert(m_selected_draw >= 0);
	const TimeIndex& index = m_draws[m_selected_draw]->GetTimeIndex();

	switch (state) {
		case SEARCH_LEFT:
			SendSearchQuery((search_from + index.GetTimeRes() + index.GetDateRes()).GetTime() - wxTimeSpan::Seconds(10),
					wxInvalidDateTime, -1); 
			break;
		case SEARCH_RIGHT:
			SendSearchQuery(search_from.GetTime(), wxInvalidDateTime, 1); 
			break;
		case SEARCH_BOTH:
			SendSearchQuery(search_from.GetTime(), 
					wxInvalidDateTime,
					-1);
			SendSearchQuery(search_from.GetTime(), 
					wxInvalidDateTime, 
					1);
			break;
		case SEARCH_BOTH_PREFER_RIGHT:
			SendSearchQuery(search_from.GetTime(), 
					index.GetFirstNotDisplayedTime().GetTime(),
					-1);
			SendSearchQuery(search_from.GetTime(), 
					wxInvalidDateTime, 
					1);
			break;
		case SEARCH_BOTH_PREFER_LEFT:
			SendSearchQuery((search_from.GetTime() + index.GetTimeRes() + index.GetDateRes()) - wxTimeSpan::Seconds(10), 
					wxInvalidDateTime,
					-1);
			SendSearchQuery(search_from.GetTime(), 
					index.GetStartTime().GetTime() - wxTimeSpan::Seconds(10),
					1);
			break;
		default:
			assert(false);
			break;
	}

	m_state = state;

	BusyCursorSet();
}

void DrawsController::SendSearchQuery(const wxDateTime& start, const wxDateTime& end, int direction) {
	assert(m_selected_draw >= 0);

	time_t t1 = start.IsValid() ? start.GetTicks() : -1;
	time_t t2 = end.IsValid() ? end.GetTicks() : -1;

	wxLogInfo(_T("Sending search query, start time:%s, end_time: %s, direction: %d"),
			start.IsValid() ? start.Format().c_str() : L"none",
			end.IsValid() ? end.Format().c_str() : L"none",
			direction);

	DatabaseQuery *q = new DatabaseQuery();
	q->type = DatabaseQuery::SEARCH_DATA;
	q->draw_info = m_draws[m_selected_draw]->GetDrawInfo();
	q->draw_no = m_selected_draw;
	q->search_data.start = t1;
	q->search_data.end = t2;
	q->search_data.period_type = m_draws[m_selected_draw]->GetPeriod();
	q->search_data.direction = direction;
	q->search_data.search_condition = &DrawsController::search_condition;

	QueryDatabase(q);

}

void DrawsController::DatabaseResponse(DatabaseQuery *query) {
	switch (query->type) {
		case DatabaseQuery::SEARCH_DATA:
			HandleSearchResponse(query);
			break;
		case DatabaseQuery::GET_DATA:
			HandleDataResponse(query);
			break;
		default:
			assert(false);
			break;
	}
}

void DrawsController::HandleDataResponse(DatabaseQuery *query) {

	int draw_no = query->draw_no;

	bool data_within_view = false;

	bool no_data = m_draws.at(draw_no)->GetNoData();
	m_draws[draw_no]->AddData(query, data_within_view);
	if (no_data)
		m_observers.NotifyNoData(m_draws[draw_no]);

	if (draw_no == m_selected_draw && data_within_view)
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
				CheckAwaitedDataPresence();
				break;
		}

	delete query;

}

bool DrawsController::GetNoData() {
	return m_state == STOP;
}

void DrawsController::HandleSearchResponse(DatabaseQuery *query) {
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

	assert(m_selected_draw >= 0);

	if (query->draw_info != m_draws.at(m_selected_draw)->GetDrawInfo()) {
		delete query;
		return;
	}

	DatabaseQuery::SearchData& sd = query->search_data;

	wxDateTime search_start_time(sd.start);
	assert(search_start_time.IsValid());

	wxDateTime res;
	if (sd.response != -1) {
		res = wxDateTime(sd.response);
		wxLogInfo(_T("Response to search: %s"), res.Format().c_str());
	} else {
		wxLogInfo(_T("Invalid response to search"));
		res = wxInvalidDateTime;
	}

	switch (sd.direction) {
		case -1:
			m_got_left_search_response = true;
			m_left_search_result = DTime(m_period_type, res);
			m_left_search_result.AdjustToPeriod();
			break;
		case 1:
			m_got_right_search_response = true;
			m_right_search_result = DTime(m_period_type, res);
			m_right_search_result.AdjustToPeriod();
			break;
		default:
			assert(false);
	}

	delete query;

	DTime found_time;
	const TimeIndex& index = m_draws.at(m_selected_draw)->GetTimeIndex();
	wxTimeSpan d1, d2;
	switch (m_state) {
		case SEARCH_LEFT: 
			if (!m_got_left_search_response)
				return;
			if (m_left_search_result.IsValid()) {
				wxLogInfo(_T("Left search result:%s"), m_left_search_result.Format().c_str());
				found_time = m_left_search_result;
			} else {
				NoDataFound();
				return;
			}
			break;
		case SEARCH_RIGHT:
			if (!m_got_right_search_response) 
				return;

			if (m_right_search_result.IsValid()) {
				wxLogInfo(_T("Right search result:%s"), m_right_search_result.Format().c_str());
				found_time = m_right_search_result;
			} else {
				NoDataFound();
				return;
			}
			break;
		case SEARCH_BOTH_PREFER_LEFT:
		case SEARCH_BOTH_PREFER_RIGHT:
		case SEARCH_BOTH:
			if (!m_got_right_search_response || !m_got_left_search_response) 
				return;	
			if (!m_left_search_result.IsValid() && !m_right_search_result.IsValid())
				return NoDataFound();
			if (!m_left_search_result.IsValid())
				found_time = m_right_search_result;
			else if (!m_right_search_result.IsValid())
				found_time = m_left_search_result;
			else switch (m_state) {
				case SEARCH_BOTH_PREFER_LEFT:
					if (m_left_search_result <
							index.GetStartTime() - index.GetDatePeriod() - index.GetTimePeriod())
						found_time = m_right_search_result;
					else
						found_time = m_left_search_result;
					wxLogInfo(_T("Search both prefer left found data at time: %s"), found_time.Format().c_str());
					break;
				case SEARCH_BOTH_PREFER_RIGHT:
					if (m_right_search_result >=
							index.GetStartTime() + 2 * index.GetDatePeriod() + 2 * index.GetTimePeriod())
						found_time = m_right_search_result;
					else
						found_time = m_left_search_result;
					wxLogInfo(_T("Search both prefer right found data at time: %s"), found_time.Format().c_str());
					break;
				case SEARCH_BOTH:
					d1 = search_start_time - m_left_search_result.GetTime();
					d2 = m_right_search_result.GetTime() - search_start_time;
					found_time = (d1 < d2) ? m_left_search_result : m_right_search_result;
					wxLogInfo(_T("Search both found data at time: %s"), found_time.Format().c_str());
					break;
				default:
					assert(false);
				}
			break;
		default:
			assert(false);
			break;
	}

	if (!found_time.IsValid())
		return;

	wxLogInfo(_T("Found time: %s"), found_time.Format().c_str());

	MoveToTime(ChooseStartDate(found_time));

	FetchData();

	m_time_to_go = found_time;

	EnterWaitState(WAIT_DATA_NEAREST);
}

DTime DrawsController::ChooseStartDate(const DTime& _found_time) {
	const TimeIndex& index = m_draws[m_selected_draw]->GetTimeIndex();
	DTime found_time = _found_time;
	found_time.AdjustToPeriodStart();

	if (found_time.IsBetween(m_suggested_start_time,
				m_suggested_start_time + index.GetTimePeriod() + index.GetDatePeriod()))
		return m_suggested_start_time;

	const DTime& start_time = index.GetStartTime();
	if (found_time.IsBetween(start_time, index.GetFirstNotDisplayedTime()))
		return start_time;

	DTime ret;

	if (found_time < start_time)
		if (found_time + index.GetDatePeriod() + index.GetTimePeriod() < start_time)
			ret = index.AdjustToPeriodSpan(found_time);
		else
			ret = found_time;
	else
		if (found_time < start_time 
				+ 2 * index.GetDatePeriod() 
				+ 2 * index.GetTimePeriod()) 
			ret = found_time 
					- index.GetTimePeriod()
					- index.GetDatePeriod()
					+ TimeIndex::PeriodMult[m_period_type] * index.GetTimeRes()
					+ TimeIndex::PeriodMult[m_period_type] * index.GetDateRes();
		else
			ret = index.AdjustToPeriodSpan(found_time);

	return ret;

}

bool DrawsController::SetDoubleCursor(bool double_cursor) {
	if (m_double_cursor == double_cursor)
		return double_cursor;

	if (double_cursor) {
		if (m_current_index < 0)
			return false;

		for (size_t i = 0; i < m_draws.size(); i++)
			m_draws[i]->StartDoubleCursor(m_current_index);
	} else {
		for (size_t i = 0; i < m_draws.size(); i++)
			m_draws[i]->StopDoubleCursor();
	}

	m_double_cursor = double_cursor;

	m_observers.NotifyDoubleCursorChanged(this);

	return double_cursor;
}

void DrawsController::SetFollowLatestData(bool follow_latest_data) {
	m_follow_latest_data_mode = follow_latest_data;
	if (m_state == DISPLAY && m_follow_latest_data_mode)
		RefreshData(true);
}

bool DrawsController::GetFollowLatestData() {
	return m_follow_latest_data_mode;
}

void DrawsController::FetchData() {
	for (size_t i = 0; i < m_draws.size(); i++) {
		DatabaseQuery *q = m_draws[i]->GetDataToFetch();
		if (q)
			QueryDatabase(q);
	}
}

void DrawsController::MoveToTime(const DTime &time) {
	std::vector<size_t> moved;
	wxLogInfo(_T("Moving to time: %s"), time.GetTime().Format().c_str());	
	
	Draw *d = GetSelectedDraw();
	if (d->GetBlocked()) {
		d->MoveToTime(time);
		moved.push_back(d->GetDrawNo());
	} else 
		for (size_t i = 0; i < m_draws.size(); i++)
			if (m_draws[i]->GetBlocked() == false) {
				m_draws[i]->MoveToTime(time);
				if (m_draws[i]->GetDrawInfo())
					moved.push_back(i);
	
			}

	for (std::vector<size_t>::iterator i = moved.begin();
			i != moved.end();
			i++)
		m_observers.NotifyDrawMoved(m_draws[*i], time.GetTime());
}

void DrawsController::NoDataFound() {
	switch (m_state) {
		case SEARCH_LEFT:
		case SEARCH_RIGHT:
			EnterDisplayState(m_current_time);
			break;
		case SEARCH_BOTH:
		case SEARCH_BOTH_PREFER_RIGHT:
		case SEARCH_BOTH_PREFER_LEFT:
			if (m_current_time.IsValid()) {
				EnterDisplayState(m_current_time);
			} else {
				m_draws[m_selected_draw]->SetNoData(true);
				m_observers.NotifyNoData(m_draws[m_selected_draw]);

				bool reenabled = false;
				int selected_draw = -1;

				while (true) {
					for (int i = 1; i < m_active_draws_count; i++) {
						int j = (m_selected_draw + i) % m_active_draws_count;
						if (m_draws[j]->GetEnable() && !m_draws[j]->GetNoData()) {
							selected_draw = j;
							break;
						}
					}

					if (selected_draw != -1 || reenabled)
						break;

					for (int i = 0; i < m_active_draws_count; i++) {
						m_draws[i]->SetEnable(true);
					}

					reenabled = true;
				}

				if (selected_draw != -1) {
					SetSelectedDraw(selected_draw);
					EnterSearchState(SEARCH_BOTH, m_time_to_go, DTime());
				} else {
					ThereIsNoData();
				}
			}
			break;
		default:
			assert(false);
	}
}

void DrawsController::DoSet(DrawSet *set) {
	m_double_cursor = false;

	m_draw_set = set;
	m_current_set_name = set->GetName();
	m_current_prefix = set->GetDrawsSets()->GetPrefix();

	DrawInfoArray *draws = set->GetDraws();

	if (m_draws.size() < draws->size()) {
		Draw* selected_draw = GetSelectedDraw();
		DTime start_time;
		if (selected_draw) {
			start_time = selected_draw->GetStartTime();
		}
		for (size_t i = m_draws.size(); i < draws->size(); i++) {
			Draw* draw = new Draw(this, &m_observers, i);
			if (selected_draw)
				draw->SetStartTimeAndNumberOfValues(start_time, GetNumberOfValues(m_period_type));
			m_draws.push_back(draw);
		}
	}

	for (size_t i = 0; i < draws->size(); i++)
		m_draws.at(i)->SetDraw((*draws)[i]);	

	for (size_t i = draws->size(); i < m_draws.size(); i++)
		delete m_draws.at(i);

	m_active_draws_count = draws->size();
	m_draws.resize(m_active_draws_count);

	for (size_t i = 0; i < m_draws.size(); i++) {
		m_draws[i]->SetDrawNo(i);
		m_draws[i]->SetInitialDrawNo(i);
	}

}

void DrawsController::ConfigurationWasReloaded(wxString prefix) {
	if (prefix != m_current_prefix)
		return;

	DrawsSets *dss = m_config_manager->GetConfigByPrefix(prefix);
	assert(dss);

	DrawSetsHash& dsh = dss->GetDrawsSets();
	DrawSetsHash::iterator i = dsh.find(m_current_set_name);
	if (i != dsh.end())
		DoSet(i->second);
	else {
		SortedSetsArray sets = dss->GetSortedDrawSetsNames();
		DoSet(sets[0]);
	}

	if (m_selected_draw >= m_active_draws_count)
		m_selected_draw = 0;

	DisableDisabledDraws();

	for (int i = 0; i < m_active_draws_count; i++)
		m_draws[i]->RefreshData(true);
	FetchData();

	if (m_state == DISPLAY)
		m_time_to_go = m_current_time;
	m_current_time = DTime();

	for (int i = 0; i < m_active_draws_count; i++) {
		m_observers.NotifyDrawInfoReloaded(m_draws[i]);
	}

	EnterWaitState(WAIT_DATA_NEAREST);
}

void DrawsController::SetRemoved(wxString prefix, wxString name) {
	if (m_current_prefix == prefix && m_current_set_name == name) {
		SortedSetsArray array = m_config_manager->GetConfigByPrefix(m_current_prefix)->GetSortedDrawSetsNames();
		Set(array[0]);
	}
}

void DrawsController::SetModified(wxString prefix, wxString name, DrawSet *set) {
	if (m_current_prefix == prefix && m_current_set_name == name)
		Set(set);
}

void DrawsController::SetRenamed(wxString prefix, wxString from, wxString to, DrawSet *set) {
	if (m_current_prefix == prefix && m_current_set_name == from)
		Set(set);
}

void DrawsController::Set(DrawSet *set) {

	DoSet(set);

	DisableDisabledDraws();

	if (m_selected_draw >= m_active_draws_count)
		m_selected_draw = 0;

	for (int i = 0; i < m_active_draws_count; i++) {
		int j = (i + m_selected_draw) % m_active_draws_count;
		if (m_draws[j]->GetEnable()) {
			m_selected_draw = j;
			break;
		}
	}
	if (m_draws[m_selected_draw]->GetEnable() == false) {
		m_draws[m_selected_draw]->SetEnable(true);
	}
	wxLogInfo(_T("Set '%s' choosen, selected draw index; %d"), set->GetName().c_str(), m_selected_draw);
	for (size_t i = 0; i < m_draws.size(); i++)
		m_observers.NotifyDrawInfoChanged(m_draws[i]);

	FetchData();
	
	if (m_state == DISPLAY)
		m_time_to_go = m_current_time;
	m_current_time = DTime();
	m_current_index = -1;

	EnterWaitState(WAIT_DATA_NEAREST);

}

void DrawsController::DoSwitchNextDraw(int dir) {
	if (GetNoData())
		return;

	int nsel = m_selected_draw;
	do {
		nsel += dir;
		if (nsel >= m_active_draws_count)
			nsel = 0;
		if (nsel < 0)
			nsel = m_active_draws_count - 1;
	} while (m_draws[nsel]->GetNoData() || m_draws[nsel]->GetEnable() == false);

	if (nsel == m_selected_draw)
		return;

	int psel = m_selected_draw;

	m_current_time = DTime();

	m_time_to_go = m_draws[nsel]->GetTimeOfIndex(m_current_index);

	m_current_index = -1;

	m_selected_draw = nsel;

	m_observers.NotifyDrawDeselected(m_draws.at(psel));

	m_observers.NotifyDrawSelected(m_draws.at(m_selected_draw));

	EnterWaitState(WAIT_DATA_NEAREST);
}

void DrawsController::SelectPreviousDraw() {
	DoSwitchNextDraw(-1);
}

void DrawsController::SelectNextDraw() {
	DoSwitchNextDraw(1);
}

void DrawsController::Set(PeriodType period_type) {
	m_period_type = period_type;

	m_double_cursor = false;

	if (m_state == DISPLAY)
		m_time_to_go = m_time_reference.Adjust(period_type, m_current_time);
	else
		m_time_to_go = m_time_reference.Adjust(period_type, m_time_to_go);

	m_current_time = DTime();
	m_current_index = -1;

	for (size_t i = 0; i < m_draws.size(); i++)
		m_draws.at(i)->SetPeriod(m_time_to_go, GetNumberOfValues(period_type));

	for (int i = 0; i < m_active_draws_count; i++)
		m_observers.NotifyPeriodChanged(m_draws[i], period_type);

	EnterSearchState(SEARCH_BOTH, m_time_to_go, DTime());

}

void DrawsController::Set(PeriodType pt, const wxDateTime& time, int draw_to_select) {
	if (m_period_type == pt)
		return Set(draw_to_select, time);

	m_time_to_go = DTime(pt, time).AdjustToPeriod();
	m_current_time = DTime();
	m_current_index = -1;

	int psel = m_selected_draw;
	m_selected_draw = draw_to_select;
	if (psel >= 0)
		m_observers.NotifyDrawDeselected(m_draws.at(psel));

	m_period_type = pt;
	for (size_t i = 0; i < m_draws.size(); i++)
		m_draws.at(i)->SetPeriod(m_time_to_go, GetNumberOfValues(pt));

	for (size_t i = 0; i < m_draws.size(); i++)
		m_observers.NotifyPeriodChanged(m_draws[i], pt);

	m_observers.NotifyDrawSelected(m_draws.at(m_selected_draw));

	EnterSearchState(SEARCH_BOTH, m_time_to_go, DTime());

}

void DrawsController::Set(DrawSet *set, PeriodType pt, const wxDateTime& time, int draw_to_select) {
	if (m_draw_set == set)
		return Set(pt, time, draw_to_select);

	m_double_cursor = false;

	DoSet(set);

	m_time_to_go = DTime(pt, time).AdjustToPeriod();
	m_current_time = DTime();
	m_current_index = -1;

	if (draw_to_select >= m_active_draws_count)
		draw_to_select = 0;

	m_selected_draw = draw_to_select;

	DisableDisabledDraws();

	m_period_type = pt;
	for (size_t i = 0; i < m_draws.size(); i++)
		m_draws.at(i)->SetPeriod(m_time_to_go, GetNumberOfValues(pt));

	for (size_t i = 0; i < m_draws.size(); i++)
		m_observers.NotifyDrawInfoChanged(m_draws[i]);

	EnterSearchState(SEARCH_BOTH, m_time_to_go, DTime());
}

void DrawsController::Set(int draw_to_select, const wxDateTime& time) {
	if (m_selected_draw == draw_to_select)
		return Set(time);

	m_time_to_go = DTime(m_draws[draw_to_select]->GetPeriod(), time);
	m_time_to_go.AdjustToPeriod();

	m_current_time = DTime();

	wxLogInfo(_T("selecting draw no;%d at date: %s"), draw_to_select, time.Format().c_str());

	int previously_selected = m_selected_draw;
	m_selected_draw = draw_to_select;
	if (previously_selected >= 0)
		m_observers.NotifyDrawDeselected(m_draws[previously_selected]);

	m_observers.NotifyDrawSelected(m_draws[m_selected_draw]);

	EnterWaitState(WAIT_DATA_NEAREST);
}

void DrawsController::Set(const wxDateTime& time) {
	m_time_to_go = DTime(m_draws[m_selected_draw]->GetPeriod(), time);
	m_time_to_go.AdjustToPeriod();

	EnterWaitState(WAIT_DATA_NEAREST);
}

void DrawsController::Set(int draw_no) {
	if (m_selected_draw == draw_no)
		return;

	SetSelectedDraw(draw_no);
	EnterWaitState(WAIT_DATA_NEAREST);
}

void DrawsController::Set(DrawSet *set, int draw_to_select) {
	m_double_cursor = false;

	DoSet(set);

	m_selected_draw = draw_to_select;

	DisableDisabledDraws();

	for (size_t i = 0; i < m_draws.size(); i++)
		m_observers.NotifyDrawInfoChanged(m_draws[i]);

	EnterWaitState(WAIT_DATA_NEAREST);

}

void DrawsController::DoSetSelectedDraw(int draw_no) {
	int index;
	if (m_selected_draw >= 0) {
		DTime odt;
		index = m_draws[m_selected_draw]->GetIndex(m_current_time);
		if (index >= 0)
			m_time_to_go = m_draws[draw_no]->GetTimeOfIndex(index);
	}
	m_selected_draw = draw_no;
}

void DrawsController::SetSelectedDraw(int draw_no) {
	if (draw_no == m_selected_draw)
		return;

	int previously_selected = m_selected_draw;
	DoSetSelectedDraw(draw_no);
	if (previously_selected >= 0)
		m_observers.NotifyDrawDeselected(m_draws.at(previously_selected));

	m_observers.NotifyDrawSelected(m_draws.at(m_selected_draw));

}

DrawsController::TimeReference::TimeReference(const wxDateTime &datetime) {
	wxDateTime now = wxDateTime::Now();
	m_month = now.GetMonth();
	m_day = now.GetDay();
	m_wday = now.GetWeekDay();
	m_hour = now.GetHour();
	m_minute = now.GetMinute();
	m_second = now.GetSecond();
}

void DrawsController::TimeReference::Update(const DTime& time) {
	const wxDateTime& wxt = time.GetTime();
	switch (time.GetPeriod()) {
		case PERIOD_T_30MINUTE:
			m_second = wxt.GetSecond();
		case PERIOD_T_DAY:
			m_minute = wxt.GetMinute();
			m_hour = wxt.GetHour();
		case PERIOD_T_WEEK:
			m_wday = wxt.GetWeekDay();
			m_hour = wxt.GetHour() / 8 * 8 + m_hour % 8;
		case PERIOD_T_MONTH:
		case PERIOD_T_SEASON:
			m_day = wxt.GetDay();
		case PERIOD_T_YEAR:
		case PERIOD_T_DECADE:
			break;
		default:
			assert(false);
	}
}

DTime DrawsController::TimeReference::Adjust(PeriodType pt, const DTime& time) {
	wxDateTime t = time.GetTime();

	assert(t.IsValid());

	switch (pt) {
		default:
			break;
		case PERIOD_T_30MINUTE:
			t.SetSecond(m_second);
		case PERIOD_T_DAY:
			t.SetMinute(m_minute);
			t.SetHour(m_hour);
			t.SetDay(std::min(m_day, (int)wxDateTime::GetNumberOfDays(t.GetMonth(), t.GetYear())));
			break;
		case PERIOD_T_WEEK:
			t.SetHour(m_hour / 8 * 8);
			t.SetToWeekDayInSameWeek(m_wday);
			break;
		case PERIOD_T_MONTH:
			t.SetDay(std::min(m_day, (int)wxDateTime::GetNumberOfDays(t.GetMonth(), t.GetYear())));
			break;
		case PERIOD_T_SEASON:
			//actually do nothing
			break;
	}

	return DTime(pt, t);

}

int DrawsController::GetCurrentIndex() const {
	return m_current_index;
}

bool DrawsController::AtTheNewestValue() {
	return false;
}

void DrawsController::ThereIsNoData() {
	EnterStopState();
	m_observers.NotifyNoData(this);
}

void DrawsController::EnterStopState() {
	wxLogInfo(_T("Entering stop state!"));
	m_state = STOP;
}

DrawInfo* DrawsController::GetCurrentDrawInfo() {
	if (m_selected_draw >= 0)
		return m_draws[m_selected_draw]->GetDrawInfo();
	else
		return NULL;
}

time_t DrawsController::GetCurrentTime() {
	return m_current_time.GetTime().GetTicks();
}

bool DrawsController::GetDoubleCursor() {
	return m_double_cursor;
}

int DrawsController::GetFilter() {
	return m_filter;
}

DrawSet* DrawsController::GetSet() {
	return m_draw_set;
}

bool DrawsController::GetDrawEnabled(int i) {
	return m_draws.at(i)->GetEnable();
}

size_t DrawsController::GetDrawsCount() {
	return m_active_draws_count;
}

Draw* DrawsController::GetDraw(size_t i) {
	return m_draws.at(i);
}

Draw* DrawsController::GetSelectedDraw() const {
	Draw* ret;
	if (m_selected_draw < 0)
		ret = NULL;
	else
		ret = m_draws.at(m_selected_draw);

	return ret;
}

const PeriodType& DrawsController::GetPeriod() {
	return m_period_type;
}

void DrawsController::EnterDisplayState(const DTime& time) {
	Draw *d = GetSelectedDraw();

	int dist = 0;
	int pi = m_current_index;

	m_current_time = m_time_to_go = time;
	m_current_index = d->GetIndex(m_current_time);

	if (m_double_cursor) {
		int ssi = d->GetStatsStartIndex();
		wxLogInfo(_T("Statistics start index: %d"), ssi);
		dist = m_current_index - ssi;
		for (std::vector<Draw*>::iterator i = m_draws.begin();
				i != m_draws.end();
				i++)
			(*i)->DragStats(dist);
	}
	wxLogInfo(_T("Entering display state current index set to: %d"),  m_current_index);

	m_time_reference.Update(time);

	m_observers.NotifyCurrentProbeChanged(d, pi, m_current_index, dist);

	if (wxIsBusy())
		wxEndBusyCursor();

	m_state = DISPLAY;

}

void DrawsController::SetFilter(int filter) {
	int p = m_filter;

	m_filter = filter;

	for (size_t i = 0; i < m_draws.size(); i++)
		m_draws.at(i)->SwitchFilter(p, filter);

	FetchData();

	m_observers.NotifyFilterChanged(this);

}

void DrawsController::MoveCursorLeft(int n) {
	if (m_state != DISPLAY)
		return;

	m_suggested_start_time = DTime();
	m_time_to_go = m_draws[m_selected_draw]->GetTimeOfIndex(m_current_index - n);
	EnterWaitState(WAIT_DATA_LEFT);
}

void DrawsController::MoveCursorRight(int n) {
	if (m_state != DISPLAY)
		return;

	m_suggested_start_time = DTime();
	m_time_to_go = m_draws[m_selected_draw]->GetTimeOfIndex(m_current_index + n);
	EnterWaitState(WAIT_DATA_RIGHT);
}

void DrawsController::MoveScreenLeft() {
	if (m_state != DISPLAY)
		return;

	const TimeIndex& index = m_draws[m_selected_draw]->GetTimeIndex();
	DTime suggested_start_time = index.GetStartTime() - index.GetDatePeriod() - index.GetTimePeriod();
	m_time_to_go = m_draws[m_selected_draw]->GetTimeOfIndex(m_current_index - GetNumberOfValues(GetPeriod()));
	EnterSearchState(SEARCH_BOTH_PREFER_LEFT, m_time_to_go, suggested_start_time);
}

void DrawsController::MoveScreenRight() {
	if (m_state != DISPLAY)
		return;

	const TimeIndex& index = m_draws[m_selected_draw]->GetTimeIndex();
	DTime suggested_start_time = index.GetStartTime() + index.GetDatePeriod() + index.GetTimePeriod();
	m_time_to_go = m_draws[m_selected_draw]->GetTimeOfIndex(m_current_index + GetNumberOfValues(GetPeriod()));
	EnterSearchState(SEARCH_BOTH_PREFER_RIGHT, m_time_to_go, suggested_start_time);
}

void DrawsController::MoveCursorBegin() {
	if (m_state != DISPLAY)
		return;

	m_time_to_go = m_draws[m_selected_draw]->GetStartTime();
	EnterWaitState(WAIT_DATA_NEAREST);
}


size_t DrawsController::GetNumberOfValues(const PeriodType& period_type) {
	return m_units_count[period_type] * TimeIndex::PeriodMult[period_type];
}

bool DrawsController::SetDrawEnabled(int draw, bool enable) {
	if (m_draws.at(draw)->GetEnable() == enable)
		return enable;

	if (draw == m_selected_draw && enable == false)
		return false;

	DrawInfo *di = m_draws.at(draw)->GetDrawInfo();
	if (di == NULL)
		return false;

	m_draws[draw]->SetEnable(enable);

	if (enable)
		m_disabled_draws.erase(std::make_pair(di->GetSetName(), std::make_pair(di->GetName(), draw)));
	else
		m_disabled_draws[std::make_pair(di->GetSetName(), std::make_pair(di->GetName(), draw))] = true;

	m_observers.NotifyEnableChanged(m_draws[draw]);

	if (enable)
		FetchData();

	return true;

}

void DrawsController::SetBlocked(int index, bool blocked) {
	if (m_draws.at(index)->GetBlocked() == blocked)
		return;

	std::vector<Draw*> moved;

	DTime dt;

	if (blocked == false) {
		if (m_selected_draw == index) {
			dt = m_draws[index]->GetStartTime();
			for (std::vector<Draw*>::iterator i = m_draws.begin();
					i != m_draws.end();
					i++)
				if ((*i)->GetBlocked() == false) {
					moved.push_back(*i);
					(*i)->MoveToTime(dt);
				}	
		} else {
			dt = m_draws[m_selected_draw]->GetStartTime();
			moved.push_back(m_draws[index]);
			m_draws[index]->MoveToTime(dt);
		}
		FetchData();
	}


	m_draws.at(index)->SetBlocked(blocked);
	m_observers.NotifyBlockedChanged(m_draws.at(index));
	for (std::vector<Draw*>::iterator i = moved.begin();
			i != moved.end();
			i++)
		if ((*i)->GetDrawInfo())
			m_observers.NotifyDrawMoved(*i, dt);

}

void DrawsController::SetNumberOfUnits(size_t number_of_units) {
	m_units_count[m_period_type] = number_of_units;

	m_double_cursor = false;

	for (std::vector<Draw*>::iterator i = m_draws.begin(); i != m_draws.end(); i++)
		(*i)->SetNumberOfValues(m_units_count[m_period_type] * TimeIndex::PeriodMult[m_period_type]);

	switch (m_state) {
		case DISPLAY:
			FetchData();
			m_current_index = -1;
			m_time_to_go = m_current_time;
			m_current_time = DTime();
			EnterWaitState(WAIT_DATA_NEAREST);
		case STOP:
			break;
		default:
			FetchData();
	}

	m_observers.NotifyNumberOfValuesChanged(this);

}

void DrawsController::SendQueryForEachPrefix(DatabaseQuery::QueryType qt) {
	DrawInfo *di = m_draws.at(0)->GetDrawInfo();
	for (int i = 1; i < m_active_draws_count; ++i) {
		DrawInfo *_di = m_draws[i]->GetDrawInfo();
		if (di->GetBasePrefix() != _di->GetBasePrefix()) {
			DatabaseQuery* q = new DatabaseQuery;
			q->type = qt;
			q->draw_info = di;
			QueryDatabase(q);

			di = _di;
		}
	}

	DatabaseQuery* q = new DatabaseQuery;
	q->type = qt;
	q->draw_info = di;
	QueryDatabase(q);
}

void DrawsController::ClearCache() {
	SendQueryForEachPrefix(DatabaseQuery::CLEAR_CACHE);
}

void DrawsController::GoToLatestDate() {
	if (m_state != DISPLAY || m_selected_draw < 0)
		return;

	const TimeIndex& index = m_draws[m_selected_draw]->GetTimeIndex();
	wxDateTime t = wxDateTime(std::numeric_limits<time_t>::max() - 10)
		- index.GetTimeRes()
		- index.GetDateRes();
	EnterSearchState(SEARCH_LEFT, DTime(GetPeriod(), t), DTime());
}

void DrawsController::RefreshData(bool auto_refresh) {
	if (auto_refresh && m_state != DISPLAY)
		return;

	if (auto_refresh == false)
		SendQueryForEachPrefix(DatabaseQuery::RESET_BUFFER);

	for (int i = 0; i < m_active_draws_count; i++)
		m_draws[i]->RefreshData(auto_refresh);

	if (m_follow_latest_data_mode) {
		assert(m_selected_draw >= 0);
		GoToLatestDate();
	} else {
		FetchData();
		EnterWaitState(WAIT_DATA_NEAREST);
	}
}

void DrawsController::MoveCursorEnd() {
	if (m_state == STOP)
		return;

	m_time_to_go = m_draws[m_selected_draw]->GetLastTime();
	EnterWaitState(WAIT_DATA_NEAREST);
}

void DrawsController::BusyCursorSet() {
	switch (m_state) {
		case STOP:
		case DISPLAY:
			if (wxIsBusy())
				wxEndBusyCursor();
			break;
		case SEARCH_LEFT:
		case SEARCH_RIGHT:
		case SEARCH_BOTH:
		case WAIT_DATA_NEAREST:
		case WAIT_DATA_LEFT:
		case WAIT_DATA_RIGHT:
		case SEARCH_BOTH_PREFER_LEFT:
		case SEARCH_BOTH_PREFER_RIGHT:
			if (!wxIsBusy())
				wxBeginBusyCursor();
			break;
	}
}

void DrawsController::AttachObserver(DrawObserver *observer) {
	m_observers.AttachObserver(observer);
}

void DrawsController::DetachObserver(DrawObserver *observer) {
	m_observers.DetachObserver(observer);
}

namespace {
	bool both_enabled(const Draw* d1, const Draw* d2, bool& ret) {
		if (d1->GetEnable())
			if (d2->GetEnable())
				return true;
			else
				ret = true;
		else
			if (d2->GetEnable())
				ret = false;
			else
				ret = false;
		return false;
	}

	bool cmp_avg(const Draw* d1, const Draw* d2) {
		bool r;
		if (!both_enabled(d1, d2, r))
			return r;

		const Draw::VT& vt1 = d1->GetValuesTable();
		const Draw::VT& vt2 = d2->GetValuesTable();

		if (vt1.m_number_of_values == 0 || vt2.m_number_of_values == 0)
			return false;

		return vt1.m_sum / vt1.m_number_of_values > vt2.m_sum / vt2.m_number_of_values;
	}

	bool cmp_min(const Draw* d1, const Draw* d2) {
		bool r;
		if (!both_enabled(d1, d2, r))
			return r;

		return d1->GetValuesTable().m_min > d2->GetValuesTable().m_min;
	}

	bool cmp_max(const Draw* d1, const Draw* d2) {
		bool r;
		if (!both_enabled(d1, d2, r))
			return r;

		return d1->GetValuesTable().m_max > d2->GetValuesTable().m_max;
	}

	bool cmp_hsum(const Draw* d1, const Draw* d2) {
		bool r;
		if (!both_enabled(d1, d2, r))
			return r;

		return d1->GetValuesTable().m_hsum > d2->GetValuesTable().m_hsum;
	}

	bool cmp_dno(const Draw* d1, const Draw* d2) {
		return d1->GetInitialDrawNo() < d2->GetInitialDrawNo();
	}

}

void DrawsController::SortDraws(SORTING_CRITERIA criteria) {
	switch (criteria) {
		case NO_SORT:
			std::sort(m_draws.begin(), m_draws.end(), cmp_dno);
			break;
		case BY_AVERAGE:
			std::sort(m_draws.begin(), m_draws.end(), cmp_avg);
			break;
		case BY_MAX:
			std::sort(m_draws.begin(), m_draws.end(), cmp_max);
			break;
		case BY_MIN:
			std::sort(m_draws.begin(), m_draws.end(), cmp_min);
			break;
		case BY_HOURSUM:
			std::sort(m_draws.begin(), m_draws.end(), cmp_hsum);
			break;
	}

	Draw* d = GetSelectedDraw();
	for (size_t i = 0; i < m_draws.size(); i++) {
		m_draws[i]->SetDrawNo(i);
		if (d == m_draws[i])
			m_selected_draw = i;
	}

	m_observers.NotifyDrawsSorted(this);
}

void DrawsObservers::AttachObserver(DrawObserver *observer) {
	m_observers.push_back(observer);
}

void DrawsObservers::DetachObserver(DrawObserver *observer) {

	std::vector<DrawObserver*>::iterator i;

	i = std::remove(m_observers.begin(),
		m_observers.end(), 
		observer);

	m_observers.erase(i, m_observers.end());
}

void DrawsObservers::NotifyNewData(Draw *draw, int idx) {
	for (std::vector<DrawObserver*>::iterator i = m_observers.begin();
			i != m_observers.end();
			++i) 
		(*i)->NewData(draw, idx);
}

void DrawsObservers::NotifyEnableChanged(Draw *draw) {
	std::for_each(m_observers.begin(), m_observers.end(), std::bind2nd(std::mem_fun(&DrawObserver::EnableChanged), draw));
}

void DrawsObservers::NotifyNewRemarks(Draw *draw) {
	std::for_each(m_observers.begin(), m_observers.end(), std::bind2nd(std::mem_fun(&DrawObserver::NewRemarks), draw));
}

void DrawsObservers::NotifyStatsChanged(Draw *draw) {
	std::for_each(m_observers.begin(), m_observers.end(), std::bind2nd(std::mem_fun(&DrawObserver::StatsChanged), draw));
}

void DrawsObservers::NotifyDrawMoved(Draw *draw, const wxDateTime &start_date) {
	for (std::vector<DrawObserver*>::iterator i = m_observers.begin();
			i != m_observers.end();
			++i) 
		(*i)->ScreenMoved(draw, start_date);
}

void DrawsObservers::NotifyCurrentProbeChanged(Draw *draw, int pi, int ni, int d) {
	for (std::vector<DrawObserver*>::iterator i = m_observers.begin();
			i != m_observers.end();
			++i)
		(*i)->CurrentProbeChanged(draw, pi, ni, d);
}

void DrawsObservers::NotifyNoData(Draw *draw) {
	for (std::vector<DrawObserver*>::iterator i = m_observers.begin();
			i != m_observers.end();
			++i)
		(*i)->NoData(draw);
}

void DrawsObservers::NotifyNoData(DrawsController *draws_controller) {
	for (std::vector<DrawObserver*>::iterator i = m_observers.begin();
			i != m_observers.end();
			++i)
		(*i)->NoData(draws_controller);
}

void DrawsObservers::NotifyDrawDeselected(Draw* draw) {
	std::for_each(m_observers.begin(), m_observers.end(), std::bind2nd(std::mem_fun(&DrawObserver::DrawDeselected), draw));
}

void DrawsObservers::NotifyDrawInfoChanged(Draw* draw) {
	std::for_each(m_observers.begin(), m_observers.end(), std::bind2nd(std::mem_fun(&DrawObserver::DrawInfoChanged), draw));
}

void DrawsObservers::NotifyDrawInfoReloaded(Draw* draw) {
	std::for_each(m_observers.begin(), m_observers.end(), std::bind2nd(std::mem_fun(&DrawObserver::DrawInfoReloaded), draw));
}

void DrawsObservers::NotifyDrawSelected(Draw* draw) {
	std::for_each(m_observers.begin(), m_observers.end(), std::bind2nd(std::mem_fun(&DrawObserver::DrawSelected), draw));
}

void DrawsObservers::NotifyPeriodChanged(Draw* draw, PeriodType pt) {
	for (std::vector<DrawObserver*>::iterator i = m_observers.begin();
			i != m_observers.end();
			++i)
		(*i)->PeriodChanged(draw, pt);
}

void DrawsObservers::NotifyFilterChanged(DrawsController* draws_controller) {
	std::for_each(m_observers.begin(), m_observers.end(), std::bind2nd(std::mem_fun(&DrawObserver::FilterChanged), draws_controller));
}

void DrawsObservers::NotifyDoubleCursorChanged(DrawsController* draws_controller) {
	std::for_each(m_observers.begin(), m_observers.end(), std::bind2nd(std::mem_fun(&DrawObserver::DoubleCursorChanged), draws_controller));
}

void DrawsObservers::NotifyNumberOfValuesChanged(DrawsController *draws_controller) {
	std::for_each(m_observers.begin(), m_observers.end(), std::bind2nd(std::mem_fun(&DrawObserver::NumberOfValuesChanged), draws_controller));
}

void DrawsObservers::NotifyBlockedChanged(Draw* draw) {
	std::for_each(m_observers.begin(), m_observers.end(), std::bind2nd(std::mem_fun(&DrawObserver::BlockedChanged), draw));
}

void DrawsObservers::NotifyDrawsSorted(DrawsController* controller) {
	std::for_each(m_observers.begin(), m_observers.end(), std::bind2nd(std::mem_fun(&DrawObserver::DrawsSorted), controller));
}
