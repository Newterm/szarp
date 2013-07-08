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

#include <wx/intl.h>
#include <wx/bitmap.h>
#include <wx/log.h>
#include <wx/xrc/xmlres.h>

#include <math.h>

#include "classes.h"
#include "ids.h"
#include "cconv.h"
#include "coobs.h"
#include "dbinquirer.h"
#include "database.h"
#include "drawtime.h"
#include "drawobs.h"
#include "draw.h"
#include "drawsctrl.h"
#include "drawswdg.h"
#include "drawpnl.h"
#include "seldraw.h"
#include "datechooser.h"
#include "drawprint.h"
#include "drawurl.h"
#include "remarks.h"
#include "remarksfetcher.h"
#include "disptime.h"
#include "cfgmgr.h"
#include "drawdnd.h"

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

DrawsWidget::DrawsWidget(DrawPanel * parent, ConfigManager *cfg, DatabaseManager* dm, DrawGraphs* graphs, RemarksFetcher *rf) : 
	m_graphs(graphs), m_cfg(cfg), m_parent(parent), m_draws_controller(NULL), m_action(NONE), m_remarks_fetcher(rf)
{
	m_display_timer = new wxTimer(this, DISPLAY_TIMER_ID);
	m_keyboard_timer = new wxTimer(this, KEYBOARD_TIMER_ID);
	m_draws_controller = new DrawsController(cfg, dm);

	m_display_timer_events = 0;
	m_display_timer->Start(5000, wxTIMER_ONE_SHOT);

}

DrawsWidget::~DrawsWidget()
{
	delete m_draws_controller;
	delete m_display_timer;
	delete m_keyboard_timer;

}

void DrawsWidget::ClearCache()
{
	m_draws_controller->ClearCache();
}

wxDateTime DrawsWidget::GetCurrentTime()
{
	return m_draws_controller->GetCurrentTime();
}

DrawInfo *DrawsWidget::GetCurrentDrawInfo()
{
	return m_draws_controller->GetCurrentDrawInfo();
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

	m_keyboard_timer->Start(CursorMovementSpeed[m_draws_controller->GetPeriod()], wxTIMER_ONE_SHOT);
}

void DrawsWidget::OnDisplayTimerEvent(wxTimerEvent & event)
{
	wxDateTime now = wxDateTime::Now();
	m_display_timer->Start((5 - (now.GetSecond() % 5)) * 1000, wxTIMER_ONE_SHOT);

	m_display_timer_events += 1;
	if (m_draws_controller->GetPeriod() == PERIOD_T_30MINUTE) {
		if (m_display_timer_events > 1) {
			m_display_timer_events = 0;
			m_draws_controller->RefreshData(true);
		}
	} else {
		if (m_display_timer_events >= 10) {
			m_display_timer_events = 0;
			m_draws_controller->RefreshData(true);
		}
	}
}

size_t DrawsWidget::GetNumberOfUnits() {
	return m_draws_controller->GetNumberOfValues(m_draws_controller->GetPeriod()) / TimeIndex::PeriodMult[m_draws_controller->GetPeriod()];
}

void DrawsWidget::SetNumberOfUnits(size_t count) {
	m_draws_controller->SetNumberOfUnits(count);
}

void DrawsWidget::SetPeriod(PeriodType period)
{
	assert(period != PERIOD_T_OTHER);

	m_draws_controller->Set(period);
}

void DrawsWidget::SwitchCurrentDrawBlock() {
	Draw* draw = m_draws_controller->GetSelectedDraw();
	if (draw == NULL)
		return;

	BlockDraw(draw->GetDrawNo(), !draw->GetBlocked());
}

void DrawsWidget::BlockDraw(size_t index, bool block) {
	m_draws_controller->SetBlocked(index, block);
}

bool DrawsWidget::GetDrawBlocked(size_t index) {
	return m_draws_controller->GetDraw(index)->GetBlocked();
}

void DrawsWidget::SelectDraw(int idx, bool move_time, wxDateTime move_datetime)
{	
	if (move_time)
		m_draws_controller->Set(idx, move_datetime);
	else
		m_draws_controller->Set(idx);
}

void DrawsWidget::SelectNextDraw()
{
	m_draws_controller->SelectNextDraw();
}

void DrawsWidget::SelectPreviousDraw()
{
	m_draws_controller->SelectPreviousDraw();
}

size_t DrawsWidget::GetNumberOfValues() {
	return m_draws_controller->GetNumberOfValues(m_draws_controller->GetPeriod());
}

void DrawsWidget::MoveCursorLeft(int n)
{
	m_draws_controller->MoveCursorLeft(n);
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
	m_draws_controller->MoveCursorRight(n);
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

bool DrawsWidget::SetDrawDisable(size_t index)
{
	return m_draws_controller->SetDrawEnabled(index, false);
}

void DrawsWidget::SetDrawEnable(size_t index)
{
	m_draws_controller->SetDrawEnabled(index, true);
}

void DrawsWidget::MoveScreenRight()
{
	m_draws_controller->MoveScreenRight();
}

void DrawsWidget::MoveScreenLeft()
{
	m_draws_controller->MoveScreenLeft();
}

void DrawsWidget::MoveCursorBegin()
{
	m_draws_controller->MoveCursorBegin();
}

void DrawsWidget::MoveCursorEnd()
{
	m_draws_controller->MoveCursorEnd();
}

Draw* DrawsWidget::GetSelectedDraw() {
	return m_draws_controller->GetSelectedDraw();
}

int DrawsWidget::GetSelectedDrawIndex() {
	return m_draws_controller->GetSelectedDraw()->GetDrawNo();
}

void DrawsWidget::OnJumpToDate() {

	wxDateTime date = m_draws_controller->GetCurrentTime();

	if (date.IsValid() == false)
		return;

	DateChooserWidget *dcw = 
		new DateChooserWidget(
				m_parent,
				_("Select date"),
				1,
				wxDateTime(1, 1, 1990).GetTicks(),
				wxDateTime::Now().GetTicks(),
				10
		);
	
	bool ret = dcw->GetDate(date);
	delete dcw;

	if (ret == false)
		return;

	m_draws_controller->Set(date);

}

bool DrawsWidget::DoubleCursorSet(bool enable) {
	if (enable) {
		if (m_draws_controller->GetDoubleCursor())
			return true;
		if (m_draws_controller->SetDoubleCursor(true) == false)
			return false;
		return true;
	} else {
		if (m_draws_controller->GetDoubleCursor())
			m_draws_controller->SetDoubleCursor(false);
		return  false;
	}

}

bool DrawsWidget::ToggleSplitCursor() {
	bool is_double = m_draws_controller->GetDoubleCursor();

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

bool DrawsWidget::GetDoubleCursor() { 
	Draw* draw = GetSelectedDraw();

	if (draw == NULL)
		return false;

	return draw->GetDoubleCursor();
}

void DrawsWidget::SetSet(DrawSet *set) {
	m_draws_controller->Set(set);
}

void DrawsWidget::SetSet(DrawSet *set, DrawInfo *di) {
	DrawInfoArray *draws = set->GetDraws();

	size_t i;
	for (i = 0; i < draws->size(); i++)
		if ((*draws)[i] == di)
			break;

	if (i == draws->size())
		return;

	m_draws_controller->Set(set, i);
}

void DrawsWidget::Print(bool preview) {
	std::vector<Draw*> draws;
	for (size_t i = 0; i < m_draws_controller->GetDrawsCount(); i++)
		draws.push_back(m_draws_controller->GetDraw(i));
			
	if (preview)
		Print::DoPrintPreviev(draws, draws.size());
	else
		Print::DoPrint(m_parent, draws, draws.size());
}
		
size_t DrawsWidget::GetDrawsCount() {
	return m_draws_controller->GetDrawsCount();
}

void DrawsWidget::SetFilter(int f) {
	m_draws_controller->SetFilter(f);
}

DrawInfo* DrawsWidget::GetDrawInfo(int index) {
	return m_draws_controller->GetDraw(index)->GetDrawInfo();
}

bool DrawsWidget::GetNoData() {
	return m_draws_controller->GetNoData();
}

void DrawsWidget::RefreshData(bool auto_refresh)
{
	m_draws_controller->RefreshData(auto_refresh);
	if (!auto_refresh)
		m_display_timer_events = 0;
}

wxDragResult DrawsWidget::SetSetInfo(wxString window,
		wxString prefix, 
		time_t time,
		PeriodType pt, 
		wxDragResult def) {

	return SetSet(window, prefix, time, pt, 0) ? def : wxDragError;
}

bool DrawsWidget::SetSet(wxString sset,
		wxString prefix, 
		time_t time,
		PeriodType pt,
		int selected_draw) {

	DrawsSets* ds = m_cfg->GetConfigByPrefix(prefix);
	if (ds == NULL)
		return false;

	DrawSet *set = NULL;

	if (!sset.IsEmpty()) {
			for (DrawSetsHash::iterator i = ds->GetDrawsSets().begin();
					i != ds->GetDrawsSets().end();
					i++)
				if (i->second->GetName() == sset) {
					set = i->second;
					break;
				}
	}
	if (set == NULL)
		if ((set = m_draws_controller->GetSet()) == NULL) {
			SortedSetsArray ssa = ds->GetSortedDrawSetsNames();
			set = ssa[0];
		}

	if (set == NULL)
		return false;

	if (time <= 0)
		time = wxDateTime::Now().GetTicks();

	if (pt == PERIOD_T_OTHER)
		pt = m_draws_controller->GetPeriod();
	if (pt == PERIOD_T_OTHER)
		pt = PERIOD_T_YEAR;

	if (selected_draw < 0 || selected_draw >= (int)set->GetDraws()->size()) {
		if (m_draws_controller->GetSelectedDraw())
			selected_draw = m_draws_controller->GetSelectedDraw()->GetDrawNo();
		else
			selected_draw = 0;
	}

	m_draws_controller->Set(set, pt, time, selected_draw);

	return true;

}

void DrawsWidget::CopyToClipboard() {
	Draw *d = m_draws_controller->GetSelectedDraw();

	if (d == NULL)
		return;

	if (wxTheClipboard->Open() == false)
		return;
	
	DrawInfo *di = d->GetDrawInfo();

	SetInfoDataObject* wido = 
		new SetInfoDataObject(di->GetBasePrefix(), di->GetSetName(), d->GetPeriod(), d->GetCurrentTime().GetTicks(), d->GetDrawNo());

	wxTheClipboard->SetData(wido);
	wxTheClipboard->Close();
}


void DrawsWidget::PasteFromClipboard() {
	if (wxTheClipboard->Open() == false)
		return;

	SetInfoDataObject wido(_T(""), _T(""), PERIOD_T_OTHER, -1, -1);
	if (wxTheClipboard->GetData(wido)) 
		SetSet(wido.GetSet(), wido.GetPrefix(), wido.GetTime(), wido.GetPeriod(), wido.GetSelectedDraw());

	wxTheClipboard->Close();

}


void DrawsWidget::OnCopyParamName(wxCommandEvent &event) {
	Draw *d = m_draws_controller->GetSelectedDraw();

	if (d == NULL)
		return;

	CopyParamName(d->GetDrawInfo());
}

void DrawsWidget::CopyParamName(DrawInfo *di) {
	if (di == NULL)
		return;

	if (wxTheClipboard->Open() == false)
		return;

	wxTheClipboard->SetData(new wxTextDataObject(di->GetParamName()));
	wxTheClipboard->Close();
}


wxString DrawsWidget::GetUrl(bool with_infinity) {
	Draw* d = m_draws_controller->GetSelectedDraw();
	if (d == NULL)
		return wxEmptyString;

	time_t t;

	if (with_infinity && m_draws_controller->AtTheNewestValue()) {
		t = std::numeric_limits<time_t>::max();
	} else {
		t = m_draws_controller->GetCurrentTime();
	}

	wxString prefix = m_draws_controller->GetSet()->GetDrawsSets()->GetPrefix();

	DrawInfo* di = d->GetDrawInfo();

	SetInfoDataObject* wido = 
		new SetInfoDataObject(prefix, di->GetSetName(), d->GetPeriod(), t , d->GetDrawNo());

	wxString tmp = wido->GetUrl();

	delete wido;

	return tmp;
}

DrawSet* DrawsWidget::GetCurrentDrawSet() {
	return m_draws_controller->GetSet();
}

void DrawsWidget::SetFocus() {
	m_graphs->SetFocus();
}

bool DrawsWidget::IsDrawEnabled(size_t idx) {
	return m_draws_controller->GetDrawEnabled(idx);
}

void DrawsWidget::ShowRemarks(int idx) {
	Draw* d = m_draws_controller->GetSelectedDraw();
	if (d == NULL)
		return;

	wxDateTime fromtime = d->GetTimeOfIndex(idx).GetTime();
	wxDateTime totime = d->GetTimeOfIndex(idx + 1).GetTime();

	m_remarks_fetcher->ShowRemarks(fromtime, totime);
}

void DrawsWidget::AttachObserver(DrawObserver *draw_observer) {
	draw_observer->Attach(m_draws_controller);
}

void DrawsWidget::DetachObserver(DrawObserver *draw_observer) {
	draw_observer->Detach(m_draws_controller);
}

DrawsController* DrawsWidget::GetDrawsController() {
	return m_draws_controller;
}
