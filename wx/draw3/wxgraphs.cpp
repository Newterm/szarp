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
#include "classes.h"
#include "drawdnd.h"
#include "drawview.h"
#include "dbinquirer.h"
#include "drawtime.h"
#include "database.h"
#include "coobs.h"
#include "drawsctrl.h"
#include "cfgmgr.h"
#include "dbmgr.h"
#include "drawobs.h"
#include "drawswdg.h"
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

void WxGraphs::ResetDraws(DrawsController *controller) {
	m_draws.resize(0);
	for (size_t i = 0; i < controller->GetDrawsCount(); i++)
		m_draws.push_back(controller->GetDraw(i));

	size_t pc = m_graphs.size();
	if (pc > m_draws.size()) for (size_t i = m_draws.size(); i < pc; i++)
			delete m_graphs.at(i);
	m_graphs.resize(m_draws.size());

	for (size_t i = pc; i < m_draws.size(); i++) {
		m_graphs.at(i) = new GraphView(this);
		m_graphs.at(i)->Attach(m_draws[i]);
		m_graphs.at(i)->DrawAll();
	}

	Draw* d = controller->GetSelectedDraw();
	if (d)
		m_bg_view->Attach(m_draws[d->GetDrawNo()]);

	for (size_t i = 0; i < std::min(pc, m_graphs.size()); i++)
		m_graphs.at(i)->DrawInfoChanged(m_draws[i]);


	SetMargins();

	Refresh();

}

	/**Redraws view*/
void WxGraphs::DrawInfoChanged(Draw *draw) {
	if (draw->GetSelected())
		ResetDraws(draw->GetDrawsController());
}

void WxGraphs::DrawsSorted(DrawsController *controller) {
	ResetDraws(controller);
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

	if (m_draws_wdg->GetNoData() == false) {
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

				for (size_t j = 0; j <= m_draws.size(); ++j) {
					size_t i;
	
					if ((int)j == selected_draw)
						continue;

					if (j == m_draws.size())
						i = selected_draw;
					else
						i = j;

					Draw *d = m_draws[i];

					if (d->GetEnable() == false)
						continue;

					wxDC *pdc = m_graphs.at(i)->GetDC();
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

void WxGraphs::FullRefresh() {
	for (size_t i = 0; i < m_draws.size(); i++)
		m_graphs.at(i)->DrawAll();

	Refresh();
}

void WxGraphs::DrawWindowInfo(wxDC * dc, const wxRegion & repainted_region)
{

	if (repainted_region.IsEmpty())
		return;

	int info_left_marg = m_screen_margins.leftmargin + 8;
	int param_name_shift = 5;

	if (m_draws.size() < 1)
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

	for (int i = 0; i < (int)m_draws.size(); ++i) {

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

void WxGraphs::DrawDeselected(Draw *d) {
	m_bg_view->Detach(m_draws.at(d->GetDrawNo()));
	m_graphs.at(d->GetDrawNo())->DrawAll();
}

void WxGraphs::DrawSelected(Draw *d) {
	m_bg_view->Attach(m_draws[d->GetDrawNo()]);
	m_graphs.at(d->GetDrawNo())->DrawAll();
	Refresh();
}

void WxGraphs::FilterChanged(DrawsController *draws_controller) {
	for (size_t i = 0; i < m_graphs.size(); i++)
		m_graphs.at(i)->FilterChanged(m_draws[i]);
	Refresh();
}

void WxGraphs::EnableChanged(Draw *draw) {
	Refresh();
}

/**Repaints view*/
void WxGraphs::PeriodChanged(Draw *draw, PeriodType period) {
	if (draw->GetSelected())
		m_bg_view->PeriodChanged(draw, period);
	m_graphs.at(draw->GetDrawNo())->PeriodChanged(draw, period);
	Refresh();
}

/**Repaints view*/
void WxGraphs::ScreenMoved(Draw* draw, const wxDateTime &start_date) {
	size_t i = draw->GetDrawNo();
	if (i >= m_draws.size())
		return;

	if (draw->GetSelected())
		m_bg_view->ScreenMoved(draw, start_date);

	m_graphs.at(draw->GetDrawNo())->ScreenMoved(draw, start_date);
}

void WxGraphs::NewData(Draw* draw, int i) {
	m_graphs.at(draw->GetDrawNo())->NewData(draw, i);
}

void WxGraphs::CurrentProbeChanged(Draw* draw, int pi, int ni, int d) {
	m_graphs.at(draw->GetDrawNo())->CurrentProbeChanged(draw, pi, ni, d);
}

void WxGraphs::NewRemarks(Draw *draw) {
	m_bg_view->NewRemarks(draw);
}

void WxGraphs::OnSize(wxSizeEvent & WXUNUSED(event))
{
	int w, h;
	GetClientSize(&w, &h);
	if (m_size.GetWidth() == w && m_size.GetHeight() == h)
		return;

	m_size = wxSize(w, h);

	wxLogVerbose(_T("Resizing to %d:%d"), w, h);
	for (size_t i = 0; i < m_draws.size(); ++i) {
		m_graphs.at(i)->SetSize(w, h);
		m_graphs.at(i)->DrawAll();
	}

	if (m_draws.size() > 0)
		m_bg_view->SetSize(w, h);

	RefreshRect(wxRect(0, 0, w, h));
}

void WxGraphs::SetMargins() {
	int leftmargin = 36;
	int bottommargin = 12;
	int topmargin = 24;

	wxClientDC dc(this);
	dc.SetFont(GetFont());
	for (size_t i = 0; i < m_graphs.size(); i++) {
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
	for (size_t i = 0; i < m_graphs.size(); i++)
		m_graphs.at(i)->SetMargins(m_screen_margins.leftmargin, m_screen_margins.rightmargin, m_screen_margins.topmargin, m_screen_margins.bottommargin);
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

	VoteInfo infos[m_graphs.size()];

	int x = event.GetX();
	int y = event.GetY();

	int remark_idx = m_bg_view->GetRemarkClickedIndex(x, y);
	if (remark_idx != -1) {
		m_draws_wdg->ShowRemarks(remark_idx);
		return;
	}

	for (size_t i = 0; i < m_graphs.size(); ++i) {

		VoteInfo & in = infos[i];
		in.dist = -1;

		if (m_draws[i]->GetEnable())
			m_graphs.at(i)->GetDistance(x, y, in.dist, in.time);
	}

	double min = INFINITY;
	int j = -1;

	int selected_draw = m_draws_wdg->GetSelectedDrawIndex();

	for (size_t i = 1; i <= m_draws.size(); ++i) {
		size_t k = (i + selected_draw) % m_draws.size();

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

void WxGraphs::DoubleCursorChanged(DrawsController *draws_controller) {
	m_graphs[draws_controller->GetSelectedDraw()->GetDrawNo()]->DrawAll();
	Refresh();
}

void WxGraphs::NumberOfValuesChanged(DrawsController *draws_controller) {
	m_bg_view->DoDraw(m_bg_view->GetDC());
	for (size_t i = 0; i < m_draws.size(); i++)
		m_graphs.at(i)->DrawAll();

}

void WxGraphs::OnMouseLeftDClick(wxMouseEvent & event)
{
	if (m_draws.size() < 0)
		return;

	if (m_draws_wdg->GetNoData())
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
	for (size_t i = 0; i < m_graphs.size(); ++i) 
		delete m_graphs.at(i);

	delete m_bg_view;
}

