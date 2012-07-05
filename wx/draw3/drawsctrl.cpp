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

void DrawsController::State::SetDrawsController(DrawsController *c) {
	m_c = c;
}

void DrawsController::State::MoveCursorLeft(int n) {}

void DrawsController::State::MoveCursorRight(int n) {}

void DrawsController::State::MoveScreenLeft() {}

void DrawsController::State::MoveScreenRight() {}

void DrawsController::State::GoToLatestDate() {}

void DrawsController::State::NewDataForSelectedDraw() {}

void DrawsController::State::HandleDataResponse(DatabaseQuery*) {}

void DrawsController::State::HandleSearchResponse(DatabaseQuery*) {}

void DrawsController::State::MoveCursorBegin() {}

void DrawsController::State::MoveCursorEnd() {}

void DrawsController::State::Reset() {}

void DrawsController::State::SetNumberOfUnits() {
	m_c->FetchData();
}

void DrawsController::DisplayState::MoveCursorLeft(int n) {
	DTime t = m_c->m_draws[m_c->m_selected_draw]->GetTimeOfIndex(m_c->m_current_index - n);
	m_c->EnterState(WAIT_DATA_LEFT, t);
}

void DrawsController::DisplayState::MoveCursorRight(int n) {
	DTime t = m_c->m_draws[m_c->m_selected_draw]->GetTimeOfIndex(m_c->m_current_index + n);
	m_c->EnterState(WAIT_DATA_RIGHT, t);
}

void DrawsController::DisplayState::MoveScreenLeft() {
	DTime t = m_c->m_draws[m_c->m_selected_draw]->GetTimeOfIndex(m_c->m_current_index - m_c->GetNumberOfValues(m_c->GetPeriod()));
	m_c->EnterState(SEARCH_BOTH_PREFER_RIGHT, t);
}

void DrawsController::DisplayState::MoveScreenRight() {
	DTime t = m_c->m_draws[m_c->m_selected_draw]->GetTimeOfIndex(m_c->m_current_index + m_c->GetNumberOfValues(m_c->GetPeriod()));
	m_c->EnterState(SEARCH_BOTH_PREFER_LEFT, t);
}

void DrawsController::DisplayState::GoToLatestDate() {
	const TimeIndex& index = m_c->m_draws[m_c->m_selected_draw]->GetTimeIndex();
	wxDateTime t = wxDateTime(std::numeric_limits<time_t>::max() - 10)
		- index.GetTimeRes()
		- index.GetDateRes();
	m_c->EnterState(SEARCH_LEFT, DTime(m_c->GetPeriod(), t));
}

void DrawsController::DisplayState::MoveCursorBegin() {
	DTime t = m_c->m_draws[m_c->m_selected_draw]->GetStartTime();
	m_c->EnterState(WAIT_DATA_NEAREST, t);
}

void DrawsController::DisplayState::MoveCursorEnd() {
	m_c->EnterState(WAIT_DATA_NEAREST, m_c->m_draws[m_c->m_selected_draw]->GetLastTime());
}

void DrawsController::DisplayState::SetNumberOfUnits() {
	m_c->FetchData();
	m_c->m_current_index = -1;
	m_c->EnterState(WAIT_DATA_NEAREST, m_c->m_current_time);
}

void DrawsController::DisplayState::Reset() {
	m_c->FetchData();
	m_c->EnterState(WAIT_DATA_NEAREST, m_c->m_current_time);
}

void DrawsController::DisplayState::Enter(const DTime& time) {
	Draw *d = m_c->GetSelectedDraw();

	int dist = 0;
	int pi = m_c->m_current_index;

	m_c->m_current_time = time;
	m_c->m_current_index = d->GetIndex(m_c->m_current_time);

	if (m_c->m_double_cursor) {
		int ssi = d->GetStatsStartIndex();
		wxLogInfo(_T("Statistics start index: %d"), ssi);
		dist = m_c->m_current_index - ssi;
		for (std::vector<Draw*>::iterator i = m_c->m_draws.begin();
				i != m_c->m_draws.end();
				i++)
			(*i)->DragStats(dist);
	}
	wxLogInfo(_T("Entering display state current index set to: %d"),  m_c->m_current_index);

	m_c->m_time_reference.Update(time);

	m_c->m_observers.NotifyCurrentProbeChanged(d, pi, m_c->m_current_index, dist);

	if (wxIsBusy())
		wxEndBusyCursor();
}

void DrawsController::DisplayState::Leaving() {
	if (!wxIsBusy())
		wxBeginBusyCursor();
}

const DTime& DrawsController::DisplayState::GetStateTime() const {
	return m_c->m_current_time;
}

void DrawsController::StopState::Enter(const DTime& time) {
	m_stop_time = time;
}

const DTime& DrawsController::StopState::GetStateTime() const {
	return m_stop_time;
}

void DrawsController::StopState::SetNumberOfUnits() { }

void DrawsController::StopState::Reset() {
	m_c->EnterState(SEARCH_BOTH, m_stop_time);
}

void DrawsController::WaitState::Reset() {
	m_c->FetchData();
}

void DrawsController::WaitState::NewDataForSelectedDraw() {
	assert(m_c->m_selected_draw >= 0);

	Draw* draw = m_c->m_draws[m_c->m_selected_draw];
	int current_index = draw->GetIndex(m_time_to_go);

	const Draw::VT& values = draw->GetValuesTable();

	wxLogInfo(_T("Waiting for data at index: %d, time: %s, data at index is in state %d, value: %f"),
			current_index,
			m_time_to_go.Format().c_str(),
			values.at(current_index).state,
			values.at(current_index).val
			);

	CheckForDataPresence(draw);
}

void DrawsController::WaitState::Enter(const DTime& time) {
	m_time_to_go = time;

	assert(m_c->m_selected_draw >= 0);

	m_c->FetchData();

	CheckForDataPresence(m_c->m_draws[m_c->m_selected_draw]);
}

const DTime& DrawsController::WaitState::GetStateTime() const {
	return m_time_to_go;
}

void DrawsController::WaitDataLeft::CheckForDataPresence(Draw* draw) {
	const Draw::VT& values = draw->GetValuesTable();

	int i = draw->GetIndex(m_time_to_go);
	for (; i >= 0; --i) {
		if (values.at(i).MayHaveData()) {
			if (values.at(i).IsData())
				m_c->EnterState(DISPLAY, draw->GetDTimeOfIndex(i));
			return;
		}
	}
	m_c->EnterState(SEARCH_LEFT, m_time_to_go);
}

void DrawsController::WaitDataRight::CheckForDataPresence(Draw* draw) {
	const Draw::VT& values = draw->GetValuesTable();
	int i = draw->GetIndex(m_time_to_go);
	if (i >= 0)
		for (; i < int(values.size()); ++i) {
			if (values.at(i).MayHaveData()) {
				if (values.at(i).IsData())
					m_c->EnterState(DISPLAY, draw->GetDTimeOfIndex(i));
				return;
			}
		}

	m_c->EnterState(SEARCH_RIGHT, m_time_to_go);
}

void DrawsController::WaitDataBoth::CheckForDataPresence(Draw *draw) {
	const Draw::VT& values = draw->GetValuesTable();
	int i = draw->GetIndex(m_time_to_go);

	if (i >= 0) {
		int j = i;
		while (i >= 0 || j < int(values.size())) {
			if (i >= 0 && values.at(i).MayHaveData()) {
				if (values.at(i).IsData()) {
					wxLogInfo(_T("Stopping scan because value at index: %d has data"), i);
					m_c->EnterState(DISPLAY, draw->GetDTimeOfIndex(i));
				} else {
					wxLogInfo(_T("Stopping scan because value at index: %d might have data"), i);
				}
				return;
			}

			if (j < int(values.size()) && values.at(j).MayHaveData()) {
				if (values.at(j).IsData()) {
					wxLogInfo(_T("Stopping scan because value at index: %d has data"), j);
					m_c->EnterState(DISPLAY, draw->GetDTimeOfIndex(j));
				} else {
					wxLogInfo(_T("Stopping scan because value at index: %d may have data"), j);
				}
				return;
			}

			i -= 1;
			j += 1;
		}

	}

	m_c->EnterState(SEARCH_BOTH, m_time_to_go);
}


const DTime& DrawsController::SearchState::GetStateTime() const {
	return m_search_time;
}

void DrawsController::SearchState::SendSearchQuery(const wxDateTime& start, const wxDateTime& end, int direction) {
	assert(m_c->m_selected_draw >= 0);

	time_t t1 = start.IsValid() ? start.GetTicks() : -1;
	time_t t2 = end.IsValid() ? end.GetTicks() : -1;

	wxLogInfo(_T("Sending search query, start time:%s, end_time: %s, direction: %d"),
			start.IsValid() ? start.Format().c_str() : L"none",
			end.IsValid() ? end.Format().c_str() : L"none",
			direction);

	DatabaseQuery *q = new DatabaseQuery();
	q->type = DatabaseQuery::SEARCH_DATA;
	q->draw_info = m_c->m_draws[m_c->m_selected_draw]->GetDrawInfo();
	if (q->draw_info->IsValid())
		q->param = q->draw_info->GetParam()->GetIPKParam();
	else 
		q->param = NULL;
	q->draw_no = m_c->m_selected_draw;
	q->search_data.start = t1;
	q->search_data.end = t2;
	q->search_data.period_type = m_c->m_draws[m_c->m_selected_draw]->GetPeriod();
	q->search_data.direction = direction;
	q->search_data.search_condition = &DrawsController::search_condition;

	m_c->QueryDatabase(q);

}

void DrawsController::SearchState::Reset() {
	Enter(m_search_time);
}

void DrawsController::SearchState::HandleSearchResponse(DatabaseQuery* query) {
	DatabaseQuery::SearchData& data = query->search_data;
	wxDateTime time = data.response != -1 ? wxDateTime(data.response) : wxInvalidDateTime;

	wxLogInfo(_T("Search response dir: %d, start: %s, end: %s,  response: %s"),
			data.direction,
			wxDateTime(data.start).Format().c_str(),
			data.end != -1 ? wxDateTime(data.end).Format().c_str() : L"none",
			time.Format().c_str());

	switch (data.direction) {
		case -1:
			HandleLeftResponse(time);
			break;
		case 1:
			HandleRightResponse(time);
			break;
		default:
			assert(false);
	}
}

void DrawsController::SearchState::HandleLeftResponse(wxDateTime& time) {}

void DrawsController::SearchState::HandleRightResponse(wxDateTime& time) {}

void DrawsController::SearchLeft::Enter(const DTime& search_from) {
	m_search_time = search_from;
	const TimeIndex& index = m_c->m_draws[m_c->m_selected_draw]->GetTimeIndex();
	SendSearchQuery((search_from + index.GetTimeRes() + index.GetDateRes()).GetTime() - wxTimeSpan::Seconds(10),
		wxInvalidDateTime, -1); 
}

void DrawsController::SearchLeft::HandleLeftResponse(wxDateTime& time) {
	if (time.IsValid()) {
		DTime draw_time(m_c->GetPeriod(), time);
		m_c->MoveToTime(m_c->ChooseStartDate(draw_time));
		m_c->EnterState(WAIT_DATA_NEAREST, draw_time);
	} else {
		m_c->EnterState(DISPLAY, m_c->m_current_time);
	}
}

void DrawsController::SearchRight::Enter(const DTime& search_from) {
	m_search_time = search_from;
	SendSearchQuery(search_from.GetTime(), wxInvalidDateTime, 1); 
}

void DrawsController::SearchRight::HandleRightResponse(wxDateTime& time) {
	if (time.IsValid()) {
		DTime draw_time(m_c->GetPeriod(), time);
		m_c->MoveToTime(m_c->ChooseStartDate(draw_time));
		m_c->EnterState(WAIT_DATA_NEAREST, draw_time);
	} else {
		m_c->EnterState(DISPLAY, m_c->m_current_time);
	}
}

DTime DrawsController::SearchBoth::FindCloserTime(const DTime& reference, const DTime& left, const DTime& right) {
	if (!left.IsValid())
		return right;
	if (!right.IsValid())
		return left;

	wxTimeSpan dl = reference.GetTime() - left.GetTime();
	wxTimeSpan dr = right.GetTime() - reference.GetTime();

	return dr > dl ? left : right;
}

void DrawsController::SearchBothPreferCloser::Enter(const DTime& search_from) {
	m_search_time = search_from;
	SendSearchQuery(search_from.GetTime(), 
			wxInvalidDateTime,
			1);
}

void DrawsController::SearchBothPreferCloser::HandleRightResponse(wxDateTime& time) {
	m_right_result = DTime(m_c->GetPeriod(), time);
	if (m_right_result == m_search_time) {
		m_c->MoveToTime(m_c->ChooseStartDate(m_right_result, m_start_time));
		m_c->EnterState(WAIT_DATA_NEAREST, m_right_result);
	} else {
		SendSearchQuery(m_search_time.GetTime() - wxTimeSpan::Seconds(10), 
			wxInvalidDateTime,
			-1);
	}
}

void DrawsController::SearchBothPreferCloser::HandleLeftResponse(wxDateTime& time) {
	DTime result(m_c->GetPeriod(), time);
	DTime found = FindCloserTime(m_search_time, result, m_right_result);
	if (found.IsValid()) {
		m_c->MoveToTime(m_c->ChooseStartDate(found, m_start_time));
		m_c->EnterState(WAIT_DATA_NEAREST, found);
	} else {
		m_c->m_draws[m_c->m_selected_draw]->SetNoData(true);
		m_c->m_observers.NotifyNoData(m_c->m_draws[m_c->m_selected_draw]);

		if (!SwitchToGraphThatMayHaveData())
			m_c->ThereIsNoData(m_search_time);
	}
}

bool DrawsController::SearchBothPreferCloser::SwitchToGraphThatMayHaveData() {
	//nothing found, so there is no data for this graph
	//anything and there is no current time

	int selected_draw = -1;

	for (int i = 1; i < m_c->m_active_draws_count; i++) {
		int j = (m_c->m_selected_draw + i) % m_c->m_active_draws_count;
		if (m_c->m_draws[j]->GetEnable() && !m_c->m_draws[j]->GetNoData()) {
			selected_draw = j;
			break;
		}
	}

	if (selected_draw != -1) {
		DTime t = m_c->SetSelectedDraw(selected_draw);
		m_c->EnterState(SEARCH_BOTH, t);
		return true;
	}

	for (int i = 1; i < m_c->m_active_draws_count; i++) {
		int j = (m_c->m_selected_draw + i) % m_c->m_active_draws_count;
		if (!m_c->m_draws[j]->GetNoData()) {
			selected_draw = j;
			break;
		}
	}

	if (selected_draw != -1) {
		m_c->SetDrawEnabled(selected_draw, true);
		DTime t = m_c->SetSelectedDraw(selected_draw);
		m_c->EnterState(SEARCH_BOTH, t);
		return true;
	}

	return false;
}

void DrawsController::SearchBothPreferRight::Enter(const DTime& search_from) {
	const TimeIndex& index = m_c->m_draws[m_c->m_selected_draw]->GetTimeIndex();

	m_search_time = search_from;
	m_start_time = index.GetStartTime() - index.GetDatePeriod() - index.GetTimePeriod();

	SendSearchQuery(search_from.GetTime(), 
			index.GetStartTime().GetTime() - wxTimeSpan::Seconds(10),
			1);
}

void DrawsController::SearchBothPreferRight::HandleRightResponse(wxDateTime& time) {
	m_right_result = DTime(m_c->GetPeriod(), time);
	if (m_right_result == m_search_time) {
		m_c->MoveToTime(m_c->ChooseStartDate(m_right_result, m_start_time));
		m_c->EnterState(WAIT_DATA_RIGHT, m_right_result);
	} else {
		SendSearchQuery(m_search_time.GetTime() - wxTimeSpan::Seconds(10), 
			wxInvalidDateTime,
			-1);
	}
}

void DrawsController::SearchBothPreferRight::HandleLeftResponse(wxDateTime& time) {
	DTime left_result(m_c->GetPeriod(), time);
	if (left_result.IsValid())
		if (m_right_result.IsValid()) {
			const TimeIndex& index = m_c->m_draws[m_c->m_selected_draw]->GetTimeIndex();
			if (left_result < index.GetStartTime() - index.GetDatePeriod() - index.GetTimePeriod()) {
				m_c->MoveToTime(m_c->ChooseStartDate(m_right_result, m_start_time));
				m_c->EnterState(WAIT_DATA_NEAREST, m_right_result);
			} else {
				DTime closer = FindCloserTime(m_search_time, left_result, m_right_result);	
				m_c->MoveToTime(m_c->ChooseStartDate(closer, m_start_time));
				m_c->EnterState(WAIT_DATA_NEAREST, closer);
			}
		} else {
			m_c->MoveToTime(m_c->ChooseStartDate(left_result, m_start_time));
			m_c->EnterState(WAIT_DATA_NEAREST, left_result);
		}
	else
		if (m_right_result.IsValid()) {
			m_c->MoveToTime(m_c->ChooseStartDate(m_right_result, m_start_time));
			m_c->EnterState(WAIT_DATA_NEAREST, m_right_result);
		} else {
			m_c->EnterState(DISPLAY, m_c->m_current_time);
		}
}

void DrawsController::SearchBothPreferLeft::Enter(const DTime& search_from) {
	const TimeIndex& index = m_c->m_draws[m_c->m_selected_draw]->GetTimeIndex();

	m_search_time = search_from;
	m_start_time = index.GetStartTime() + index.GetDatePeriod() + index.GetTimePeriod();

	SendSearchQuery(search_from.GetTime(), 
			index.GetFirstNotDisplayedTime().GetTime(),
			-1);
}

void DrawsController::SearchBothPreferLeft::HandleLeftResponse(wxDateTime& time) {
	m_left_result = DTime(m_c->GetPeriod(), time);
	if (m_left_result == m_search_time) {
		m_c->MoveToTime(m_c->ChooseStartDate(m_left_result, m_start_time));
		m_c->EnterState(WAIT_DATA_RIGHT, m_left_result);
	} else {
		SendSearchQuery(m_search_time.GetTime() - wxTimeSpan::Seconds(10), 
					wxInvalidDateTime,
					1);
	}
}

void DrawsController::SearchBothPreferLeft::HandleRightResponse(wxDateTime& time) {
	DTime right_result(m_c->GetPeriod(), time);
	if (right_result.IsValid())
		if (m_left_result.IsValid()) {
			const TimeIndex& index = m_c->m_draws[m_c->m_selected_draw]->GetTimeIndex();
			if (right_result >= index.GetStartTime() + 2 * index.GetDatePeriod() + 2 * index.GetTimePeriod()) {
				m_c->MoveToTime(m_c->ChooseStartDate(m_left_result, m_start_time));
				m_c->EnterState(WAIT_DATA_NEAREST, m_left_result);
			} else {
				DTime closer = FindCloserTime(m_search_time, m_left_result, right_result);	
				m_c->MoveToTime(m_c->ChooseStartDate(closer, m_start_time));
				m_c->EnterState(WAIT_DATA_NEAREST, closer);
			}
		} else {
			m_c->MoveToTime(m_c->ChooseStartDate(right_result, m_start_time));
			m_c->EnterState(WAIT_DATA_NEAREST, right_result);
		}
	else
		if (m_left_result.IsValid()) {
			m_c->MoveToTime(m_c->ChooseStartDate(m_left_result, m_start_time));
			m_c->EnterState(WAIT_DATA_NEAREST, m_left_result);
		} else {
			m_c->EnterState(DISPLAY, m_c->m_current_time);
		}
}



DrawsController::DrawsController(ConfigManager *config_manager, DatabaseManager *database_manager) :
	DBInquirer(database_manager), 
	m_time_reference(wxDateTime::Now()),
	m_config_manager(config_manager),
	m_current_time(PERIOD_T_OTHER),
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


	m_states[STOP] = new StopState();
	m_states[DISPLAY] = new DisplayState();
	m_states[WAIT_DATA_LEFT] = new WaitDataLeft();
	m_states[WAIT_DATA_RIGHT] = new WaitDataRight();
	m_states[WAIT_DATA_NEAREST] = new WaitDataBoth();
	m_states[SEARCH_LEFT] = new SearchLeft();
	m_states[SEARCH_RIGHT] = new SearchRight();
	m_states[SEARCH_BOTH] = new SearchBothPreferCloser();
	m_states[SEARCH_BOTH_PREFER_LEFT] = new SearchBothPreferLeft();
	m_states[SEARCH_BOTH_PREFER_RIGHT] = new SearchBothPreferRight();

	for (int i = 0; i < FIRST_UNUSED_STATE_ID; i++)
		m_states[i]->SetDrawsController(this);

	m_state = m_states[STOP];
}

DrawsController::~DrawsController() {
	for (size_t i = 0; i < m_draws.size(); i++)
		delete m_draws[i];

	for (int i = 0; i < FIRST_UNUSED_STATE_ID; i++)
		delete m_states[i];

	m_config_manager->DeregisterConfigObserver(this);
}

void DrawsController::DisableDisabledDraws() {
	bool any_disabled_draw_present = false;
	for (std::vector<Draw*>::iterator i = m_draws.begin(); i != m_draws.end(); i++) {
		DrawInfo *di = (*i)->GetDrawInfo();
		if (di != NULL &&
				m_disabled_draws.find(std::make_pair(di->GetSetName(), std::make_pair(di->GetName(), (*i)->GetDrawNo()))) 
					!= m_disabled_draws.end()) {
			(*i)->SetEnable(false);
			any_disabled_draw_present = true;
		} else {
			(*i)->SetEnable(true);
		}
	}

	if (m_draws[m_selected_draw]->GetEnable() == false) for (std::vector<Draw*>::iterator i = m_draws.begin(); i != m_draws.end(); i++) 
		if ((*i)->GetEnable()) {
			m_selected_draw = std::distance(m_draws.begin(), i);
			break;
		}

	if (m_draws[m_selected_draw]->GetEnable() == false)
		m_draws[m_selected_draw]->SetEnable(true);
	
	if (!any_disabled_draw_present) for (size_t i = MAXIMUM_NUMBER_OF_INITIALLY_ENABLED_DRAWS; i < m_draws.size(); i++) {
		DrawInfo *di = m_draws[i]->GetDrawInfo();
		m_draws[i]->SetEnable(false);
		if (di == NULL)
			continue;
		m_disabled_draws[std::make_pair(di->GetSetName(), std::make_pair(di->GetName(), m_draws[i]->GetDrawNo()))] = true;
	}
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
	if (size_t(draw_no) >= m_draws.size()
			|| query->value_data.period_type != GetPeriod()
			|| query->draw_info != m_draws[draw_no]->GetDrawInfo()) {
		delete query;
		return;
	}

	bool data_within_view = false;
	bool no_data = m_draws.at(draw_no)->GetNoData();

	m_draws[draw_no]->AddData(query, data_within_view);

	if (no_data)
		m_observers.NotifyNoData(m_draws[draw_no]);

	if (draw_no == m_selected_draw && data_within_view)
		m_state->NewDataForSelectedDraw();

	delete query;
}

bool DrawsController::GetNoData() {
	return m_state == m_states[STOP];
}

std::pair<time_t, time_t> DrawsController::GetStatsInterval() {
	if (m_selected_draw == -1)
		return std::make_pair(time_t(-1), time_t(-1));

	const Draw::VT& vt = m_draws[m_selected_draw]->GetValuesTable();

	time_t end = m_draws[m_selected_draw]->GetTimeOfIndex(vt.m_stats.m_end - vt.m_view.Start()).GetTime().GetTicks();
	time_t start = m_draws[m_selected_draw]->GetTimeOfIndex(vt.m_stats.m_start - vt.m_view.Start()).GetTime().GetTicks();

	return std::make_pair(std::min(start, end), std::max(start, end));
}

void DrawsController::HandleSearchResponse(DatabaseQuery *query) {
	if (query->search_data.period_type == GetPeriod() && GetCurrentDrawInfo() == query->draw_info)
		m_state->HandleSearchResponse(query);
	delete query;
}

DTime DrawsController::ChooseStartDate(const DTime& _found_time, const DTime& suggested_start_time) {
	const TimeIndex& index = m_draws[m_selected_draw]->GetTimeIndex();
	DTime found_time = _found_time;
	found_time.AdjustToPeriodStart();

	if (found_time.IsBetween(suggested_start_time,
				suggested_start_time + index.GetTimePeriod() + index.GetDatePeriod()))
		return suggested_start_time;

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
	//ugh
	if (m_state == m_states[DISPLAY] && m_follow_latest_data_mode)
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

	m_state->Reset();
		
	for (int i = 0; i < m_active_draws_count; i++)
		m_observers.NotifyDrawInfoReloaded(m_draws[i]);

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
	if (set == NULL)
		return;

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

	m_state->Reset();
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

	Set(nsel);
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

	DTime state_time = m_state->GetStateTime();
	DTime time = m_time_reference.Adjust(period_type, state_time);

	m_current_time = DTime();
	m_current_index = -1;

	for (size_t i = 0; i < m_draws.size(); i++)
		m_draws.at(i)->SetPeriod(time, GetNumberOfValues(period_type));

	for (int i = 0; i < m_active_draws_count; i++)
		m_observers.NotifyPeriodChanged(m_draws[i], period_type);

	EnterState(SEARCH_BOTH, time);
}

void DrawsController::Set(PeriodType pt, const wxDateTime& time, int draw_to_select) {
	if (m_period_type == pt)
		return Set(draw_to_select, time);

	DTime period_time = DTime(pt, time).AdjustToPeriod();
	m_current_time = DTime();
	m_current_index = -1;

	int psel = m_selected_draw;
	m_selected_draw = draw_to_select;
	if (psel >= 0)
		m_observers.NotifyDrawDeselected(m_draws.at(psel));

	m_period_type = pt;
	for (size_t i = 0; i < m_draws.size(); i++)
		m_draws.at(i)->SetPeriod(period_time, GetNumberOfValues(pt));

	for (size_t i = 0; i < m_draws.size(); i++)
		m_observers.NotifyPeriodChanged(m_draws[i], pt);

	m_observers.NotifyDrawSelected(m_draws.at(m_selected_draw));

	EnterState(SEARCH_BOTH, period_time);

}

void DrawsController::Set(DrawSet *set, PeriodType pt, const wxDateTime& time, int draw_to_select) {
	if (m_draw_set == set)
		return Set(pt, time, draw_to_select);

	m_double_cursor = false;

	DoSet(set);

	DTime period_time = DTime(pt, time).AdjustToPeriod();
	m_current_time = DTime();
	m_current_index = -1;

	if (draw_to_select >= m_active_draws_count)
		draw_to_select = 0;

	m_selected_draw = draw_to_select;

	DisableDisabledDraws();

	m_period_type = pt;
	for (size_t i = 0; i < m_draws.size(); i++)
		m_draws.at(i)->SetPeriod(period_time, GetNumberOfValues(pt));

	for (size_t i = 0; i < m_draws.size(); i++)
		m_observers.NotifyDrawInfoChanged(m_draws[i]);

	EnterState(SEARCH_BOTH, period_time);
}

void DrawsController::Set(int draw_to_select, const wxDateTime& time) {
	if (m_selected_draw == draw_to_select)
		return Set(time);

	DTime period_time = DTime(m_draws[draw_to_select]->GetPeriod(), time).AdjustToPeriod();

	m_current_time = DTime();

	wxLogInfo(_T("selecting draw no;%d at date: %s"), draw_to_select, time.Format().c_str());

	int previously_selected = m_selected_draw;
	m_selected_draw = draw_to_select;
	if (previously_selected >= 0)
		m_observers.NotifyDrawDeselected(m_draws[previously_selected]);

	m_observers.NotifyDrawSelected(m_draws[m_selected_draw]);

	EnterState(WAIT_DATA_NEAREST, period_time);
}

void DrawsController::Set(const wxDateTime& _time) {
	DTime time(m_draws[m_selected_draw]->GetPeriod(), _time);
	time.AdjustToPeriod();
	EnterState(WAIT_DATA_NEAREST, time);
}

void DrawsController::Set(int draw_no) {
	if (m_selected_draw == draw_no)
		return;

	DTime t = SetSelectedDraw(draw_no);
	EnterState(WAIT_DATA_NEAREST, t);
}

void DrawsController::Set(DrawSet *set, int draw_to_select) {
	m_double_cursor = false;

	DoSet(set);

	m_selected_draw = draw_to_select;

	DisableDisabledDraws();

	for (size_t i = 0; i < m_draws.size(); i++)
		m_observers.NotifyDrawInfoChanged(m_draws[i]);

	EnterState(WAIT_DATA_NEAREST, m_state->GetStateTime());

}

DTime DrawsController::DoSetSelectedDraw(int draw_no) {
	int dist;
	DTime ret;
	if (m_selected_draw >= 0) {
		DTime odt;
		m_draws[m_selected_draw]->GetIndex(m_state->GetStateTime(), &dist);
		ret = m_draws[draw_no]->GetTimeOfIndex(dist);
	} else {
		ret = DTime(GetPeriod(), wxDateTime::Now());
	}
	m_selected_draw = draw_no;
	return ret;
}

DTime DrawsController::SetSelectedDraw(int draw_no) {
	if (draw_no == m_selected_draw)
		return m_state->GetStateTime();

	int previously_selected = m_selected_draw;
	DTime time = DoSetSelectedDraw(draw_no);
	if (previously_selected >= 0)
		m_observers.NotifyDrawDeselected(m_draws.at(previously_selected));

	m_observers.NotifyDrawSelected(m_draws.at(m_selected_draw));

	return time;
}

void DrawsController::EnterState(STATE state, const DTime &time) {
	m_state->Leaving();
	m_state = m_states[state];
	m_state->Enter(time);
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

void DrawsController::ThereIsNoData(const DTime& time) {
	EnterState(STOP, time);
	m_observers.NotifyNoData(this);
}

DrawInfo* DrawsController::GetCurrentDrawInfo() {
	if (m_selected_draw >= 0)
		return m_draws[m_selected_draw]->GetDrawInfo();
	else
		return NULL;
}

time_t DrawsController::GetCurrentTime() {
	return m_current_time.IsValid() ? m_current_time.GetTime().GetTicks() : -1;
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

void DrawsController::SetFilter(int filter) {
	int p = m_filter;

	m_filter = filter;

	for (size_t i = 0; i < m_draws.size(); i++)
		m_draws.at(i)->SwitchFilter(p, filter);

	FetchData();

	m_observers.NotifyFilterChanged(this);

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

	m_state->SetNumberOfUnits();

	m_observers.NotifyNumberOfValuesChanged(this);
}

void DrawsController::SendQueryForEachPrefix(DatabaseQuery::QueryType qt) {
	DrawInfo *di = m_draws.at(0)->GetDrawInfo();
	for (int i = 1; i < m_active_draws_count; ++i) {
		DrawInfo *_di = m_draws[i]->GetDrawInfo();
		if (_di->IsValid() == false)
			continue;
		if (di->GetBasePrefix() != _di->GetBasePrefix()) {
			DatabaseQuery* q = new DatabaseQuery;
			q->type = qt;
			q->draw_info = di;
			q->param = di->GetParam()->GetIPKParam();
			QueryDatabase(q);

			di = _di;
		}
	}

	if (!di->IsValid())
		return;

	DatabaseQuery* q = new DatabaseQuery;
	q->type = qt;
	q->draw_info = di;
	q->param = di->GetParam()->GetIPKParam();
	QueryDatabase(q);
}

void DrawsController::ClearCache() {
	SendQueryForEachPrefix(DatabaseQuery::CLEAR_CACHE);
}

void DrawsController::GoToLatestDate() {
	m_state->GoToLatestDate();
}

void DrawsController::RefreshData(bool auto_refresh) {
	if (auto_refresh && m_state != m_states[DISPLAY]) //ugh
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
		EnterState(WAIT_DATA_NEAREST, m_state->GetStateTime());
	}
}

void DrawsController::MoveCursorLeft(int n) {
	m_state->MoveCursorLeft(n);
}

void DrawsController::MoveCursorRight(int n) {
	m_state->MoveCursorRight(n);
}

void DrawsController::MoveScreenRight() {
	m_state->MoveScreenRight();
}

void DrawsController::MoveScreenLeft() {
	m_state->MoveScreenLeft();
}

void DrawsController::MoveCursorBegin() {
	m_state->MoveCursorBegin();
}

void DrawsController::MoveCursorEnd() {
	m_state->MoveCursorEnd();
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
	bool (*cmp_func)(const Draw*, const Draw*);
	switch (criteria) {
		case NO_SORT:
			cmp_func = cmp_dno;
			break;
		case BY_AVERAGE:
			cmp_func = cmp_avg;
			break;
		case BY_MAX:
			cmp_func = cmp_max;
			break;
		case BY_MIN:
			cmp_func = cmp_min;
			break;
		case BY_HOURSUM:
			cmp_func = cmp_hsum;
			break;
	}

	std::sort(m_draws.begin(), m_draws.end(), cmp_func);

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
