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
 * draw3
 * SZARP
 
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 *
 * $Id$
 */

#include <algorithm>

#include <wx/clipbrd.h>

#include "drawswdg.h"

#include <wx/intl.h>
#include <wx/bitmap.h>
#include <wx/log.h>
#include <wx/xrc/xmlres.h>

#ifdef __WXMSW__
#include <windows.h>
#endif

#include <math.h>

#include "cconv.h"
#include "draw.h"
#include "infowdg.h"
#include "timewdg.h"
#include "disptime.h"
#include "drawpnl.h"
#include "summwin.h"
#include "piewin.h"
#include "relwin.h"
#include "seldraw.h"
#include "datechooser.h"
#include "drawprint.h"
#include "selset.h"
#include "drawfrm.h"
#include "drawurl.h"
#include "remarks.h"

#define DISPLAY_TIMER_ID 1001
#define KEYBOARD_TIMER_ID 1002

#define FULL_SCREEN_SPEED 1500
/**< Values (in ms) for cursor movement speed. */
int CursorMovementSpeed[PERIOD_T_LAST] = {
	FULL_SCREEN_SPEED / 12,	/* year */
	FULL_SCREEN_SPEED / 30,	/* month */
	FULL_SCREEN_SPEED / 21,	/* week */
	FULL_SCREEN_SPEED / 160,	/* day */
	FULL_SCREEN_SPEED / 50,	/* season */
	FULL_SCREEN_SPEED / 100,	/* other */
};

BEGIN_EVENT_TABLE(DrawsWidget, wxEvtHandler)
    EVT_TIMER(DISPLAY_TIMER_ID, DrawsWidget::OnDisplayTimerEvent)
    EVT_TIMER(KEYBOARD_TIMER_ID, DrawsWidget::OnKeyboardTimerEvent)
END_EVENT_TABLE()

const size_t DrawsWidget::default_units_count[PERIOD_T_LAST] = { 12, 31, 7, 24, 40 };

DrawsWidget::DrawsWidget(DrawPanel * parent, ConfigManager *cfg, DatabaseManager* dm, DrawGraphs* graphs, SummaryWindow *smw, PieWindow *pw, RelWindow *rw, RemarksFetcher *rf, wxWindowID id, InfoWidget * info, PeriodType pt, time_t time, int selected_draw) : 
	DBInquirer(dm), m_graphs(graphs), m_cfg(cfg), m_parent(parent), m_action(NONE), m_summ_win(smw), m_pie_win(pw), m_rel_win(rw), m_remarks_fetcher(rf), m_switch_selected_draw(selected_draw)
{
	m_units_count[PERIOD_T_YEAR] = default_units_count[PERIOD_T_YEAR];
	m_units_count[PERIOD_T_MONTH] = default_units_count[PERIOD_T_MONTH];
	m_units_count[PERIOD_T_WEEK] = default_units_count[PERIOD_T_WEEK];
	m_units_count[PERIOD_T_DAY] = default_units_count[PERIOD_T_DAY];
	m_units_count[PERIOD_T_SEASON]  = default_units_count[PERIOD_T_SEASON];

	m_selected_draw = -1;

	/* Set period to 1 year and end time to now. */
	if (pt != PERIOD_T_OTHER)
		this->period = pt;
	else
		this->period = PERIOD_T_YEAR;

	this->info = info;

	m_displaytime = NULL;

	m_display_timer = new wxTimer(this, DISPLAY_TIMER_ID);
	m_keyboard_timer = new wxTimer(this, KEYBOARD_TIMER_ID);

	SetDisplayTimer();

	m_seldraw = NULL;

	m_selset = NULL;

	wxDateTime now = wxDateTime::Now();
	m_reference.m_month = now.GetMonth();
	m_reference.m_day = now.GetDay();
	m_reference.m_wday = now.GetWeekDay();
	m_reference.m_hour = now.GetHour();
	m_reference.m_minute = now.GetMinute();

	m_filter = 0;

	m_switch_period = pt;

        if (time <= 0)
		m_switch_date = wxDateTime::Now();
	else
		m_switch_date = wxDateTime(time);

	m_switch_selected_draw_info = NULL;

	m_widget_freshly_created = true;

}

DrawsWidget::~DrawsWidget()
{
	delete m_display_timer;
	delete m_keyboard_timer;

	for (size_t i = 0; i < m_draws.GetCount(); ++i) 
		delete m_draws[i];

}

void DrawsWidget::DatabaseResponse(DatabaseQuery * query)
{
	size_t d = query->draw_no;
	if (d < m_draws.GetCount())
		m_draws[d]->DatabaseResponse(query);
	else
		delete query;
}

#if 0
void DrawsWidget::CleanDatabase(DatabaseQuery * query)
{
	m_parent->CleanDatabase(query);
}
#endif

void DrawsWidget::ClearCache()
{
	DrawInfo *di = m_draws[0]->GetDrawInfo();

	for (size_t i = 0; i < m_draws.GetCount(); ++i) {
		DrawInfo *_di = m_draws[i]->GetDrawInfo();
		if (di->GetBasePrefix() != _di->GetBasePrefix()) {
			DatabaseQuery* q = new DatabaseQuery;
			q->type = DatabaseQuery::CLEAR_CACHE;
			q->prefix = _di->GetBasePrefix().c_str();
			QueryDatabase(q);
			di = _di;
		}
	}

	DatabaseQuery* q = new DatabaseQuery;
	q->type = DatabaseQuery::CLEAR_CACHE;
	q->draw_info = di;
	q->prefix = di->GetBasePrefix();
	
	QueryDatabase(q);
}

void DrawsWidget::QueryDatabase(DatabaseQuery * query)
{
	DBInquirer::QueryDatabase(query);
}

void DrawsWidget::QueryDatabase(std::list<DatabaseQuery*> &qlist)
{
	DBInquirer::QueryDatabase(qlist);
}

time_t DrawsWidget::GetCurrentTime()
{
	if (m_selected_draw < 0 || m_selected_draw >= (int)m_draws.GetCount())
		return -1;

	const wxDateTime & t = m_draws[m_selected_draw]->GetCurrentTime();
	return t.IsValid()? t.GetTicks() : -1;

}

DrawInfo *DrawsWidget::GetCurrentDrawInfo()
{
	if (m_selected_draw < 0 || m_selected_draw >= (int)m_draws.GetCount())
		return NULL;

	return m_draws[m_selected_draw]->GetDrawInfo();
}

void DrawsWidget::OnKeyboardTimerEvent(wxTimerEvent & event)
{
	switch (m_action) {
	case CURSOR_LEFT:
	case CURSOR_LEFT_KB:
		MoveCursorLeft();
		break;
	case CURSOR_RIGHT:
	case CURSOR_RIGHT_KB:
		MoveCursorRight();
		break;
	default:
		break;
	}

	m_keyboard_timer->Start(CursorMovementSpeed[period], wxTIMER_ONE_SHOT);
}

void DrawsWidget::OnDisplayTimerEvent(wxTimerEvent & event)
{
	SetDisplayTimer();
}

size_t DrawsWidget::GetNumberOfUnits() {
	return m_units_count[period];
}

void DrawsWidget::SetNumberOfUnits(size_t count) {
	m_units_count[period] = count;
	SetPeriod(period, true);
}

size_t DrawsWidget::GetNumberOfValues() {
	return m_units_count[period] * Draw::PeriodMult[period];
}

bool DrawsWidget::IsDefaultNumberOfValues() {
	return m_units_count[period] == default_units_count[period];

}

void DrawsWidget::SwitchToPeriod(PeriodType period, const wxDateTime& current_time) {
	this->period = period;

	Draw* draw = GetSelectedDraw();
	if (draw) 
		DoubleCursorSet(false);	

	if (m_draws.GetCount() >= 0) for (size_t i = 0; i <= m_draws.GetCount(); ++i) {
		if ((int)i == m_selected_draw)
			continue;

		size_t j = i;
		if (j == m_draws.GetCount())
			j = m_selected_draw;
				
		m_draws[j]->SetPeriod(period, current_time);
	}

	m_timewdg->Select(period, false);

	int sid = 0;
	wxMenuBar *mb = m_parent->GetMenuBar();
	switch (period) {
		case PERIOD_T_YEAR:
			sid = XRCID("YEAR_RADIO");
			break;
		case PERIOD_T_MONTH:
			sid = XRCID("MONTH_RADIO");
			break;
		case PERIOD_T_WEEK:
			sid = XRCID("WEEK_RADIO");
			break;
		case PERIOD_T_DAY:
			sid = XRCID("DAY_RADIO");
			break;
		case PERIOD_T_SEASON:
			sid = XRCID("SEASON_RADIO");
			break;
		default:
			assert(false);
			break;
	} 

	mb->FindItem(sid)->Check(true);

}

void DrawsWidget::SetPeriod(PeriodType period, bool force)
{

	assert(period != PERIOD_T_OTHER);

	if (this->period == period && !force)
		return;

	StopAllDraws();

	SwitchToPeriod(period, GetCurrentTime());

	StartAllDraws();

	m_graphs->Refresh();
}

void DrawsWidget::SetDrawClear()
{
	m_draws_proposed.Empty();
}

void DrawsWidget::SetDrawAdd(DrawInfo * draw)
{
	m_draws_proposed.Add(draw);
}

void DrawsWidget::StopAllDraws()
{
	for (size_t i = 0; i < m_draws.size(); ++i)
		m_draws[i]->Stop();
}

void DrawsWidget::StartAllDraws()
{
	for (size_t i = 0; i < m_draws.size(); ++i)
		m_draws[i]->Start();
}


bool DrawsWidget::IsDrawBlocked(size_t index) {
	assert(index < m_draws.GetCount());

	return m_draws[index]->GetBlocked();

}

void DrawsWidget::SwitchCurrentDrawBlock() {
	if (m_selected_draw == -1)
		return;

	BlockDraw(m_selected_draw, !m_draws[m_selected_draw]->GetBlocked());
}

void DrawsWidget::BlockDraw(size_t index, bool block) {
	assert(index < m_draws.GetCount());

	if (m_draws[index]->GetBlocked() == block)
		return;

	Draw* draw = m_draws[index];

	draw->SetBlocked(block);

	if (!block) {
		if ((int)index == m_selected_draw)
			//sync all non-blocked draws with this draw
			NotifyScreenMoved(m_draws[m_selected_draw]->GetTimeOfIndex(0));
		else
			//sync this draw with the selected draw
			m_draws[index]->MoveToTime(m_draws[m_selected_draw]->GetTimeOfIndex(0));
	}

	m_seldraw->SetBlocked(index, block);
}

void DrawsWidget::SwitchToDrawInfoUponSetChange(DrawInfo *di) {
	m_switch_selected_draw_info = di;
}	

void DrawsWidget::SetDrawApply()
{
	if (m_widget_freshly_created) {
		DatabaseQuery* q = new DatabaseQuery;
		q->type = DatabaseQuery::CHECK_CONFIGURATIONS_CHANGE;
		q->draw_info = NULL;
		QueryDatabase(q);
		m_widget_freshly_created = false;
	}

	Draw* draw = GetSelectedDraw();
	if (draw && draw->IsDoubleCursor()) {
		draw->StopDoubleCursor();
		m_parent->UncheckSplitCursor();
		info->DoubleCursorMode(false);
	}

	StopAllDraws();

	wxDateTime st = wxInvalidDateTime;
	for (size_t i = 0; i < m_draws.GetCount(); ++i)
		if (!m_draws[i]->GetBlocked()) {
			st = m_draws[i]->GetTimeOfIndex(0);
			break;
		}

	wxDateTime t;
	if (m_selected_draw >= 0 && m_selected_draw < (int)m_draws.GetCount())
		t = m_draws[m_selected_draw]->GetCurrentTime();

	if (m_switch_date.IsValid())
		t = m_switch_date;

	if (t.IsValid() == false)
		t = wxDateTime::Now();

	size_t pc = m_draws.GetCount();
	int previous = m_selected_draw;

	if (m_draws_proposed.GetCount() > m_draws.GetCount()) {

		size_t j = m_draws.GetCount();

		m_draws.SetCount(m_draws_proposed.GetCount());

		for (size_t i = j; i < m_draws_proposed.GetCount(); ++i) {
			m_draws[i] = new Draw(this, &m_reference, i);
			m_draws[i]->SetPeriod(period, t);
			m_draws[i]->SetDraw(m_draws_proposed[i]);
			m_draws[i]->SetFilter(m_filter);

		}

	}

	for (size_t i = pc; i < m_draws_proposed.GetCount(); ++i) {
		m_draws[i]->SetDraw(m_draws_proposed[i]);
		m_draws[i]->SetEnable(true);
		m_draws[i]->SetPeriod(period, t);
		if (st.IsValid())
			m_draws[i]->MoveToTime(st);
		m_draws[i]->SetFilter(m_filter);
		m_summ_win->Attach(m_draws[i]);
		m_pie_win->Attach(m_draws[i]);
		m_rel_win->Attach(m_draws[i]);
	}

	for (size_t i = m_draws_proposed.GetCount(); i < pc; ++i) {
		m_summ_win->Detach(m_draws[i]);
		m_pie_win->Detach(m_draws[i]);
		m_rel_win->Detach(m_draws[i]);
	}

	for (size_t i = 0; i < std::min(pc, m_draws_proposed.GetCount()); i++) {
		bool blocked = m_draws[i]->GetBlocked();
		m_draws[i]->SetBlocked(false);
		if (blocked && st.IsValid())
			m_draws[i]->MoveToTime(st);
		m_draws[i]->SetEnable(true);
		m_draws[i]->SetDraw(m_draws_proposed[i]);
	}

	if ((m_draws.GetCount() > 0) && ((m_selected_draw < 0) || (m_selected_draw >= (int)m_draws_proposed.GetCount())))
		m_selected_draw = 0;

	for (size_t i = m_draws_proposed.GetCount(); i < pc; i++)
		delete m_draws[i];

	if (pc > m_draws_proposed.GetCount())
		m_draws.RemoveAt(m_draws_proposed.GetCount(), pc - m_draws_proposed.GetCount());

	m_nodata = false;

	m_graphs->SetDrawsChanged(m_draws);

	if (m_switch_selected_draw_info != NULL) for (size_t i = 0; i < m_draws_proposed.GetCount(); i++)
		if (m_draws_proposed[i] == m_switch_selected_draw_info) {
			m_switch_selected_draw = i;
			break;
		}
	m_switch_selected_draw_info = NULL;

	if (m_switch_selected_draw >= 0) {
		if (m_switch_selected_draw >= (int)m_draws_proposed.GetCount())
			m_selected_draw = 0;
		else
			m_selected_draw = m_switch_selected_draw;
		m_switch_selected_draw = -1;
	}

	if (m_switch_date.IsValid()) {
		t = m_switch_date;
		m_switch_date = wxInvalidDateTime;
	}

	if (m_switch_period != PERIOD_T_OTHER) {
		SwitchToPeriod(m_switch_period, t);
		m_switch_period = PERIOD_T_OTHER;
	}

	int sel = m_selected_draw;
	m_selected_draw = -1;

	for (size_t i = 0; i < m_draws.GetCount(); i++) {
		DrawInfo *di = m_draws[i]->GetDrawInfo();
		if (m_disabled_draws.find(std::make_pair(di->GetSetName(), std::make_pair(di->GetName(), i))) 
				!= m_disabled_draws.end()) {
			m_seldraw->SetChecked(i, false);
			m_draws[i]->SetEnable(false);
		}
	}

	if (sel >= 0) {

		if (m_draws[sel]->GetEnable() == false)	{
			bool is_enabled = false;
			for (size_t i = 0; i < m_draws.GetCount(); i++) {
				if ((int) i == sel)
					continue;
				if (m_draws[i]->GetEnable()) {
					sel = i;
					is_enabled = true;
					break;
				}
			}

			if (is_enabled == false) {
				m_seldraw->SetChecked(sel, true);
				m_draws[sel]->SetEnable(true);
			}
		}
		m_selected_draw = sel;
	} 

	if (previous != m_selected_draw) {
		if (previous != -1 && (size_t) previous < m_draws.GetCount()) {
			m_draws[previous]->Deselect();
			DetachObservers(previous);
		}
		m_draws[m_selected_draw]->Select(t);
		AttachObservers(m_selected_draw);
	}

	StartAllDraws();

}

void DrawsWidget::SelectDraw(wxString name)
{	
	size_t i;
	for (i = 0; i <  m_draws.GetCount(); i++)
		if (m_draws[i]->GetDrawInfo()->GetName()==name)
			break;
	if (i >= m_draws.GetCount())
		return;
	else
		Select(i);

}

void DrawsWidget::SelectDraw(int idx, bool move_time, wxDateTime move_datetime)
{	
	if (idx >= 0 && idx < (int)m_draws.GetCount())
		Select(idx, move_time, move_datetime);
}

void DrawsWidget::Select(int i, bool move_time, wxDateTime move_datetime)
{
	int previous = m_selected_draw;
	m_selected_draw = i;

	if (previous != -1) {
		if (m_selected_draw != previous) {
			StopAllDraws();

			DetachObservers(previous);

			int index = m_draws[previous]->GetCurrentIndex();

			wxDateTime t;
			if (move_time) {
				t = move_datetime;
			} else {
				t = m_draws[m_selected_draw]->GetTimeOfIndex(index);
			}

			m_draws[previous]->Deselect();

			m_draws[m_selected_draw]->Select(t);

			AttachObservers(m_selected_draw);

			StartAllDraws();
		} else {
			if (move_time)
				m_draws[m_selected_draw]->SetCurrentTime(move_datetime);
		}

	}

}


void DrawsWidget::SelectNextDraw()
{

	if (m_selected_draw == -1)
		return;

	int i;
	for (i = m_selected_draw + 1; i < (int)m_draws.GetCount(); i++)
		if (m_draws[i]->GetEnable())
			break;

	if ((i >= (int)m_draws.GetCount())) {
		for (i = 0; i < (int)m_draws.GetCount(); i++)
			if (m_draws[i]->GetEnable())
				break;

		if (i >= (int)m_draws.GetCount()) {
			m_nodata = true;
#if 0
			Refresh();
#endif
			return;
		}

	}
	Select(i);

}

void DrawsWidget::SelectPreviousDraw()
{
	if (m_selected_draw == -1)
		return;
	
	int i;

	for (i = m_selected_draw - 1; i >= 0; i--)
		if (m_draws[i]->GetEnable())
			break;

	if (i < 0) {
		for (i = m_draws.GetCount() - 1; i >= 0; i--)
			if (m_draws[i]->GetEnable())
				break;
		
		if (i < 0) {
			m_nodata = true;
#if 0
			Refresh();
#endif
			return;
		}
	}
	Select(i);
}

void DrawsWidget::NotifyScreenMoved(const wxDateTime & time)

{
#if 0
	if (m_draws[m_selected_draw]->GetBlocked())
		return;
#endif

	for (size_t i = 0; i < m_draws.GetCount(); ++i)
		m_draws[i]->MoveToTime(time);
}

void DrawsWidget::NotifyDragStats(int disp) {
	for (size_t i = 0; i < m_draws.GetCount(); ++i)
		m_draws[i]->DragStats(disp);
}

void DrawsWidget::NotifyDoubleCursorEnabled(int idx) {
	for (size_t i = 0; i < m_draws.GetCount(); ++i)
		m_draws[i]->DoubleCursorSet(idx);
}

void DrawsWidget::NotifyDoubleCursorDisabled() {
	for (size_t i = 0; i < m_draws.GetCount(); ++i)
		m_draws[i]->DoubleCursorStopped();
}

void DrawsWidget::MoveCursorLeft(int n)
{
	if (m_selected_draw < 0 || m_nodata)
		return;

	m_draws[m_selected_draw]->MoveCursorLeft(n);
}

void DrawsWidget::StopMovingCursor() {
	switch (m_action) {
		case CURSOR_LEFT:
		case CURSOR_RIGHT:
			m_action = NONE;
			break;
		default:
			break;
	}
}

void DrawsWidget::MoveCursorRight(int n)
{
	if (m_selected_draw < 0 || m_nodata)
		return;

	m_draws[m_selected_draw]->MoveCursorRight(n);
}

void DrawsWidget::SetKeyboardAction(ActionKeyboardType action)
{
	if (m_action == action)
		return;

	wxLogInfo(_T("DEBUG: Keyboard action %d requested"), action);

	switch (action) {
	case NONE:
		if (m_action >= CURSOR_LEFT_KB) {
			m_action = NONE;
			m_keyboard_timer->Stop();
		}
		break;
	case CURSOR_LEFT_KB:
		m_action = action;
		m_keyboard_timer->Start(FULL_SCREEN_SPEED / 3,
					wxTIMER_ONE_SHOT);
		MoveCursorLeft();
		break;
	case CURSOR_RIGHT_KB:
		m_action = action;
		m_keyboard_timer->Start(FULL_SCREEN_SPEED / 3,
					wxTIMER_ONE_SHOT);
		MoveCursorRight();
		break;
	case CURSOR_LONG_LEFT_KB:
		m_action = action;
		m_keyboard_timer->Start(FULL_SCREEN_SPEED / 3,
					wxTIMER_ONE_SHOT);
		MoveCursorLeft(6);
		break;
	case CURSOR_LONG_RIGHT_KB:
		m_action = action;
		m_keyboard_timer->Start(FULL_SCREEN_SPEED / 3,
					wxTIMER_ONE_SHOT);
		MoveCursorRight(6);
		break;
	case CURSOR_HOME_KB:
		m_action = action;
		MoveCursorBegin();
		break;
	case CURSOR_END_KB:
		m_action = action;
		MoveCursorEnd();
		break;
	case SCREEN_LEFT_KB:
		m_action = action;
		MoveScreenLeft();
		break;
	case SCREEN_RIGHT_KB:
		m_action = action;
		MoveScreenRight();
		break;
	default:
		break;
	}
}

int DrawsWidget::SetDrawDisable(size_t index)
{
	if ((m_selected_draw == (int)index && !m_draws[index]->HasNoData())
			|| index >= m_draws.GetCount())
		return 0;

	m_draws[index]->SetEnable(false);

	DrawInfo *di = m_draws[index]->GetDrawInfo();
	m_disabled_draws[std::make_pair(di->GetSetName(), std::make_pair(di->GetName(), index))] = true;

	return 1;
}

void DrawsWidget::SetDrawEnable(size_t index)
{
	if (index < m_draws.GetCount() && !m_draws[index]->HasNoData()) {
		m_draws[index]->SetEnable(true);

		DrawInfo *di = m_draws[index]->GetDrawInfo();
		m_disabled_draws.erase(std::make_pair(di->GetSetName(), std::make_pair(di->GetName(), index)));

	}
}

void DrawsWidget::SetDisplayTimer()
{
	if (m_displaytime) {
		m_displaytime->Update();
	}

	m_display_timer->Start((60 - wxDateTime::Now().GetSecond()) *
		1000, wxTIMER_ONE_SHOT);

	RefreshData(true);
}

void DrawsWidget::ParamHasNoData()
{
	m_seldraw->SetDrawEnable(m_selected_draw, false);
	m_draws[m_selected_draw]->SetEnable(false);
	SelectNextDraw();
}

void DrawsWidget::SetDisplayTimeWidget(DisplayTimeWidget * dtw)
{
	m_displaytime = dtw;
}

void DrawsWidget::MoveScreenRight()
{
	if (m_selected_draw < 0)
		return;
	m_draws[m_selected_draw]->MoveScreenRight();
}

void DrawsWidget::MoveScreenLeft()
{
	if (m_selected_draw < 0 || m_nodata)
		return;
	m_draws[m_selected_draw]->MoveScreenLeft();
}

void DrawsWidget::MoveCursorBegin()
{
	if (m_selected_draw < 0 || m_nodata)
		return;
	m_draws[m_selected_draw]->MoveCursorBegin();
}

void DrawsWidget::MoveCursorEnd()
{
	if (m_selected_draw < 0 || m_nodata)
		return;
	m_draws[m_selected_draw]->MoveCursorEnd();
}

void DrawsWidget::AttachObservers(int i) {
	info->Attach(m_draws[i]);
	m_remarks_fetcher->Attach(m_draws[i]);
	m_graphs->Selected(i);
		
}

void DrawsWidget::DetachObservers(int i) {
	info->Detach(m_draws[i]);
	m_remarks_fetcher->Detach(m_draws[i]);
	m_graphs->Deselected(i);
}
	
void DrawsWidget::SetSelWidgets(SelectDrawWidget *seldraw, SelectSetWidget *selset, TimeWidget *timewdg) {
	assert(seldraw);
	m_seldraw = seldraw;
	assert(selset);
	m_selset = selset;
	assert(timewdg);
	m_timewdg = timewdg;
}

Draw* DrawsWidget::GetSelectedDraw() {
	if (m_selected_draw >= 0)
		return m_draws[m_selected_draw];
	else 
		return NULL;
}

int DrawsWidget::GetSelectedDrawIndex() {
	return m_selected_draw;
}

void DrawsWidget::OnJumpToDate() {
	if (m_selected_draw < 0)
		return;

	wxDateTime date = m_draws[m_selected_draw]->GetCurrentTime();

	if (date.IsValid() == false)
		date = wxDateTime::Now();

	DateChooserWidget *dcw = 
		new DateChooserWidget(
				m_parent,
				_("Select date"),
				10,
				wxDateTime(1, 1, 1990).GetTicks(),
				wxDateTime::Now().GetTicks()
		);
	
	bool ret = dcw->GetDate(date);
	delete dcw;

	if (ret == false)
		return;

	m_draws[m_selected_draw]->SetCurrentTime(date);

}

bool DrawsWidget::DoubleCursorSet(bool enable) {
	Draw* draw = GetSelectedDraw();
	assert(draw);

	if (enable) {
		if (draw->IsDoubleCursor())
			return true;
		if (draw->StartDoubleCursor() == false)
			return false;
		info->DoubleCursorMode(true);
		m_parent->CheckSplitCursor();
		return true;
	} else {
		if (!draw->IsDoubleCursor())
			return false;

		draw->StopDoubleCursor();
		m_parent->UncheckSplitCursor();
		info->DoubleCursorMode(false);
		return  false;
	}

}

bool DrawsWidget::ToggleSplitCursor() {
	if (m_nodata == true) {
		wxBell();
		return false;
	}

	Draw* draw = GetSelectedDraw();
	if (draw == NULL) {
		wxBell();
		return false;
	}

	bool is_double = draw->IsDoubleCursor();


	if (is_double) {
		return DoubleCursorSet(false);
	} else {
		if (DoubleCursorSet(true) == false) {
			wxBell();
			return false;
		} else
			return true;
	}

}

bool DrawsWidget::IsNoData() {
	return m_nodata;
}

bool DrawsWidget::IsDoubleCursor() { 
	Draw* draw = GetSelectedDraw();

	if (draw == NULL)
		return false;

	return draw->IsDoubleCursor();
}

void DrawsWidget::Print(bool preview) {
	if (preview)
		Print::DoPrintPreviev(m_draws, m_draws.GetCount());
	else
		Print::DoPrint(m_parent, m_draws, m_draws.GetCount());
}

		
size_t DrawsWidget::GetDrawsCount() {
	return m_draws.GetCount();
}

void DrawsWidget::SetFilter(int f) {
	for (size_t i = 0; i < m_draws.GetCount(); ++i) {
		m_draws[i]->SetFilter(f);
	}
	m_filter = f;
}

DrawInfo* DrawsWidget::GetDrawInfo(int index) {
	if (index < 0 || index >= (int)m_draws.GetCount())
		return NULL;

	return m_draws[index]->GetDrawInfo();
}

void DrawsWidget::RefreshData(bool auto_refresh)
{
	if (m_selected_draw < 0 || m_nodata)
		return;

	if (auto_refresh == false) {
		DrawInfo *di = m_draws[0]->GetDrawInfo();

		for (size_t i = 0; i < m_draws.GetCount(); ++i) {
			DrawInfo *_di = m_draws[i]->GetDrawInfo();
			if (di->GetBasePrefix() != _di->GetBasePrefix()) {
				DatabaseQuery* q = new DatabaseQuery;
				q->type = DatabaseQuery::RESET_BUFFER;
				q->draw_info = di;
				QueryDatabase(q);

				di = _di;
			}
		}

		DatabaseQuery* q = new DatabaseQuery;
		q->type = DatabaseQuery::RESET_BUFFER;
		q->draw_info = di;
		QueryDatabase(q);
	}

	for (size_t i = 0; i < m_draws.GetCount(); ++i) 
		m_draws[i]->RefreshData(auto_refresh);

	if (!auto_refresh) 
		m_graphs->FullRefresh();
}

#if 0
#ifndef NO_GSTREAMER
void DrawsWidget::StopDance() {
	delete m_graphs_bitmap;
	m_dancing = false;

	gst_element_set_state(m_gstreamer_bin, GST_STATE_NULL);
	gst_object_unref(m_gstreamer_bin);

	Refresh();
	return;
}

void gstreamer_new_pad(GstElement * element, GstPad * pad, gpointer data)
{
	GstPad *sinkpad;
	/* We can now link this pad with the audio decoder */

	sinkpad = gst_element_get_pad((GstElement*)data, "sink");
	gst_pad_link(pad, sinkpad);

	gst_object_unref(sinkpad);
}

gboolean gstreamer_message_handler(GstBus * bus, GstMessage * message, gpointer data)
{
        if (message->type == GST_MESSAGE_ELEMENT) {
                const GstStructure *s = gst_message_get_structure(message);
                const gchar *name = gst_structure_get_name(s);

                if (strcmp(name, "spectrum") == 0) {

			DrawsWidget *drawswidget = (DrawsWidget*)(data);
			std::vector<float>& spec_vals = drawswidget->GetSpectrumVals();

                        const GValue *list;
                        const GValue *value;
                        guint i;

                        list = gst_structure_get_value(s, "magnitude");
                        for (i = 0; i < spec_vals.size(); ++i) {
                                value = gst_value_list_get_value(list, i);
                                spec_vals[i] = g_value_get_float(value);
                        }
			drawswidget->Refresh();
                }
        }
        return TRUE;
}

void DrawsWidget::StartDance() {
	int w,h;
	wxMemoryDC dc;

	wxFileDialog dialog(this, _("Choose file to play:"), wxEmptyString, wxEmptyString, _T("Ogg files (*.ogg)|*.ogg"));		
	dialog.CentreOnParent();
	dialog.SetDirectory(wxGetHomeDir());

	if (dialog.ShowModal() != wxID_OK)
		return;

	GstElement *src = NULL, *parser = NULL, *spectrum = NULL, *conv = NULL, *decoder = NULL, *sink = NULL;
	GstBus *bus = NULL;

	m_gstreamer_bin = gst_pipeline_new("bin");
	if (m_gstreamer_bin == NULL)
		goto error;

	src = gst_element_factory_make("filesrc", "src");
	if (src == NULL) {
		wxLogError(_T("Failed to create files source factory"));
		goto error;
	}
	g_object_set(G_OBJECT(src), "location", SC::S2A(dialog.GetPath()).c_str(), NULL);

	parser = gst_element_factory_make("oggdemux", "ogg-parser");
	if (parser == NULL) {
		wxLogError(_T("Failed to create oggdemux object"));
		goto error;
	}

	decoder = gst_element_factory_make("vorbisdec", "vorbis-decoder");
	if (decoder == NULL) {
		wxLogError(_T("Failed to create vorbis-decoder object"));
		goto error;
	}

	conv = gst_element_factory_make("audioconvert", "converter");
	if (conv == NULL) {
		wxLogError(_T("Failed to create audioconverter object"));
		goto error;
	}

	spectrum = gst_element_factory_make("spectrum", "spectrum");
	if (spectrum == NULL) {
		wxLogError(_T("Failed to create spectrum analyzer"));
		goto error;
	}

	sink = gst_element_factory_make("alsasink", "alsa-output");
	if (sink == NULL) {
		wxLogError(_T("Failed to create alasa output sink"));
		goto error;
	}

	gst_bin_add_many(GST_BIN(m_gstreamer_bin), src, parser, decoder, conv, spectrum, sink, NULL);
	if (!gst_element_link_many(src, parser, NULL)) {
		wxLogError(_T("Failed to chain objects, phase 1"));
		gst_object_unref(m_gstreamer_bin);
		goto error2;
	}

        if (!gst_element_link_many(decoder, conv, spectrum, sink, NULL)) {
		wxLogError(_T("Failed to chain objects, phase 2"));
		gst_object_unref(m_gstreamer_bin);
		goto error2;
	}

        g_signal_connect(parser, "pad-added", G_CALLBACK(gstreamer_new_pad), decoder);

        g_object_set(G_OBJECT(spectrum), "bands", m_spectrum_vals.size(), "message", TRUE, NULL);

        bus = gst_element_get_bus(m_gstreamer_bin);
        gst_bus_add_watch(bus, gstreamer_message_handler, this);
        gst_object_unref(bus);

	gst_element_set_state(m_gstreamer_bin, GST_STATE_PLAYING);

	GetSize(&w, &h);

	m_graphs_bitmap = new wxBitmap(w, h);

	dc.SelectObject(*m_graphs_bitmap);

	dc.Blit(0, 0, w, h, bg_view->GetDC(), 0, 0, wxCOPY, true);

	for (size_t i = 0; i < m_draws.GetCount(); ++i) {
		if (i == (size_t) m_selected_draw)
			continue;

		wxDC *pdc = m_graphs[i]->GetDC();
		if (pdc->Ok()) 
			dc.Blit(0, 0, w, h, pdc, 0, 0, wxCOPY, true);
	}
	dc.SelectObject(wxNullBitmap);

	m_dancing = true;

	return;

error:
	if (src != NULL)
		gst_object_unref(src);

	if (parser != NULL)
		gst_object_unref(parser);

	if (spectrum != NULL)
		gst_object_unref(spectrum);

	if (conv != NULL)
		gst_object_unref(conv);	
		
	if (decoder != NULL)
		gst_object_unref(decoder);

	if (sink != NULL)
		gst_object_unref(sink);
error2:
	wxMessageBox(_("Failed to start music playback, make sure that you have chosen proper file (ogg format) and that you have installed required gstreamer modules."
			" If you use Debian, you should have gstreamer-plugins-bad package on your system."),
			_("Error"),
			wxICON_ERROR);
	return;
}

std::vector<float>& DrawsWidget::GetSpectrumVals() {
	return m_spectrum_vals;
}

void DrawsWidget::Dance() {
	if (m_dancing)
		StopDance();
	else
		StartDance();
}

void DrawsWidget::DrawDance(wxDC *dc) {
	int w, h;	
	dc->GetSize(&w, &h);

	wxPen pp = dc->GetPen();
	wxPen p = pp;

	p.SetColour(m_draws[m_selected_draw]->GetDrawInfo()->GetDrawColor());
	dc->SetPen(p);

	w -= m_screen_margins.leftmargin + m_screen_margins.rightmargin;
	h -= m_screen_margins.topmargin + m_screen_margins.bottommargin;

#define GETPOS(INDEX, X, Y) 								\
	{										\
		X = m_screen_margins.leftmargin + INDEX * w / m_spectrum_vals.size();	\
		float v = m_spectrum_vals[INDEX];					\
		Y = m_screen_margins.topmargin - v * h / 60; 				\
	}

	dc->DrawBitmap(*m_graphs_bitmap, 0, 0, true);

	int prevx, prevy;
	GETPOS(0, prevx, prevy);

	for (size_t i = 1; i < m_spectrum_vals.size(); ++i) {
		int x, y;

		GETPOS(i, x, y);
		dc->DrawLine(prevx, prevy, x, y);

		prevx = x;
		prevy = y;
	}

	dc->SetPen(pp);	
}
#endif
#endif

wxDragResult DrawsWidget::SetSetInfo(wxString window,
		wxString prefix, 
		time_t time,
		PeriodType pt, 
		wxDragResult def) {

	return Switch(window, prefix, time, pt,-1) ? def : wxDragError;
}

bool DrawsWidget::Switch(wxString window,
		wxString prefix, 
		time_t time,
		PeriodType pt,
		int selected_draw) {

	DrawsSets* ds = m_cfg->GetConfigByPrefix(prefix);
	if (ds == NULL)
		return false;

	DrawSet *set = NULL;
	for (DrawSetsHash::iterator i = ds->GetDrawsSets().begin();
			i != ds->GetDrawsSets().end();
			i++)
		if (i->second->GetName() == window) {
			set = i->second;
			break;
		}
	if (set == NULL)
		return false;

	m_switch_date = time;
	m_switch_period = pt;
	m_switch_selected_draw = selected_draw;

	m_selset->SelectSet(ds->GetID(), set);
	m_timewdg->Select(pt, false);

	wxWindow *parent = m_parent;
	DrawFrame *frame = NULL;
	while (((frame = dynamic_cast<DrawFrame*>(parent)) == NULL) && parent != NULL)
		parent = parent->GetParent();
	assert(frame);
	frame->SetTitle(ds->GetID(), ds->GetPrefix());
	m_parent->SetConfigName(ds->GetID());
	m_graphs->Refresh();

	return true;

}

void DrawsWidget::CopyToClipboard() {
	if (m_selected_draw == -1)
		return;

	if (wxTheClipboard->Open() == false)
		return;
	
	Draw *d = m_draws[m_selected_draw];
	DrawInfo *di = d->GetDrawInfo();

	SetInfoDataObject* wido = 
		new SetInfoDataObject(di->GetBasePrefix(), di->GetSetName(), d->GetPeriod(), d->GetCurrentTime().GetTicks(), m_selected_draw);

	wxTheClipboard->SetData(wido);
	wxTheClipboard->Close();
}


void DrawsWidget::PasteFromClipboard() {
	if (wxTheClipboard->Open() == false)
		return;

	SetInfoDataObject wido(_T(""), _T(""), PERIOD_T_OTHER, -1, -1);
	if (wxTheClipboard->GetData(wido)) 
		Switch(wido.GetSet(), wido.GetPrefix(), wido.GetTime(), wido.GetPeriod(), wido.GetSelectedDraw());

	wxTheClipboard->Close();

}

wxString DrawsWidget::GetUrl(bool with_infinity) {
	if (m_selected_draw == -1)
		return wxEmptyString;
	Draw *d = m_draws[m_selected_draw];
	DrawInfo *di = d->GetDrawInfo();

	time_t t;

	if (with_infinity && d->IsTheNewestValue()) {
		t = std::numeric_limits<time_t>::max();
	} else {
		t = d->GetCurrentTime().GetTicks();
	}

	SetInfoDataObject* wido = 
		new SetInfoDataObject(di->GetBasePrefix(), di->GetSetName(), d->GetPeriod(), t , m_selected_draw);

	wxString tmp = wido->GetUrl();

	delete wido;

	return tmp;
}

void DrawsWidget::SetFocus() {
	m_graphs->SetFocus();
}

bool DrawsWidget::IsDrawEnabled(size_t idx) {
	if (idx >= m_draws.GetCount())
		return false;

	return m_draws[idx]->GetEnable();
}

void DrawsWidget::ShowRemarks(int idx) {
	wxDateTime fromtime = m_draws[m_selected_draw]->GetTimeOfIndex(idx);		
	wxDateTime totime = m_draws[m_selected_draw]->GetTimeOfIndex(idx + 1);

	m_remarks_fetcher->ShowRemarks(fromtime, totime);
}
