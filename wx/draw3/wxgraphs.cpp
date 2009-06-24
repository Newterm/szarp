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

 *
 * $Id$
 */

#include <wx/dcbuffer.h>
#include <wx/clipbrd.h>

#include "ids.h"
#include "drawdnd.h"
#include "drawview.h"
#include "draw.h"
#include "wxgraphs.h"

BEGIN_EVENT_TABLE(WxGraphs, wxWindow)
    EVT_PAINT(WxGraphs::OnPaint)
    EVT_LEFT_DOWN(WxGraphs::OnMouseLeftDown)
    EVT_LEFT_DCLICK(WxGraphs::OnMouseLeftDClick)
    EVT_LEAVE_WINDOW(WxGraphs::OnMouseLeftUp)
    EVT_LEFT_UP(WxGraphs::OnMouseLeftUp)
    EVT_IDLE(WxGraphs::OnIdle)
    EVT_SIZE(WxGraphs::OnSize)
    EVT_SET_FOCUS(WxGraphs::OnSetFocus)
    EVT_ERASE_BACKGROUND(WxGraphs::OnEraseBackground)
END_EVENT_TABLE()

WxGraphs::WxGraphs(wxWindow *parent, ConfigManager *cfg) : wxWindow(parent, wxID_ANY) {
	SetHelpText(_T("draw3-base-win"));

	wxFont f = GetFont();
#ifdef __WXGTK__
	f.SetPointSize(f.GetPointSize() - 2);
#endif
	SetFont(f);

#ifndef NO_GSTREAMER
	m_spectrum_vals.resize(24);

	m_dancing = false;
#endif
	m_bg_view = new BackgroundView(this, cfg);

	SetBackgroundStyle(wxBG_STYLE_CUSTOM);

	SetInfoDropTarget* dt = new SetInfoDropTarget(this);
	SetDropTarget(dt);

	m_screen_margins.leftmargin = 36;
	m_screen_margins.rightmargin = 10;
	m_screen_margins.topmargin = 24;
	m_screen_margins.bottommargin = 12;
	m_screen_margins.infotopmargin = 7;

	m_draw_current_draw_name = false;
	
	m_cfg_mgr = cfg;

	/* Set minimal size of widget. */
	SetSizeHints(300, 200);

}

void WxGraphs::OnIdle(wxIdleEvent & event)
{
	if (m_invalid_region.IsEmpty())
		return;

	wxRect rect = m_invalid_region.GetBox();
	wxLogInfo(_T("Idle triggers refresh of rectange %d %d, %d %d"),
		  rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight());

	RefreshRect(rect);
	m_invalid_region.Clear();
}

void WxGraphs::OnEraseBackground(wxEraseEvent & WXUNUSED(event))
{ 
}


void WxGraphs::OnPaint(wxPaintEvent & WXUNUSED(event))
{
	wxBufferedPaintDC dc(this);
	dc.SetMapMode(wxMM_TEXT);

	dc.SetBackground(*wxBLACK);

	dc.SetFont(GetFont());

	int selected_draw = m_draws_wdg->GetSelectedDrawIndex();
	if (selected_draw == -1) {
		dc.Clear();
		return;
	}

	wxRegion region = GetUpdateRegion();

	wxRegionIterator i(region);

	wxDC *pdc = m_bg_view->GetDC();

	if (m_draws_wdg->IsNoData() == false) {
#ifndef NO_GSTREAMER
		if (!m_dancing) 
#endif
			while (i) {

				int x = i.GetX();
				int y = i.GetY();
 				int w = i.GetW();
				int h = i.GetH();

				wxLogInfo(_T("Repainting - x:%d y:%d, w:%d h:%d"), x, y, w, h);

				if (pdc->Ok())
					dc.Blit(x, y, w, h, pdc, x, y, wxCOPY);

				for (size_t j = 0; j <= m_draws.GetCount(); ++j) {
					size_t i;
	
					if ((int)j == selected_draw)
						continue;

					if (j == m_draws.GetCount())
						i = selected_draw;
					else
						i = j;

					Draw *d = m_draws[i];

					if (d->GetEnable() == false)
						continue;

					wxDC *pdc = m_graphs[i]->GetDC();
					if (pdc->Ok()) 
						dc.Blit(x, y, w, h, pdc, x, y, wxCOPY, true);
		
				}

				++i;
			}
#ifndef NO_GSTREAMER
		else	
			DrawDance(&dc);
#endif
	} else {
		int w, h, tw, th;
		GetClientSize(&w, &h);

		if (pdc->Ok())
			dc.Blit(0, 0, w, h, pdc, 0, 0, wxCOPY);

		wxString no_data = _("No data");

		dc.SetFont(GetFont());

		dc.SetTextForeground(*wxWHITE);

		dc.GetTextExtent(no_data, &tw, &th);

		dc.DrawText(no_data, (w - tw) / 2, (h - th) / 2);
	}

	DrawWindowInfo(&dc, region);

	if (m_draw_current_draw_name)
		DrawCurrentParamName(&dc);

}

void WxGraphs::DrawCurrentParamName(wxDC *dc) {
	DrawInfo *di = m_draws_wdg->GetCurrentDrawInfo();
	if (di == NULL)
		return;

	wxFont f = GetFont();

	int ps = f.GetPointSize();
	int fw = f.GetWeight();
	f.SetWeight(wxFONTWEIGHT_BOLD);
	f.SetPointSize(ps * 1.25);
	dc->SetFont(f);

	wxString text = m_cfg_mgr->GetConfigTitles()[di->GetBasePrefix()] + _T(":") + di->GetParamName();

	int tw, th;
	dc->GetTextExtent(text, &tw, &th);

	int w, h;
	GetSize(&w, &h);
	dc->SetTextForeground(di->GetDrawColor());
	dc->SetBrush(*wxBLACK_BRUSH);
	dc->SetPen(*wxWHITE_PEN);
	dc->DrawRectangle(w / 2 - tw / 2 - 1, h / 2 - th / 2 - 1, tw + 2, th + 2);
	dc->DrawText(text, w / 2 - tw / 2, h / 2 - th / 2);

	f.SetPointSize(ps);
	f.SetWeight(fw);
	dc->SetFont(f);

}

void WxGraphs::Refresh() {
	m_invalid_region.Union(m_size);
}

void WxGraphs::SetDrawsChanged(DrawPtrArray draws) {
	size_t pc = m_graphs.GetCount();

	if (pc > draws.GetCount()) {
		for (size_t i = draws.GetCount(); i < pc; i++)
			delete m_graphs[i];
		m_graphs.RemoveAt(draws.GetCount(), pc - draws.GetCount());
	}
	m_graphs.SetCount(draws.GetCount());

	int w, h;
	GetClientSize(&w, &h);
	for (size_t i = pc; i < draws.GetCount(); i++) {
		m_graphs[i] = new GraphView(this);
		m_graphs[i]->Attach(draws[i]);
		m_graphs[i]->SetSize(w, h);
		m_graphs[i]->DrawAll();
	}

	m_draws = draws;

	SetMargins();

	Refresh();
}

void WxGraphs::FullRefresh() {
	for (size_t i = 0; i < m_draws.GetCount(); i++)
		m_graphs[i]->DrawAll();

	Refresh();
}

void WxGraphs::DrawWindowInfo(wxDC * dc, const wxRegion & repainted_region)
{

	if (repainted_region.IsEmpty())
		return;

	int info_left_marg = m_screen_margins.leftmargin + 8;
	int param_name_shift = 5;

	if (m_draws.GetCount() < 1)
		return;

	int w, h;
	dc->GetSize(&w, &h);

	DrawInfo *info = m_draws[0]->GetDrawInfo();
	wxString name = info->GetSetName().c_str();

	int namew, nameh;
	dc->GetTextExtent(name, &namew, &nameh);

	if (repainted_region.Contains(info_left_marg, m_screen_margins.infotopmargin, w - m_screen_margins.infotopmargin, nameh) == wxOutRegion)
		return;

	dc->SetTextForeground(*wxWHITE);

	dc->DrawText(name, info_left_marg, m_screen_margins.infotopmargin);

	wxColor color = dc->GetTextForeground();

	int xpos = info_left_marg + namew + param_name_shift;

	for (int i = 0; i < (int)m_draws.GetCount(); ++i) {

		if (!m_draws[i]->GetEnable())
			continue;

		DrawInfo *info = m_draws[i]->GetDrawInfo();

		dc->SetTextForeground(info->GetDrawColor());

		name = info->GetShortName().c_str();
		dc->GetTextExtent(name, &namew, &nameh);

		dc->DrawText(name, xpos, m_screen_margins.infotopmargin);
		xpos += namew + param_name_shift;
	}

	dc->SetTextForeground(color);

}

void WxGraphs::Deselected(int i) {
	m_bg_view->Detach(m_draws[i]);
}

void WxGraphs::Selected(int i) {
	m_bg_view->Attach(m_draws[i]);
	Refresh();
}

void WxGraphs::OnSize(wxSizeEvent & WXUNUSED(event))
{
	int w, h;
	GetClientSize(&w, &h);
	if (m_size.GetWidth() == w && m_size.GetHeight() == h)
		return;

	m_size = wxSize(w, h);

	wxLogVerbose(_T("Resizing to %d:%d"), w, h);
	for (size_t i = 0; i < m_draws.GetCount(); ++i) {
		m_graphs[i]->SetSize(w, h);
		m_graphs[i]->DrawAll();
	}

	if (m_draws.GetCount() > 0)
		m_bg_view->SetSize(w, h);

	RefreshRect(wxRect(0, 0, w, h));
}

void WxGraphs::SetMargins() {
	int leftmargin = 36;
	int bottommargin = 12;
	int topmargin = 24;

	wxClientDC dc(this);
	dc.SetFont(GetFont());
	for (size_t i = 0; i < m_graphs.GetCount(); i++) {
		int tw, th;
		DrawInfo *di = m_draws[i]->GetDrawInfo();
		wxString sval = di->GetValueStr(di->GetMax(), _T(""));

		dc.GetTextExtent(sval, &tw, &th);
		if (leftmargin < tw + 1)
			leftmargin = tw + 1;
		if (bottommargin < th + 1)
			bottommargin = th + 1;

		if (topmargin < th + m_screen_margins.infotopmargin)
			topmargin = th + m_screen_margins.infotopmargin;

	}

	m_screen_margins.leftmargin = leftmargin;
	m_screen_margins.bottommargin = bottommargin;
	m_screen_margins.topmargin = topmargin;

	m_bg_view->SetMargins(m_screen_margins.leftmargin, m_screen_margins.rightmargin, m_screen_margins.topmargin, m_screen_margins.bottommargin);
	for (size_t i = 0; i < m_graphs.GetCount(); i++)
		m_graphs.Item(i)->SetMargins(m_screen_margins.leftmargin, m_screen_margins.rightmargin, m_screen_margins.topmargin, m_screen_margins.bottommargin);
}

void WxGraphs::OnMouseLeftDown(wxMouseEvent & event)
{

	/* get widget size */
	int w, h;
	GetClientSize(&w, &h);

	/* check for 'move cursor left' event */
	if (event.GetX() < m_screen_margins.leftmargin) {
		m_draws_wdg->MoveCursorLeft();
		return;
	}

	/* check for 'move cursor right' event */
	if (event.GetX() > (w - m_screen_margins.rightmargin)) {
		m_draws_wdg->MoveCursorRight();
		return;
	}

	int index = m_draws_wdg->GetSelectedDrawIndex();
	if (index < 0)
		return;

	wxDateTime ct = m_draws_wdg->GetSelectedDraw()->GetCurrentTime();
	if (!ct.IsValid())
		return;

	struct VoteInfo {
		double dist;
		wxDateTime time;
	};

	VoteInfo infos[m_graphs.GetCount()];

	int x = event.GetX();
	int y = event.GetY();

	for (size_t i = 0; i < m_graphs.GetCount(); ++i) {

		VoteInfo & in = infos[i];
		in.dist = -1;

		if (m_draws[i]->GetEnable())
			m_graphs[i]->GetDistance(x, y, in.dist, in.time);
	}

	double min = INFINITY;
	int j = -1;

	int selected_draw = m_draws_wdg->GetSelectedDrawIndex();

	for (size_t i = 1; i <= m_draws.GetCount(); ++i) {
		size_t k = (i + selected_draw) % m_draws.GetCount();

		VoteInfo & in = infos[k];
		if (in.dist < 0)
			continue;

		if (in.dist < GraphView::cursorrectanglesize * GraphView::cursorrectanglesize
				&& in.time == ct) {
			j = k;
			break;
		}

		if ((int)k == selected_draw && in.dist == 0) {
			j = k;
			break;
		}

		if (min > in.dist) {
			j = k;
			min = in.dist;
		}
	}

	if (j == -1) 
		return;

	if (selected_draw != j && infos[j].dist < 1000) {
		m_draws_wdg->SelectDraw(j, true, infos[j].time);
	} else
		m_draws_wdg->SelectDraw(selected_draw, true, infos[selected_draw].time);

}

void WxGraphs::OnMouseLeftUp(wxMouseEvent & event)
{
	m_draws_wdg->StopMovingCursor();
}

void WxGraphs::UpdateArea(const wxRegion & region)
{
	if (region.IsEmpty())
		return;
	m_invalid_region.Union(region);
}

void WxGraphs::OnMouseLeftDClick(wxMouseEvent & event)
{
	if (m_draws.GetCount() < 0)
		return;

	if (m_draws_wdg->IsNoData())
		return;

	/* get widget size */
	int w, h;
	GetClientSize(&w, &h);

	/* check for 'move screen left' event */
	if (event.GetX() < m_screen_margins.leftmargin) {
		m_draws_wdg->MoveScreenLeft();
		return;
	}

	/* check for 'move screen right' event */
	if (event.GetX() > w - m_screen_margins.rightmargin) {
		m_draws_wdg->MoveScreenRight();
		return;
	}
}

#ifndef NO_GSTREAMER
void WxGraphs::DrawDance(wxDC* dc) {
}
#endif

void WxGraphs::OnSetFocus(wxFocusEvent& event) {
}

void WxGraphs::OnMouseRightDown(wxMouseEvent &event) {
	if (m_draws_wdg->GetSelectedDrawIndex() == -1)
		return;

	Draw *d = m_draws[m_draws_wdg->GetSelectedDrawIndex()];
	DrawInfo *di = d->GetDrawInfo();

	SetInfoDataObject wido(di->GetBasePrefix(), di->GetSetName(), d->GetPeriod(), d->GetCurrentTime().GetTicks(), m_draws_wdg->GetSelectedDrawIndex());
	wxDropSource ds(wido, this);
	ds.DoDragDrop(0);
}

wxDragResult WxGraphs::SetSetInfo(wxCoord x, 
		wxCoord y, 
		wxString window,
		wxString prefix, 
		time_t time,
		PeriodType pt, 
		wxDragResult def) {
	return m_draws_wdg->SetSetInfo(window, prefix, time, pt, def);
}

void WxGraphs::SetFocus() {
	wxWindow::SetFocus();
}

void WxGraphs::StartDrawingParamName() { 
	m_draw_current_draw_name = true;
	Refresh();
}

void WxGraphs::StopDrawingParamName() {
	m_draw_current_draw_name = false;
	Refresh();
}


WxGraphs::~WxGraphs() {
	for (size_t i = 0; i < m_graphs.GetCount(); ++i) 
		delete m_graphs[i];

	delete m_bg_view;
}

