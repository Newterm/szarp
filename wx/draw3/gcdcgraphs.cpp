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
 * $Id: wxgraphs.h 1 2009-06-24 15:09:25Z isl $
 */

#ifdef __WXGTK__

#include <wx/dcbuffer.h>

#include "bitmaps/remark_flag.xpm"
#include "gcdcgraphs.h"
#include "draw.h"
#include "graphsutils.h"

/**Significant part of this code is just copied from classic graphs implementation, it's bad practice, I know, but I just wanted
 * to make it work fast. 3.0 version of wxWidgets promises to provide wxGCDC that would have
 * the same API as wxDC, so we won't need most of this code anyway.*/

GCDCGraphs::GCDCGraphs(wxWindow* parent, ConfigManager *cfg) : wxWindow(parent, wxID_ANY), m_draw_param_name(false), m_cfg_mgr(cfg), m_recalulate_margins(true) {
	m_screen_margins.leftmargin = 10;
	m_screen_margins.rightmargin = 10;
	m_screen_margins.topmargin = 10;
	m_screen_margins.bottommargin = 12;
	m_screen_margins.infotopmargin = 7;
	m_refresh = true;

	m_remark_flag_bitmap = wxBitmap(remark_flag_xpm);

}

double GCDCGraphs::GetX(int i) {
	int w, h;
	GetClientSize(&w, &h);

	size_t size = m_draws_wdg->GetSelectedDraw()->GetValuesTable().size();

	double res = ((double) w - m_screen_margins.leftmargin - m_screen_margins.rightmargin) / size;

	double ret = i * res + m_screen_margins.leftmargin + res / 2;

	return ret;
}

double GCDCGraphs::GetY(double value, DrawInfo *di) {
	int w, h;
	GetClientSize(&w, &h);

	h -= m_screen_margins.bottommargin + m_screen_margins.topmargin;

	double max = di->GetMax();
	double min = di->GetMin();

	double dmax = max - min;
	double dmin = 0;

	double smin = 0;
	double smax = 0;
	double k = 0;

	int sc = di->GetScale();
	if (sc) {
		smin = di->GetScaleMin() - min;
		smax = di->GetScaleMax() - min;
		assert(smax > smin);
		k = sc / 100. * (dmax - dmin) / (smax - smin);
		dmax += (k - 1) * (smax - smin);
	}

	double dif = dmax - dmin;
	if (dif <= 0) {
		wxLogInfo(_T("%s %f %f"), di->GetName().c_str(), min, max);
		assert(false);
	}

	double dvalue = value - min;

	double scaled = 
		wxMax(dvalue - smax, 0) +
		wxMax(wxMin(dvalue - smin, smax - smin), 0) * k +
		wxMax(wxMin(dvalue - dmin, smin), 0);

	double ret = h - scaled * h / dif + m_screen_margins.topmargin;

	if (ret < m_screen_margins.topmargin)
		ret = m_screen_margins.topmargin;

	if (ret > h + m_screen_margins.topmargin) 
		ret = h + m_screen_margins.topmargin;
		
	return ret;
}


const wxColour GCDCGraphs::back1_col = wxColour(128, 128, 128);
const wxColour GCDCGraphs::back2_col = wxColour(0, 0, 0);

void GCDCGraphs::DrawBackground(wxGraphicsContext &dc) {
	int w, h;
	GetClientSize(&w, &h);

	PeriodType pt = m_draws_wdg->GetSelectedDraw()->GetPeriod();
	size_t pc = m_draws_wdg->GetSelectedDraw()->GetValuesTable().size();

	dc.SetBrush(wxBrush(back2_col, wxSOLID));
	dc.DrawRectangle(0, 0, w, h);

	size_t i = 0;
	int c = 1;
	double x = m_screen_margins.leftmargin + 1;

	while (i < pc) {
		double x1 = GetX(i);

		i += Draw::PeriodMult[pt];
	
		double x2 = GetX(i);

		if (c > 0)
			dc.SetBrush(wxBrush(back1_col, wxSOLID));
		else
			dc.SetBrush(wxBrush(back2_col, wxSOLID));
	
		c *= -1; 

		double xn; 
		if (i < pc)
			xn = x + x2 - x1;
		else
			xn = w - m_screen_margins.rightmargin;

		dc.DrawRectangle(x, m_screen_margins.topmargin, x2 - x1, h - m_screen_margins.topmargin + 1);

		x = x + x2 - x1;
	}

}

wxDragResult GCDCGraphs::SetSetInfo(wxCoord x, wxCoord y, wxString window, wxString prefix, time_t time, PeriodType pt, wxDragResult def) {
	return m_draws_wdg->SetSetInfo(window, prefix, time, pt, def);
}

void GCDCGraphs::SetDrawsChanged(DrawPtrArray draws) {
	size_t pc = m_draws.GetCount();
	m_draws = draws;

	for (size_t i = pc; i < draws.GetCount(); i++)
		m_draws[i]->AttachObserver(this);

	m_recalulate_margins = true;
	Refresh();
}


void GCDCGraphs::StartDrawingParamName() {
	if (!m_draw_param_name) {
		m_draw_param_name = true;
		Refresh();
	}

}

void GCDCGraphs::StopDrawingParamName() {
	if (m_draw_param_name) {
		m_draw_param_name = false;
		Refresh();
	}
}

void GCDCGraphs::Selected(int i) {
	Refresh();
}

void GCDCGraphs::Deselected(int i) {
	Refresh();
}

void GCDCGraphs::FullRefresh() {
	Refresh();
}

void GCDCGraphs::NewRemarks(Draw *d) {
	Refresh();
}
	
void GCDCGraphs::SetFocus() {
	wxWindow::SetFocus();
}

void GCDCGraphs::OnSetFocus(wxFocusEvent& event) {
	SetFocus();
}

void GCDCGraphs::DrawYAxis(wxGraphicsContext& dc) {
	int w, h;
	GetClientSize(&w, &h);

	dc.StrokeLine(m_screen_margins.leftmargin, 0, m_screen_margins.leftmargin, h);
	dc.StrokeLine(m_screen_margins.leftmargin - 5, 7, m_screen_margins.leftmargin, 0);
	dc.StrokeLine(m_screen_margins.leftmargin, 0, m_screen_margins.leftmargin + 5, 7);
}

void GCDCGraphs::DrawXAxis(wxGraphicsContext &dc) {
	int arrow_width = 8;
	int arrow_height = 4;

	int w, h;
	GetClientSize(&w, &h);

	//dc.StrokeLine(0, h - m_screen_margins.bottommargin, w - m_screen_margins.rightmargin, h - m_screen_margins.bottommargin);
	dc.StrokeLine(0, h - m_screen_margins.bottommargin, w, h - m_screen_margins.bottommargin);
	dc.StrokeLine(w - arrow_width, h - m_screen_margins.bottommargin - arrow_height,
		w, h - m_screen_margins.bottommargin);
	dc.StrokeLine(w - arrow_width, h - m_screen_margins.bottommargin + arrow_height,
		w, h - m_screen_margins.bottommargin);
    
}

namespace {
const int PeriodMarkShift[PERIOD_T_LAST] = {0, 0, 1, 3, 0};
}


void GCDCGraphs::DrawUnit(wxGraphicsContext &dc) {
	Draw* draw = m_draws_wdg->GetSelectedDraw();
	if (draw == NULL) 
		return;

	double textw, texth, th, tsel;

	wxString unit = draw->GetDrawInfo()->GetUnit();
	dc.GetTextExtent(unit, &textw, &texth, &th, &tsel);
	dc.DrawText(unit, m_screen_margins.leftmargin - 6 - textw, m_screen_margins.topmargin - 2 - texth);
}

void GCDCGraphs::DrawWindowInfo(wxGraphicsContext &dc) {
	double info_left_marg = m_screen_margins.leftmargin + 8;
	double param_name_shift = 5;

	if (m_draws.GetCount() < 1)
		return;

	int w, h;
	GetClientSize(&w, &h);

	DrawInfo *info = m_draws[0]->GetDrawInfo();
	wxString name = info->GetSetName().c_str();

	double namew, nameh, th, tsel;
	dc.GetTextExtent(name, &namew, &nameh, &th, &tsel);

	dc.DrawText(name, info_left_marg, m_screen_margins.infotopmargin);

	int xpos = info_left_marg + namew + param_name_shift;

	for (int i = 0; i < (int)m_draws.GetCount(); ++i) {

		if (!m_draws[i]->GetEnable())
			continue;

		DrawInfo *info = m_draws[i]->GetDrawInfo();

		dc.SetFont(GetFont(), info->GetDrawColor());

		name = info->GetShortName().c_str();
		dc.GetTextExtent(name, &namew, &nameh, &th, &tsel);

		dc.DrawText(name, xpos, m_screen_margins.infotopmargin);
		xpos += namew + param_name_shift;
	}

}

void GCDCGraphs::DrawXAxisVals(wxGraphicsContext& dc) {
	Draw* draw = m_draws_wdg->GetSelectedDraw();
	if (draw == NULL) 
		return;
	/* Draw ticks on axis. */
	int i = PeriodMarkShift[draw->GetPeriod()];
	int size = draw->GetValuesTable().size();

	int w, h;
	GetClientSize(&w, &h);
    
	while (i < size) {
		int x = GetX(i);
		dc.StrokeLine(x, h - m_screen_margins.bottommargin + 1, x, h - m_screen_margins.bottommargin - 1);

		wxDateTime date = draw->GetTimeOfIndex(i);
	
		/* Print date */
		wxString datestring;

		switch (draw->GetPeriod()) {
			case PERIOD_T_YEAR :
				datestring = wxString::Format(_T("%02d"), date.GetMonth() + 1);
				break;
			case PERIOD_T_MONTH :
				datestring = wxString::Format(_T("%02d"), date.GetDay());
				break;
			case PERIOD_T_WEEK :
				switch (date.GetWeekDay()) {
					case 0 : datestring = _("Su"); break;
					case 1 : datestring = _("Mo"); break;
					case 2 : datestring = _("Tu"); break;
					case 3 : datestring = _("We"); break;
					case 4 : datestring = _("Th"); break;
					case 5 : datestring = _("Fr"); break;
					case 6 : datestring = _("Sa"); break;
					default : break;
				}
				break;
			case PERIOD_T_DAY :
				datestring = wxString::Format(_T("%02d"), date.GetHour());
				break;
			case PERIOD_T_SEASON :
				datestring = wxString::Format(_T("%02d"), date.GetWeekOfYear());
				break;
			default:
				break;
		}
	
		double textw, texth, td, tel;

		dc.GetTextExtent(datestring, &textw, &texth, &td, &tel);
		dc.DrawText(datestring, x - textw / 2, h - m_screen_margins.bottommargin + 1);

		i += Draw::PeriodMult[draw->GetPeriod()];
	}
    
}

void GCDCGraphs::DrawYAxisVals(wxGraphicsContext& dc) {
	Draw* draw = m_draws_wdg->GetSelectedDraw();
	if (draw == NULL) 
		return;
	DrawInfo *di = draw->GetDrawInfo();

	double min = di->GetMin();
	double max = di->GetMax();
	double dif = max - min;

	if (dif <= 0) {
		wxLogInfo(_T("%s %f %f"), di->GetName().c_str(), min, max);
		assert(false);
	}

	//procedure for calculating distance between marks stolen from SzarpDraw2
	double x = dif;
	double step;
	int i = 0;

	if (x < 1)
		for (;x < 1; x *=10, --i);
	else
		for (;(int)x / 10; x /=10, ++i);

	if (x <= 1.5)
		step = .1;
	else if (x <= 3.)
		step = .2;
	else if (x <= 7.5)
		step = .5;
	else
		step = 1.;

	double acc = 1;

	int prec = di->GetPrec();
				
	for (int p = prec; p > 0; --p)
		acc /= 10;

	double factor  = (i > 0) ? 10 : .1;

	for (;i; i -= i / abs(i))
		step *= factor;

	if (step < acc)
		step = acc;

    	dc.SetPen(wxPen(*wxWHITE, 1, wxSOLID));

	int w, h;
	GetClientSize(&w, &h);

	h -= m_screen_margins.bottommargin + m_screen_margins.topmargin;

	for (double val = max; (min - val) < acc; val -= step) {

		int y = GetY(val, di);

		dc.StrokeLine(m_screen_margins.leftmargin - 8, y, m_screen_margins.leftmargin, y);

		wxString sval = di->GetValueStr(val, _T("- -"));
		double textw, texth, textd, textel;
		dc.GetTextExtent(sval, &textw, &texth, &textd, &textel);
		dc.DrawText(sval, m_screen_margins.leftmargin - textw - 1, y + 2);
	}

}

void GCDCGraphs::DrawNoData(wxGraphicsContext &dc) {
	wxFont font = GetFont();
	font.SetPointSize(font.GetPointSize() + 8);
	dc.SetFont(font, *wxWHITE);

	int w, h;
	GetClientSize(&w, &h);

	w -= m_screen_margins.leftmargin - m_screen_margins.rightmargin;
	h -= m_screen_margins.topmargin - m_screen_margins.bottommargin;

	wxString sval = _("No data");
	double textw, texth, textd, textel;
	dc.GetTextExtent(sval, &textw, &texth, &textd, &textel);
	dc.DrawText(sval, m_screen_margins.leftmargin + w / 2 - textw / 2 - 1, m_screen_margins.topmargin + h / 2 - texth / 2);

}

void GCDCGraphs::DrawGraphs(wxGraphicsContext &dc) {
	dc.SetPen(wxPen(wxColour(255, 255, 255), 1, wxSOLID));

	int sel = m_draws_wdg->GetSelectedDrawIndex();
	if (sel < 0)
		return;

	for (size_t i = 0; i <= m_draws.GetCount(); i++) {
		 
		size_t j = i;
		if ((int) j == sel) 
			continue;
		if (j == m_draws.GetCount())	
			j = sel;

		Draw* d = m_draws[j];

		DrawGraph(dc, d);

		if ((int) j == sel)
			DrawCursor(dc, d);
	}

}

void GCDCGraphs::DrawGraph(wxGraphicsContext &dc, Draw* d) {
	if (d->GetEnable() == false)
		return;

	DrawInfo *di = d->GetDrawInfo();

	bool double_cursor = d->GetSelected() && d->IsDoubleCursor();

	wxGraphicsPath path1 = dc.CreatePath();
	wxGraphicsPath path2 = dc.CreatePath();

	wxGraphicsPath *path = &path1;

	const wxColour &wc = di->GetDrawColor();
	dc.SetPen(wxPen(wc, double_cursor ? 4 : 2, wxSOLID));

	int pc = d->GetValuesTable().size();

	bool prev_data = false;
	bool switched_to_alternate = false;
	bool switched_back = false;

	int dcs = -1, dce = -1;
	if (double_cursor) {
		dcs = d->GetValuesTable().m_stats.Start() - d->GetValuesTable().m_view.Start();
		dce = d->GetValuesTable().m_stats.End() - d->GetValuesTable().m_view.Start();
	}

	for (int i = 0; i < pc; i++) {
		if (!d->GetValuesTable().at(i).IsData()) {
			prev_data = false;
			continue;
		}

		double x = GetX(i);
		double y = GetY(d->GetValuesTable().at(i).val, di);

		bool drawn = false;

		if (i >= dcs && i <= dce && !switched_to_alternate) {
			if (prev_data)
				path->AddLineToPoint(x, y);

			path = &path2;

			path->MoveToPoint(x, y);
			if (d->GetPeriod() != PERIOD_T_DAY)
				path->AddEllipse(x - 3, y - 3, 6, 6);
			else if (!prev_data && ((i + 1) < pc) && !d->GetValuesTable().at(i + 1).IsData())
				path->AddCircle(x - 1, y, 1);

			switched_to_alternate = true;

			drawn = true;
		}

		if (i >= dce && switched_to_alternate && !switched_back) {
			if (prev_data) 
				path->AddLineToPoint(x, y);
			wxGraphicsPath* p;
			if (i == dce)
				p = &path2;
			else
				p = &path1;

			if (d->GetPeriod() != PERIOD_T_DAY)
				p->AddEllipse(x - 3, y - 3, 6, 6);
			else if (!prev_data && (i + 1) < pc && !d->GetValuesTable().at(i + 1).IsData())
				p->AddCircle(x - 1, y, 1);


			path = &path1;
			path->MoveToPoint(x, y);

			switched_back = true;

			drawn = true;
		}

		if (!drawn) {
			if (prev_data)
				path->AddLineToPoint(x, y);
			else
				path->MoveToPoint(x, y);

			if (d->GetPeriod() != PERIOD_T_DAY)
				path->AddEllipse(x - 3, y - 3, 6, 6);
			else if (!prev_data && ((i + 1) < pc) && !d->GetValuesTable().at(i + 1).IsData())
				path->AddCircle(x - 1, y, 1);
		}
		
		prev_data = true;
	}

	dc.StrokePath(path1);
	if (double_cursor) {
		dc.SetPen(wxPen(*wxWHITE, 4, wxSOLID));
		dc.StrokePath(path2);
	}
}

void GCDCGraphs::DrawCursor(wxGraphicsContext &dc, Draw* d) {
	int i = d->GetCurrentIndex();
	if (i < 0)
		return;

	if (d->GetValuesTable().at(i).IsData() == false)
		return;

	dc.SetBrush(wxBrush(wxColour(0,0,0), wxTRANSPARENT));
	dc.SetPen(wxPen(*wxWHITE, 1, wxSOLID));

	double x = GetX(i);
	double y = GetY(d->GetValuesTable().at(i).val, d->GetDrawInfo());

	dc.DrawRectangle(x - 4, y - 4, 9, 9);
}

void GCDCGraphs::OnPaint(wxPaintEvent& e) {
	wxBufferedPaintDC pdc(this);
	wxGraphicsContext* dc = wxGraphicsContext::Create(pdc);

	dc->SetFont(GetFont(), *wxWHITE);
	
	RecalculateMargins(*dc);
	DrawBackground(*dc);

	dc->SetBrush(*wxWHITE_BRUSH);
	dc->SetPen(wxPen(wxColour(255, 255, 255, 255), 1, wxSOLID));
	DrawXAxis(*dc);
	DrawXAxisVals(*dc);
	DrawYAxis(*dc);
	DrawYAxisVals(*dc);
	DrawUnit(*dc);
	DrawWindowInfo(*dc);
	DrawSeasonsLimitsInfo(*dc);
	if (m_draws_wdg->IsNoData() == true)
		DrawNoData(*dc);
	else
		DrawGraphs(*dc);
	DrawRemarksBitmaps(*dc);

	delete dc;
}

void GCDCGraphs::DrawRemarksBitmaps(wxGraphicsContext &dc) {
	Draw* draw = m_draws_wdg->GetSelectedDraw();
	if (draw == NULL)
		return;

	const ValuesTable& vt = draw->GetValuesTable();
	for (size_t i = 0; i < draw->GetValuesTable().size(); i++)
		if (vt.at(i).m_remark) { 
			double x = GetX(i);
			dc.DrawBitmap(m_remark_flag_bitmap,
					x - m_remark_flag_bitmap.GetWidth() / 2,
					m_screen_margins.topmargin - 7,
					m_remark_flag_bitmap.GetWidth(),
					m_remark_flag_bitmap.GetHeight());
		}
}

void GCDCGraphs::Refresh() {
	m_refresh = true;
}

void GCDCGraphs::RecalculateMargins(wxGraphicsContext &dc) {
	if (!m_recalulate_margins)
		return;

	double leftmargin = 36;
	double bottommargin = 12;
	double topmargin = 24;

	for (size_t i = 0; i < m_draws.GetCount(); i++) {
		double tw, th, td, tel;
		DrawInfo *di = m_draws[i]->GetDrawInfo();
		wxString sval = di->GetValueStr(di->GetMax(), _T(""));

		dc.GetTextExtent(sval, &tw, &th, &td, &tel);
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

	m_recalulate_margins = false;
}

void GCDCGraphs::DrawSeasonsLimitsInfo(wxGraphicsContext &dc) {
	Draw* draw = m_draws_wdg->GetSelectedDraw();
	if (draw == NULL)
		return;

	DrawsSets *cfg = m_cfg_mgr->GetConfigByPrefix(draw->GetDrawInfo()->GetBasePrefix());

	std::vector<SeasonLimit> limits = get_season_limits_indexes(cfg, draw);
	for (std::vector<SeasonLimit>::iterator i = limits.begin(); i != limits.end(); i++)
		DrawSeasonLimitInfo(dc, i->index, i->month, i->day, i->summer);

}

void GCDCGraphs::DrawSeasonLimitInfo(wxGraphicsContext &dc, int i, int month, int day, bool summer) {

	const double stripe_width = 2; 

	double x1 = GetX(i);
	double x2 = GetX(i + 1);

	double x = (x1 + x2) / 2;

	int w, h;
	GetClientSize(&w, &h);

	wxColour color = summer ? *wxBLUE : *wxRED;

	dc.SetFont(GetFont(), color);

	dc.SetPen(wxPen(color, 1, wxSOLID));

	wxBrush brush(color);
	dc.SetBrush(brush);
	dc.DrawRectangle(x + 1, m_screen_margins.topmargin, stripe_width, w - m_screen_margins.topmargin - m_screen_margins.bottommargin);

	wxString str;
	if (summer)
		str = wxString(_("summer season"));
	else
		str = wxString(_("winter season"));

	str += wxString::Format(_T(" %d "), day);

	switch (month) {
		case 1:
			str += _("january");
			break;
		case 2:
			str += _("february");
			break;
		case 3:
			str += _("march");
			break;
		case 4:
			str += _("april");
			break;
		case 5:
			str += _("may");
			break;
		case 6:
			str += _("june");
			break;
		case 7:
			str += _("july");
			break;
		case 8:
			str += _("august");
			break;
		case 9:
			str += _("septermber");
			break;
		case 10:
			str += _("october");
			break;
		case 11:
			str += _("november");
			break;
		case 12:
			str += _("december");
			break;
		default:
			assert(false);
	}

	double tw, th, td, tsel;
	dc.GetTextExtent(_T("W"), &tw, &th, &td, &tsel);

	int ty = m_screen_margins.topmargin + 1;
	for (size_t i = 0; i < str.Len(); ++i) {
		double lw, lh, ld, lsel;
		wxString letter = str.Mid(i, 1);
		dc.GetTextExtent(letter, &lw, &lh, &ld, &lsel);
		dc.DrawText(letter, x + stripe_width + 2 + (tw - lw) / 2, ty);
		ty += th + 2;
	}

}

int GCDCGraphs::GetRemarkClickedIndex(int x, int y) {
	if (y > m_screen_margins.topmargin - 7 + m_remark_flag_bitmap.GetHeight() || y < m_screen_margins.topmargin - 7)
		return -1;

	int sri = -1;
	int sdx = -1;

	Draw* draw = m_draws_wdg->GetSelectedDraw();
	if (draw == NULL)
		return -1;
	const ValuesTable& vt = draw->GetValuesTable();

	for (size_t i = 0; i < vt.size(); i++)
		if (vt.at(i).m_remark) { 
			int rx = GetX(i);

			int d = std::abs(x - rx);

			if (d < m_remark_flag_bitmap.GetWidth() / 2) {	//width of flag bitmap
				if (sri == -1 || sdx > d) {
					sri = i;
					sdx = d;
				}
			}
		}

	return sri;
}

int GCDCGraphs::GetIndex(int x, size_t s) {
	int w, h;
	GetClientSize(&w, &h);
    
	x -= m_screen_margins.leftmargin;
	w -= m_screen_margins.leftmargin + m_screen_margins.rightmargin;
    
	int index = x * s / w;

	return index;

}

void GCDCGraphs::GetDistance(size_t draw_index, int x, int y, double &d, wxDateTime &time) {
	d = INFINITY;
	time = wxInvalidDateTime;
	/* this defines search radius for 'direct hit' */

	int w,h;
	GetClientSize(&w, &h);
	
	const Draw::VT& vt = m_draws[draw_index]->GetValuesTable();
	/* forget about top and bottom margins */
	h -= m_screen_margins.bottommargin + m_screen_margins.topmargin;
	/* get X coordinate */
	int index = GetIndex(x, vt.size());

	int i, j;
	for (i = j = index; i >= 0 || j < (int)vt.size(); --i, ++j) {
		double d1 = INFINITY;
		double d2 = INFINITY;

		if (i >= 0 && i < (int)vt.size() && vt.at(i).IsData()) {
			double xi = GetX(i);
			double yi = GetY(vt[i].val, m_draws[draw_index]->GetDrawInfo());
			d1 = (x - xi) * (x - xi) + (y - yi) * (y - yi);
		}

		if (j >= 0 && j < (int)vt.size() && vt.at(j).IsData()) {
			double xj = GetX(j);
			double yj = GetY(vt[j].val, m_draws[draw_index]->GetDrawInfo());
			d2 = (x - xj) * (x - xj) + (y - yj) * (y - yj);
		}

		if (!std::isfinite(d1) && !std::isfinite(d1))
			continue;

		if (!std::isfinite(d2) || d1 < d2) {
			d = d1;
			time = m_draws[draw_index]->GetTimeOfIndex(i);
			wxLogInfo(_T("Nearest index %d, time: %s"), i, time.Format().c_str());
		} else {
			d = d2;
			time = m_draws[draw_index]->GetTimeOfIndex(j);
			wxLogInfo(_T("Nearest index %d, time: %s"), j, time.Format().c_str());
		}

		break;

	}

}

void GCDCGraphs::OnMouseLeftDown(wxMouseEvent& event) {
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

	VoteInfo infos[m_draws.GetCount()];

	int x = event.GetX();
	int y = event.GetY();

	int remark_idx = GetRemarkClickedIndex(x, y);
	if (remark_idx != -1) {
		m_draws_wdg->ShowRemarks(remark_idx);
		return;
	}

	for (size_t i = 0; i < m_draws.GetCount(); ++i) {

		VoteInfo & in = infos[i];
		in.dist = -1;

		if (m_draws[i]->GetEnable())
			GetDistance(i, x, y, in.dist, in.time);
	}

	double min = INFINITY;
	int j = -1;

	int selected_draw = m_draws_wdg->GetSelectedDrawIndex();

	for (size_t i = 1; i <= m_draws.GetCount(); ++i) {
		size_t k = (i + selected_draw) % m_draws.GetCount();

		VoteInfo & in = infos[k];
		if (in.dist < 0)
			continue;

		if (in.dist < 9 * 9
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

void GCDCGraphs::OnMouseLeftDClick(wxMouseEvent& event) {
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

void GCDCGraphs::OnMouseLeftUp(wxMouseEvent& event) {
	m_draws_wdg->StopMovingCursor();
}

void GCDCGraphs::OnMouseRightDown(wxMouseEvent& event) {
	if (m_draws_wdg->GetSelectedDrawIndex() == -1)
		return;

	Draw *d = m_draws[m_draws_wdg->GetSelectedDrawIndex()];
	DrawInfo *di = d->GetDrawInfo();

	SetInfoDataObject wido(di->GetBasePrefix(), di->GetSetName(), d->GetPeriod(), d->GetCurrentTime().GetTicks(), m_draws_wdg->GetSelectedDrawIndex());
	wxDropSource ds(wido, this);
	ds.DoDragDrop(0);
}

void GCDCGraphs::OnSize(wxSizeEvent& event) {
	Refresh();
}

void GCDCGraphs::OnEraseBackground(wxEraseEvent& event) {
	//
}

void GCDCGraphs::OnIdle(wxIdleEvent &e) {
	if (m_refresh) {
		wxWindow::Refresh();
		m_refresh = false;
	}
}

GCDCGraphs::~GCDCGraphs() {

}

BEGIN_EVENT_TABLE(GCDCGraphs, wxWindow) 
	EVT_PAINT(GCDCGraphs::OnPaint)
	EVT_IDLE(GCDCGraphs::OnIdle)
	EVT_LEFT_DOWN(GCDCGraphs::OnMouseLeftDown)
	EVT_LEFT_DCLICK(GCDCGraphs::OnMouseLeftDClick)
	EVT_LEAVE_WINDOW(GCDCGraphs::OnMouseLeftUp)
	EVT_LEFT_UP(GCDCGraphs::OnMouseLeftUp)
	EVT_SIZE(GCDCGraphs::OnSize)
	EVT_SET_FOCUS(GCDCGraphs::OnSetFocus)
	EVT_ERASE_BACKGROUND(GCDCGraphs::OnEraseBackground)
END_EVENT_TABLE()

#endif
