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
#include "dbmgr.h"
#include "cfgmgr.h"

#include "custom_assert.h"

static const size_t MAXIMUM_NUMBER_OF_INITIALLY_ENABLED_DRAWS = 12;

szb_nan_search_condition DrawsController::search_condition;

void DrawsController::State::SetDrawsController(StateController *c) {
	m_c = c;
}

void DrawsController::State::MoveCursorLeft(int n) {}

void DrawsController::State::MoveCursorRight(int n) {}

void DrawsController::State::MoveScreenLeft() {}

void DrawsController::State::MoveScreenRight() {}

void DrawsController::State::GoToLatestDate() {
	const TimeIndex& index = m_c->GetCurrentTimeWindow();

	///XXX:
	wxDateTime t = wxDateTime(31, wxDateTime::Dec, 2036, 23, 59, 59)
		- index.GetTimeRes()
		- index.GetDateRes();

	m_c->EnterState(SEARCH_LATEST, DTime(m_c->GetPeriod(), t));
}

void DrawsController::State::NewDataForSelectedDraw() {}

void DrawsController::State::HandleDataResponse(DatabaseQuery*) {}

void DrawsController::State::HandleSearchResponse(DatabaseQuery*) {}

void DrawsController::State::MoveCursorBegin() {}

void DrawsController::State::MoveCursorEnd() {}

void DrawsController::State::Reset() {}

void DrawsController::State::SetNumberOfUnits() {
	m_c->FetchData();
}

void DrawsController::State::ParamDataChanged(TParam* param) {
	for (auto& draw: m_c->GetDraws()) {
		if (!draw->GetEnable())
			continue;

		if (param != draw->GetDrawInfo()->GetParam()->GetIPKParam())
			continue;

		DatabaseQuery *q = draw->GetDataToFetch(true);
		if (q)
			m_c->SendQueryToDatabase(q);
	}
}

void DrawsController::State::NewValuesAdded(Draw *draw, bool non_fixed)
{ }

void DrawsController::DisplayState::MoveCursorLeft(int n) {
	DTime t = m_c->GetSelectedDraw()->GetTimeOfIndex(m_c->GetCurrentIndex() - n);
	m_c->EnterState(WAIT_DATA_LEFT, t);
}

void DrawsController::DisplayState::MoveCursorRight(int n) {
	DTime t = m_c->GetSelectedDraw()->GetTimeOfIndex(m_c->GetCurrentIndex() + n);
	m_c->EnterState(WAIT_DATA_RIGHT, t);
}

void DrawsController::DisplayState::MoveScreenLeft() {
	DTime t = m_c->GetSelectedDraw()->GetTimeOfIndex(m_c->GetCurrentIndex());
	m_c->EnterState(MOVE_SCREEN_LEFT, t);
}

void DrawsController::DisplayState::MoveScreenRight() {
	DTime t = m_c->GetSelectedDraw()->GetTimeOfIndex(m_c->GetCurrentIndex());
	m_c->EnterState(MOVE_SCREEN_RIGHT, t);
}

void DrawsController::DisplayState::MoveCursorBegin() {
	DTime t = m_c->GetSelectedDraw()->GetStartTime();
	m_c->EnterState(WAIT_DATA_NEAREST, t);
}

void DrawsController::DisplayState::MoveCursorEnd() {
	m_c->EnterState(WAIT_DATA_NEAREST, m_c->GetSelectedDraw()->GetLastTime());
}

void DrawsController::DisplayState::SetNumberOfUnits() {
	m_c->FetchData();
	m_c->SetCurrentIndex(-1);
	m_c->EnterState(WAIT_DATA_NEAREST, m_c->GetCurrentTime());
}

void DrawsController::DisplayState::Reset() {
	m_c->FetchData();
	m_c->EnterState(WAIT_DATA_NEAREST, m_c->GetCurrentTime());
}

void DrawsController::DisplayState::Enter(const DTime& time) {
	if (!time.IsValid()) {
		ASSERT(!"Cannot enter into invalid time!");
		return m_c->EnterState(STOP, m_c->GetCurrentTime());
	}

	Draw *d = m_c->GetSelectedDraw();

	int dist = 0;
	int pi = m_c->GetCurrentIndex();

	m_c->SetCurrentTime(time);
	m_c->SetCurrentIndex(d->GetIndex(m_c->GetCurrentTime()));

	if (m_c->GetDoubleCursor()) {
		int ssi = d->GetStatsStartIndex();
		wxLogInfo(_T("Statistics start index: %d"), ssi);
		dist = m_c->GetCurrentIndex() - ssi;
		for (auto& draw: m_c->GetDraws()) {
			draw->DragStats(dist);
		}
	}
	wxLogInfo(_T("Entering display state current index set to: %d"),  m_c->GetCurrentIndex());

	m_c->UpdateTimeReference(time);

	m_c->NotifyStateChanged(d, pi, m_c->GetCurrentIndex(), dist);

	std::vector<TParam*> to_sub;
	for (auto& draw: m_c->GetDraws()) {
		if (!draw->HasUnfixedData())
			continue;

		if (draw->GetSubscribed())
			continue;

		auto di = draw->GetDrawInfo();
		if (!di->IsValid())
			continue;

		to_sub.push_back(di->GetParam()->GetIPKParam());
		draw->SetSubscribed(true);
	}

	m_c->ChangeDBObservedParamsRegistration({ }, to_sub);

	if (wxIsBusy())
		wxEndBusyCursor();
}

void DrawsController::DisplayState::Leaving() {
	if (!wxIsBusy())
		wxBeginBusyCursor();
}

const DTime& DrawsController::DisplayState::GetStateTime() const {
	return m_c->GetCurrentTime();
}

void DrawsController::DisplayState::NewValuesAdded(Draw *draw, bool non_fixed)
{
	if (non_fixed) {
		if (draw->GetSubscribed())
			return;
		if (draw->GetDrawInfo()->IsValid())
			m_c->ChangeDBObservedParamsRegistration({}, { draw->GetDrawInfo()->GetParam()->GetIPKParam() });
		draw->SetSubscribed(true);
	} else {
		if (!draw->GetSubscribed() || draw->HasUnfixedData())
			return;
		if (draw->GetDrawInfo()->IsValid())
			m_c->ChangeDBObservedParamsRegistration({ draw->GetDrawInfo()->GetParam()->GetIPKParam() }, {});
		draw->SetSubscribed(false);
	}
}

void DrawsController::StopState::Enter(const DTime& time) {
	m_stop_time = time;

	if (wxIsBusy())
		wxEndBusyCursor();
}

const DTime& DrawsController::StopState::GetStateTime() const {
	return m_stop_time;
}

void DrawsController::StopState::SetNumberOfUnits() { }

void DrawsController::StopState::Reset() {
	if (!wxIsBusy())
		wxBeginBusyCursor();

	m_c->EnterState(SEARCH_BOTH, m_stop_time);
}

void DrawsController::WaitState::Reset() {
	m_c->FetchData();
}

void DrawsController::WaitState::NewDataForSelectedDraw() {
	if (m_c->GetSelectedDrawNo() < 0) {
		ASSERT(!"Invalid or no draw selected!");
		return m_c->EnterState(STOP, m_c->GetCurrentTime());
	}

	Draw* draw = m_c->GetSelectedDraw();
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
	if (m_c->GetSelectedDrawNo() < 0) {
		ASSERT(!"Invalid or no draw selected!");
		return m_c->EnterState(STOP, m_c->GetCurrentTime());
	}

	m_time_to_go = time;
	m_c->FetchData();

	Draw* draw = m_c->GetSelectedDraw();
	if (draw->GetIndex(time) == -1) {
		// not on current screen
		return onDataNotOnScreen();
	}

	CheckForDataPresence(m_c->GetSelectedDraw());
}

bool DrawsController::WaitState::checkIndex(const Draw::VT& values, int i) {
	if (i < 0 || i >= int(values.size())) return false;

	if (values.at(i).MayHaveData()) {
		Draw* draw = m_c->GetSelectedDraw();
		m_c->EnterState(DISPLAY, draw->GetDTimeOfIndex(i));
		if (values.at(i).IsData()) {
			wxLogInfo(_T("Stopping scan because value at index: %d has data"), i);
			return true;
		} else {
			m_c->NotifyStateNoData(m_c->GetSelectedDraw());
			wxLogInfo(_T("Stopping scan because value at index: %d might have data"), i);
			return true;
		}
	}

	return false;
}

void DrawsController::WaitState::onDataNotOnScreen() {
	m_c->EnterState(SEARCH_BOTH, m_time_to_go);
}


const DTime& DrawsController::WaitState::GetStateTime() const {
	return m_time_to_go;
}

void DrawsController::WaitDataLeft::onDataNotOnScreen() {
	m_c->EnterState(SEARCH_LEFT, m_time_to_go);
}

void DrawsController::WaitDataLeft::CheckForDataPresence(Draw* draw) {
	const Draw::VT& values = draw->GetValuesTable();

	int dest = draw->GetIndex(m_time_to_go);
	int current = m_c->GetCurrentIndex();

	if (dest == -1)
		return onDataNotOnScreen();

	for (auto i = dest; i < current; ++i) {
		if (checkIndex(values, i)) {
			return;
		}
	}

	for (auto i = dest; i >= 0; --i) {
		if (checkIndex(values, i)) {
			return;
		}
	}

	onDataNotOnScreen();
}

void DrawsController::WaitDataRight::onDataNotOnScreen() {
	m_c->EnterState(SEARCH_RIGHT, m_time_to_go);
}

void DrawsController::WaitDataRight::CheckForDataPresence(Draw* draw) {
	const Draw::VT& values = draw->GetValuesTable();

	int dest = draw->GetIndex(m_time_to_go);
	int current = m_c->GetCurrentIndex();

	if (dest == -1)
		return onDataNotOnScreen();

	for (auto i = dest; i > current; --i) {
		if (checkIndex(values, i)) {
			return;
		}
	}

	for (auto i = dest; i < int(values.size()); ++i) {
		if (checkIndex(values, i)) {
			return;
		}
	}

	onDataNotOnScreen();
}

void DrawsController::WaitDataBoth::CheckForDataPresence(Draw *draw) {
	const Draw::VT& values = draw->GetValuesTable();
	int i = draw->GetIndex(m_time_to_go);
	if (i == -1) 
		return onDataNotOnScreen();

	int j = i;
	while (i >= 0 || j < int(values.size())) {
		if (checkIndex(values, i--) || checkIndex(values, j++)) {
			return;
		}
	}

	onDataNotOnScreen();
}

const DTime& DrawsController::SearchState::GetStateTime() const {
	return m_search_time;
}

void DrawsController::SearchState::SendSearchQuery(const wxDateTime& start, const wxDateTime& end, int direction) {
	if (m_c->GetSelectedDrawNo() < 0) {
		ASSERT(!"Invalid or no draw selected!");
		return m_c->EnterState(STOP, m_c->GetCurrentTime());
	}

	wxLogInfo(_T("Sending search query, start time:%ls, end_time: %ls, direction: %d"),
			start.IsValid() ? start.Format().c_str() : L"none",
			end.IsValid() ? end.Format().c_str() : L"none",
			direction);

	DatabaseQuery *q = new DatabaseQuery();
	q->type = DatabaseQuery::SEARCH_DATA;
	q->prefix = m_c->GetCurrentPrefix().ToStdWstring();
	q->draw_info = m_c->GetSelectedDraw()->GetDrawInfo();
	if (q->draw_info->IsValid())
		q->param = q->draw_info->GetParam()->GetIPKParam();
	else
		q->param = NULL;
	q->draw_no = m_c->GetSelectedDrawNo();
	ToNanosecondTime(start,
		q->search_data.start_second,
		q->search_data.start_nanosecond);
	ToNanosecondTime(end,
		q->search_data.end_second,
		q->search_data.end_nanosecond);
	q->search_data.period_type = m_c->GetSelectedDraw()->GetPeriod();
	q->search_data.direction = direction;
	q->search_data.search_condition = &DrawsController::search_condition;

	q->q_id = ++m_search_query_id;

	m_c->SendQueryToDatabase(q);

}

void DrawsController::SearchState::Reset() {
	Enter(m_search_time);
}

void DrawsController::SearchState::HandleSearchResponse(DatabaseQuery* query) {
	DatabaseQuery::SearchData& data = query->search_data;

	if (query->q_id != m_search_query_id) {
		// dropping, late response
		return;
	}

	wxDateTime time = ToWxDateTime(data.response_second, data.response_nanosecond);
	wxDateTime start = ToWxDateTime(data.start_second, data.start_nanosecond);
	wxDateTime end = ToWxDateTime(data.end_second, data.end_nanosecond);

	wxLogInfo(_T("Search response dir: %d, start: %ls, end: %ls,  response: %ls"),
			data.direction,
			start.Format().c_str(),
			end.IsValid() ? end.Format().c_str() : L"none",
			time.Format().c_str());

	switch (data.direction) {
		case -1:
			HandleLeftResponse(time);
			break;
		case 1:
			HandleRightResponse(time);
			break;
		default:
			ASSERT(false);
	}
}

void DrawsController::SearchState::HandleLeftResponse(wxDateTime& time) {}

void DrawsController::SearchState::HandleRightResponse(wxDateTime& time) {}

void DrawsController::SearchLeft::Enter(const DTime& search_from) {
	m_search_time = search_from;

	const TimeIndex& index = m_c->GetSelectedDraw()->GetTimeIndex();

	// first check for data between now and next hop
	SendSearchQuery(m_search_time, m_c->GetCurrentTime() - wxTimeSpan::Milliseconds(1), RIGHT);
}

void DrawsController::SearchLeft::HandleRightResponse(wxDateTime& time) {
	if (time.IsValid()) {
		// valid data between searched time and now, jump there (between searched time and now)
		DTime draw_time(m_c->GetPeriod(), time);
		draw_time.AdjustToPeriod();
		m_c->MoveToTime(m_c->ChooseStartDate(draw_time));
		m_c->EnterState(WAIT_DATA_NEAREST, draw_time);
	}

	// no data between now and next hop, jump left
	SendSearchQuery(m_search_time.GetTime(), wxInvalidDateTime, LEFT);
}

void DrawsController::SearchLeft::HandleLeftResponse(wxDateTime& time) {
	if (time.IsValid()) {
		// found data
		DTime draw_time(m_c->GetPeriod(), time);
		draw_time.AdjustToPeriod();
		m_c->MoveToTime(m_c->ChooseStartDate(draw_time));
		m_c->EnterState(WAIT_DATA_NEAREST, draw_time);
	} else {
		// we are on first data, do nothing
		m_c->EnterState(DISPLAY, m_c->GetCurrentTime());
	}
}


void DrawsController::SearchRight::Enter(const DTime& search_from) {
	m_search_time = search_from;
	SendSearchQuery(search_from.GetTime(), wxInvalidDateTime, 1);
}

void DrawsController::SearchRight::HandleRightResponse(wxDateTime& time) {
	if (time.IsValid()) {
		DTime draw_time(m_c->GetPeriod(), time);

		DTime e_time = m_c->GetSelectedDraw()->GetLastTime();
		const TimeIndex& index = m_c->GetSelectedDraw()->GetTimeIndex();
		int n_res_probes = TimeIndex::PeriodMult[m_c->GetPeriod()];
		int d_t = e_time.GetDistance(draw_time);

		int mod = d_t%n_res_probes;
		if (mod != 0) {
			// take ceiling for alignment with start period (AdjustToPeriod but with ceiling not floor)
			d_t = d_t + n_res_probes - mod;
		}

		auto new_start = index.GetNextProbeTime(index.GetStartTime(), d_t);

		m_c->MoveToTime(new_start);
		m_c->EnterState(WAIT_DATA_NEAREST, draw_time);
	} else {
		GoToLatestDate();
	}
}

DTime DrawsController::SearchBoth::FindCloserTime(const DTime& reference, const DTime& left, const DTime& right) {
	if (!left.IsValid())
		return right;
	if (!right.IsValid())
		return left;

	// abs() won't work for wxTimeSpan as it does not work for values less than 0
	wxTimeSpan dl = reference > left? reference.GetTime() - left.GetTime() : left.GetTime() - reference.GetTime();
	wxTimeSpan dr = right > reference? right.GetTime() - reference.GetTime() : reference.GetTime() - right.GetTime();

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
	if (m_right_result == m_search_time && m_right_result.IsValid()) {
		m_c->MoveToTime(m_c->ChooseStartDate(m_right_result, m_start_time));
		m_c->EnterState(WAIT_DATA_NEAREST, m_right_result);
	} else {
		SendSearchQuery(m_search_time.GetTime(),
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
		m_c->GetSelectedDraw()->SetNoData(true);
		m_c->NotifyStateNoData(m_c->GetSelectedDraw());

		if (!SwitchToGraphThatMayHaveData()) {
			m_c->EnterState(STOP, m_search_time);
			m_c->NotifyStateNoData();
		}
	}
}

bool DrawsController::SearchBothPreferCloser::SwitchToGraphThatMayHaveData() {
	//nothing found, so there is no data for this graph
	//anything and there is no current time

	int selected_draw = -1;

	const auto& draws = m_c->GetDraws();
	for (int i = 1; i < m_c->GetActiveDrawsCount(); i++) {
		int j = (m_c->GetSelectedDrawNo() + i) % m_c->GetActiveDrawsCount();
		if (draws[j]->GetEnable() && !draws[j]->GetNoData()) {
			selected_draw = j;
			break;
		}
	}

	if (selected_draw != -1) {
		DTime t = m_c->SetSelectedDraw(selected_draw);
		m_c->EnterState(SEARCH_BOTH, t);
		return true;
	}

	for (int i = 1; i < m_c->GetActiveDrawsCount(); i++) {
		int j = (m_c->GetSelectedDrawNo() + i) % m_c->GetActiveDrawsCount();
		if (!draws[j]->GetNoData()) {
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

void DrawsController::SearchLatest::Enter(const DTime& time) {
	SendSearchQuery(time.GetTime(), wxInvalidDateTime, LEFT);
}

void DrawsController::SearchLatest::HandleLeftResponse(wxDateTime& time) {
	if (!time.IsValid()) {
		m_c->GetSelectedDraw()->SetNoData(true);
		m_c->NotifyStateNoData(m_c->GetSelectedDraw());
		m_c->EnterState(STOP, m_c->GetCurrentTime());
	} else {
		auto result = DTime(m_c->GetPeriod(), time);
		m_c->MoveToTime(m_c->ChooseStartDate(result));
		m_c->EnterState(WAIT_DATA_NEAREST, result);
	}
}

void DrawsController::MoveScreenWindowLeft::Enter(const DTime&) {
	const TimeIndex& index = m_c->GetCurrentTimeWindow();
	m_start_time = index.GetStartTime();
	m_start_time.AdjustToPeriod();
	SendSearchQuery(m_start_time.TimeJustBefore(), wxInvalidDateTime, LEFT);
}

void DrawsController::MoveScreenWindowLeft::HandleLeftResponse(wxDateTime& time) {
	if (!time.IsValid()) {
		m_c->EnterState(DISPLAY, m_c->GetCurrentTime());
		return;
	}

	const TimeIndex& index = m_c->GetCurrentTimeWindow();
	auto tp = index.GetTimePeriod(); 
	auto dp = index.GetDatePeriod();
	auto period = m_c->GetPeriod();

	DTime data_found(period, time);
	data_found.AdjustToPeriod();

	DTime c_start = m_start_time;
	DTime c_time = m_c->GetSelectedDraw()->GetTimeOfIndex(m_c->GetCurrentIndex());

	// take period start on the left of the cursor (but it has to be on the screen)
	DTime c_period_start = index.AdjustToPeriodSpan(c_time);
	if (c_period_start < c_start) {
		c_period_start = c_period_start + tp + dp;
	}

	// this is to be preserved (index of period start probe)
	int c_adj = c_start.GetDistance(c_period_start);

	// find new period start and adjust window to preserve distance to period start
	DTime period_start = index.AdjustToPeriodSpan(data_found);
	DTime window_start = index.GetNextProbeTime(period_start, -c_adj);

	if (data_found < window_start) {
		window_start = period_start - tp - dp;
		window_start = index.GetNextProbeTime(window_start, -c_adj);
	}

	auto no_of_probes = m_c->GetNumberOfValues(period);
	if (window_start.GetDistance(data_found) > no_of_probes) {
		// DST adjustment
		window_start = index.GetNextProbeTime(window_start, TimeIndex::PeriodMult[period]);
	}

	// preserve index (go to index)
	auto c_ind = m_c->GetCurrentIndex();
	auto searched_time = index.GetNextProbeTime(window_start, c_ind);

	searched_time.AdjustToPeriod();

	m_c->MoveToTime(window_start);
	m_c->EnterState(SEARCH_BOTH_PREFER_LEFT, searched_time);
}

void DrawsController::SearchBothPreferLeft::Enter(const DTime& search_from) {
	const TimeIndex& index = m_c->GetCurrentTimeWindow();
	m_search_time = search_from;

	SendSearchQuery(m_search_time.GetTime(), index.GetStartTime(), LEFT);
}

void DrawsController::SearchBothPreferLeft::HandleLeftResponse(wxDateTime& time) {
	DTime left_result(m_c->GetPeriod(), time);
	left_result.AdjustToPeriod();
	if (left_result.IsValid()) {
		m_c->EnterState(WAIT_DATA_NEAREST, left_result);
	} else {
		const TimeIndex& index = m_c->GetSelectedDraw()->GetTimeIndex();
		SendSearchQuery(m_search_time,
			index.GetNextProbeTime(index.GetLastTime()).TimeJustBefore(),
			RIGHT);
	}
}

void DrawsController::SearchBothPreferLeft::HandleRightResponse(wxDateTime& time) {
	DTime right_result(m_c->GetPeriod(), time);
	right_result.AdjustToPeriod();
	if (right_result.IsValid()) {
		m_c->EnterState(WAIT_DATA_NEAREST, right_result);
	} else {
		m_c->EnterState(SEARCH_LEFT, m_c->GetCurrentTime());
	}
}



void DrawsController::MoveScreenWindowRight::Enter(const DTime&) {
	const TimeIndex& index = m_c->GetSelectedDraw()->GetTimeIndex();
	SendSearchQuery(index.GetFirstNotDisplayedTime(),
	        wxInvalidDateTime,
	        RIGHT);
}

void DrawsController::MoveScreenWindowRight::HandleRightResponse(wxDateTime& time) {
	if (!time.IsValid()) {
		m_c->EnterState(DISPLAY, m_c->GetCurrentTime());
		return;
	}

	auto period = m_c->GetPeriod();

	DTime data_found(period, time);
	data_found.AdjustToPeriod();

	const TimeIndex& index = m_c->GetSelectedDraw()->GetTimeIndex();
	DTime c_start = index.GetStartTime();
	DTime c_time = m_c->GetSelectedDraw()->GetTimeOfIndex(m_c->GetCurrentIndex());
	DTime c_period_start = index.AdjustToPeriodSpan(c_time);

	auto tp = index.GetTimePeriod(); 
	auto dp = index.GetDatePeriod();
	if (c_period_start < c_start) {
		c_period_start = c_period_start + tp + dp;
	}

	int c_adj = c_start.GetDistance(c_period_start);

	DTime period_start = index.AdjustToPeriodSpan(data_found);
	DTime window_start = index.GetNextProbeTime(period_start, -c_adj);

	DTime window_end = window_start + tp + dp;
	if (data_found >= window_end) {
		window_start = period_start + tp + dp;
		window_start = index.GetNextProbeTime(window_start, -c_adj);
	}

	window_start.AdjustToPeriod();

	// preserve index (go to index)
	auto c_ind = m_c->GetCurrentIndex();
	auto searched_time = index.GetNextProbeTime(window_start, c_ind);

	m_c->MoveToTime(window_start);
	m_c->EnterState(SEARCH_BOTH_PREFER_RIGHT, searched_time);
}

void DrawsController::SearchBothPreferRight::Enter(const DTime& search_from) {
	const TimeIndex& index = m_c->GetSelectedDraw()->GetTimeIndex();
	m_search_time = search_from;

	SendSearchQuery(m_search_time.GetTime(), index.GetFirstNotDisplayedTime().TimeJustBefore(), RIGHT);
}

void DrawsController::SearchBothPreferRight::HandleRightResponse(wxDateTime& time) {
	DTime right_result(m_c->GetPeriod(), time);
	right_result.AdjustToPeriod();
	if (right_result.IsValid()) {
		m_c->EnterState(WAIT_DATA_NEAREST, right_result);
	} else {
		const TimeIndex& index = m_c->GetCurrentTimeWindow();
		SendSearchQuery(index.GetFirstNotDisplayedTime().TimeJustBefore(),
			index.GetStartTime(),
			LEFT);
	}
}

void DrawsController::SearchBothPreferRight::HandleLeftResponse(wxDateTime& time) {
	DTime left_result(m_c->GetPeriod(), time);
	left_result.AdjustToPeriod();
	if (left_result.IsValid()) {
		m_c->EnterState(WAIT_DATA_NEAREST, left_result);
	} else {
		m_c->EnterState(SEARCH_RIGHT, m_c->GetCurrentTime());
	}
}


DrawsController::DrawsController(ConfigManager *config_manager, DatabaseManager *database_manager) :
	DBInquirer(database_manager),
	ConfigObserver(),
	StateController(),
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
	m_units_count[PERIOD_T_5MINUTE] = TimeIndex::default_units_count[PERIOD_T_5MINUTE];
	m_units_count[PERIOD_T_MINUTE] = TimeIndex::default_units_count[PERIOD_T_MINUTE];
	m_units_count[PERIOD_T_30SEC] = TimeIndex::default_units_count[PERIOD_T_30SEC];
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
	m_states[SEARCH_LATEST] = new SearchLatest();
	m_states[MOVE_SCREEN_RIGHT] = new MoveScreenWindowRight();
	m_states[MOVE_SCREEN_LEFT] = new MoveScreenWindowLeft();
	m_states[SEARCH_BOTH_PREFER_LEFT] = new SearchBothPreferLeft();
	m_states[SEARCH_BOTH_PREFER_RIGHT] = new SearchBothPreferRight();

	for (int i = 0; i < FIRST_UNUSED_STATE_ID; i++)
		m_states[i]->SetDrawsController(this);

	m_state = m_states[STOP];
}

DrawsController::~DrawsController() {
	ChangeObservedParamsRegistration(GetSubscribedParams(), std::vector<TParam*>());

	for (size_t i = 0; i < m_draws.size(); i++)
		delete m_draws[i];

	for (int i = 0; i < FIRST_UNUSED_STATE_ID; i++)
		delete m_states[i];

	m_config_manager->DeregisterConfigObserver(this);
}

void DrawsController::SetCurrentTime(const DTime& time) {
	if (!time.IsValid()) {
		ASSERT(!"Invalid time set!");
		return EnterState(STOP, GetCurrentTime());
	}

	m_current_time = time;
}

void DrawsController::SendQueryToDatabase(DatabaseQuery* query) {
	QueryDatabase(query);
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
			ASSERT(false);
			break;
	}
}

std::vector<TParam*> DrawsController::GetSubscribedParams() const
{
	std::vector<TParam*> v;

	for (int i = 0; i < m_active_draws_count; i++)
		if (m_draws[i]->GetDrawInfo()->IsValid() && m_draws[i]->GetSubscribed())
			v.push_back(m_draws[i]->GetDrawInfo()->GetParam()->GetIPKParam());

	return v;
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

	bool non_fixed;
	m_draws[draw_no]->AddData(query, data_within_view, non_fixed);

	if (no_data)
		m_observers.NotifyNoData(m_draws[draw_no]);

	if (draw_no == m_selected_draw && data_within_view)
		m_state->NewDataForSelectedDraw();

	m_state->NewValuesAdded(m_draws[draw_no], non_fixed);

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
	if (query->search_data.period_type == GetPeriod() && GetCurrentDrawInfo() == query->draw_info) {
		m_state->HandleSearchResponse(query);
		wxLogInfo(_T("Got search response"));
	} else
		wxLogInfo(_T("Ignoring response - wrong draw info/period type"));
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
				+ 2 * index.GetTimePeriod()) {
			auto tp = index.GetTimePeriod(); 
			auto dp = index.GetDatePeriod();
			auto pmtr = TimeIndex::PeriodMult[m_period_type] * index.GetTimeRes();
			auto pmdr = TimeIndex::PeriodMult[m_period_type] * index.GetDateRes(); 
			ret = found_time
					- tp
					- dp
					+ pmtr
					+ pmdr;
		} else
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

namespace {

struct CompareQueryTimes {
	time_t m_second;
	time_t m_nanosecond;
	CompareQueryTimes(const wxDateTime& time) {
		ToNanosecondTime(time, m_second, m_nanosecond);
	}

	bool operator()(const DatabaseQuery::ValueData::V& v1, const DatabaseQuery::ValueData::V& v2) const {
		double second_diff1 = abs(difftime(v1.time_second, m_second));
		double second_diff2 = abs(difftime(v2.time_second, m_second));

		if (second_diff1 < second_diff2)
			return true;
		if (second_diff1 > second_diff2)
			return false;

		double nanosecond_diff1 = abs(difftime(v1.time_nanosecond, m_nanosecond));
		double nanosecond_diff2 = abs(difftime(v2.time_nanosecond, m_nanosecond));

		if (nanosecond_diff1 < nanosecond_diff2)
			return true;
		return false;

	}

};

}

void DrawsController::FetchData() {
	for (size_t i = 0; i < m_draws.size(); i++) {
		DatabaseQuery *q = m_draws[i]->GetDataToFetch(false);
		if (q) {
			std::sort(q->value_data.vv->begin(), q->value_data.vv->end(), CompareQueryTimes(m_state->GetStateTime().GetTime()));
			q->prefix = m_current_prefix.ToStdWstring();
			QueryDatabase(q);
		}
	}
}

void DrawsController::MoveToTime(const DTime &time) {
	if (!time.IsValid()) {
		ASSERT(!"Cannot move to invalid time!");
		return EnterState(STOP, GetCurrentTime());
	}

	std::vector<size_t> moved;
	wxLogInfo(_T("Moving to time: %s"), time.GetTime().Format().c_str());

	Draw *d = GetSelectedDraw();
	if (d->GetBlocked()) {
		d->MoveToTime(time);
		moved.push_back(d->GetDrawNo());
	} else {
		for (size_t i = 0; i < m_draws.size(); i++) {
			if (m_draws[i]->GetBlocked() == false) {
				m_draws[i]->MoveToTime(time);
				if (m_draws[i]->GetDrawInfo()) {
					moved.push_back(i);
				}
			}
		}
	}

	for (auto draw: moved) {
		m_observers.NotifyDrawMoved(m_draws[draw], time.GetTime());
	}
}

void DrawsController::DoSet(DrawSet *set) {
	m_double_cursor = false;

	m_draw_set = set;
	m_current_set_name = set->GetName();
	m_current_prefix = set->GetDrawsSets()->GetPrefix();

	ChangeObservedParamsRegistration(GetSubscribedParams(), {});

	DrawInfoArray *draws = set->GetDraws();

	if (m_draws.size() < draws->size()) {
		Draw* selected_draw = GetSelectedDraw();
		DTime start_time;
		if (selected_draw) {
			start_time = selected_draw->GetStartTime();
		}
		for (size_t i = m_draws.size(); i < draws->size(); i++) {
			Draw* draw = new Draw(this, &m_observers, i);
			if (selected_draw) {
				draw->Set(start_time, m_period_type, GetNumberOfValues(m_period_type));
			}

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
		m_draws[i]->SetSubscribed(false);
	}

}

void DrawsController::ConfigurationWasReloaded(wxString prefix) {
	if (prefix != m_current_prefix)
		return;

	m_active_draws_count = 0;

	DrawsSets *dss = m_config_manager->GetConfigByPrefix(prefix);

	if (!dss) {
		ASSERT(!"Could not get config manager!");
		return EnterState(STOP, m_current_time);
	}

	DrawSetsHash& dsh = dss->GetDrawsSets();
	DrawSetsHash::iterator i = dsh.find(m_current_set_name);
	if (i != dsh.end()) {
		DoSet(i->second);
	} else {
		SortedSetsArray sets = dss->GetSortedDrawSetsNames();
		DoSet(sets[0]);
	}

	if (m_selected_draw >= m_active_draws_count) {
		m_selected_draw = 0;
	}

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

	std::vector<TParam*> previous_params(GetSubscribedParams());

	DoSet(set);

	DisableDisabledDraws();

	if (m_selected_draw >= m_active_draws_count)
		m_selected_draw = 0;

	for (int i = 0; i < m_active_draws_count; i++) {
		m_draws[i]->SetSubscribed(false);
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
	m_current_time = DTime();
	m_current_index = -1;

	for (size_t i = 0; i < m_draws.size(); i++) {
		m_draws.at(i)->SetPeriod(period_type, GetNumberOfValues(period_type));
	}

	for (int i = 0; i < m_active_draws_count; i++)
		m_observers.NotifyPeriodChanged(m_draws[i], period_type);

	if (!state_time.IsValid()) {
		// currently no data in set
		return GoToLatestDate();
	}

	DTime time = m_time_reference.Adjust(period_type, state_time);

	for (size_t i = 0; i < m_draws.size(); i++) {
		m_draws.at(i)->SetStartTime(time);
	}

	MoveToTime(GetCurrentTimeWindow().AdjustToPeriodSpan(time));

	EnterState(SEARCH_BOTH, time);
}

void DrawsController::Set(PeriodType pt, const wxDateTime& time, int draw_to_select) {
	if (!time.IsValid()) {
		ASSERT(!"Cannot set invalid time!");
		return EnterState(STOP, m_current_time);
	}

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
	for (size_t i = 0; i < m_draws.size(); i++) {
		m_draws.at(i)->Set(period_time, pt, GetNumberOfValues(pt));
	}

	for (size_t i = 0; i < m_draws.size(); i++)
		m_observers.NotifyPeriodChanged(m_draws[i], pt);

	m_observers.NotifyDrawSelected(m_draws.at(m_selected_draw));

	MoveToTime(GetCurrentTimeWindow().AdjustToPeriodSpan(period_time));

	EnterState(SEARCH_BOTH, period_time);
}

void DrawsController::Set(DrawSet *set, PeriodType pt, const wxDateTime& time, int draw_to_select) {
	if (!time.IsValid()) {
		ASSERT(!"Cannot set invalid time!");
		return EnterState(STOP, m_current_time);
	}

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
	for (size_t i = 0; i < m_draws.size(); i++) {
		m_draws.at(i)->Set(period_time, pt, GetNumberOfValues(pt));
	}

	for (size_t i = 0; i < m_draws.size(); i++)
		m_observers.NotifyDrawInfoChanged(m_draws[i]);

	wxLogInfo(_T("Set '%s' choosen, selected draw index; %d"), set->GetName().c_str(), draw_to_select);

	MoveToTime(GetCurrentTimeWindow().AdjustToPeriodSpan(period_time));

	EnterState(SEARCH_BOTH, period_time);
}

void DrawsController::Set(int draw_to_select, const wxDateTime& time) {
	if (!time.IsValid()) {
		ASSERT(!"Cannot set invalid time!");
		return EnterState(STOP, m_current_time);
	}

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
	if (!_time.IsValid()) {
		ASSERT(!"Cannot set invalid time!");
		return EnterState(STOP, m_current_time);
	}

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

void DrawsController::EnterState(StateController::STATE state, const DTime &time) {
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
	m_minute = now.GetMinute() % 10;
	m_10minute = now.GetMinute() - (now.GetMinute() % 10);
	m_second = now.GetSecond() % 10;
	m_10second = now.GetSecond() - (now.GetSecond() % 10);
	m_milisecond = now.GetMillisecond();
}

void DrawsController::TimeReference::Update(const DTime& time) {
	const wxDateTime& wxt = time.GetTime();
	switch (time.GetPeriod()) {
		case PERIOD_T_30SEC:
		case PERIOD_T_MINUTE:
			m_milisecond = wxt.GetMillisecond();
		case PERIOD_T_5MINUTE:
			m_second = wxt.GetSecond() % 10;
		case PERIOD_T_30MINUTE:
			m_10second = wxt.GetSecond() - (wxt.GetSecond() % 10);
			m_minute = wxt.GetMinute() % 10;
		case PERIOD_T_DAY:
			m_10minute = wxt.GetMinute() - (wxt.GetMinute() % 10);
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
			ASSERT(false);
	}
}

DTime DrawsController::TimeReference::Adjust(PeriodType pt, const DTime& time) {
	if (!time.IsValid()) {
		ASSERT(!"Cannot adjust invalid time!");
		return DTime(pt, time.GetTime());
	}

	wxDateTime t = time.GetTime();

	switch (pt) {
		default:
			break;
		case PERIOD_T_30SEC:
		case PERIOD_T_MINUTE:
			t.SetMillisecond(m_milisecond);
		case PERIOD_T_5MINUTE:
			t.SetSecond(m_second + m_10second);
			t.SetMinute(m_minute + m_10minute);
			t.SetHour(m_hour);
			t.SetDay(std::min(m_day, (int)wxDateTime::GetNumberOfDays(t.GetMonth(), t.GetYear())));
			break;
		case PERIOD_T_30MINUTE:
			t.SetSecond(m_10second);
			t.SetMinute(m_minute + m_10minute);
			t.SetHour(m_hour);
			t.SetDay(std::min(m_day, (int)wxDateTime::GetNumberOfDays(t.GetMonth(), t.GetYear())));
			break;
		case PERIOD_T_DAY:
			t.SetMinute(m_10minute);
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

DrawInfo* DrawsController::GetCurrentDrawInfo() {
	if (m_selected_draw >= 0)
		return m_draws[m_selected_draw]->GetDrawInfo();
	else
		return NULL;
}

const DTime& DrawsController::GetCurrentTime() const {
	return m_current_time;
}

const TimeIndex& DrawsController::GetCurrentTimeWindow() const {
	return m_draws[m_selected_draw]->GetTimeIndex();
}

bool DrawsController::GetDoubleCursor() const {
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

const PeriodType& DrawsController::GetPeriod() const {
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

size_t DrawsController::GetNumberOfValues(const PeriodType& period_type) const {
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
		if (m_selected_draw < 0) {
			ASSERT(!"Draw invalid or not set!");
			return EnterState(STOP, m_current_time);
		}

		GoToLatestDate();
	} else {
		FetchData();
		EnterState(WAIT_DATA_NEAREST, m_state->GetStateTime());
	}
}

void DrawsController::DrawInfoAverageValueCalculationChanged(DrawInfo* di) {
	for (int i = 0; i < m_active_draws_count; i++) {
		if (!m_draws[i]->GetEnable())
			continue;
		if (m_draws[i]->GetDrawInfo() != di)
			continue;

		m_draws[i]->AverageValueCalculationMethodChanged();

		m_observers.NotifyAverageValueCalculationMethodChanged(m_draws[i]);
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
	std::function<bool(const Draw*, const Draw*)> cmp_func;
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

void DrawsController::ParamDataChanged(TParam* param) {
	m_state->ParamDataChanged(param);
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

void DrawsObservers::NotifyAverageValueCalculationMethodChanged(Draw *draw) {
	std::for_each(m_observers.begin(), m_observers.end(), std::bind2nd(std::mem_fun(&DrawObserver::AverageValueCalculationMethodChanged), draw));
}
