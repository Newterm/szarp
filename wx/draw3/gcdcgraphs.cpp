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

#include <wx/dcbuffer.h>

#include "conversion.h"

#include "bitmaps/remark_flag.xpm"
#include "ids.h"
#include "classes.h"
#include "cconv.h"
#include "cfgmgr.h"
#include "drawtime.h"
#include "drawdnd.h"
#include "dbinquirer.h"
#include "database.h"
#include "drawobs.h"
#include "draw.h"
#include "coobs.h"
#include "drawsctrl.h"
#include "drawswdg.h"
#include "drawapp.h"
#include "gcdcgraphs.h"
#include "graphsutils.h"

/**Significant part of this code is just copied from classic graphs implementation, it's bad practice, I know, but I just wanted
 * to make it work fast. 3.0 version of wxWidgets promises to provide wxGCDC that would have
 * the same API as wxDC, so we won't need most of this code anyway.*/

GCDCGraphs::GCDCGraphs(wxWindow* parent, ConfigManager *cfg) : wxWindow(parent, wxID_ANY), m_right_down(false), m_draw_param_name(false), m_cfg_mgr(cfg), m_recalculate_margins(true) {
	m_screen_margins.leftmargin = 10;
	m_screen_margins.rightmargin = 10;
	m_screen_margins.topmargin = 10;
	m_screen_margins.bottommargin = 12;
	m_screen_margins.infotopmargin = 7;
	m_refresh = true;

	m_draws_wdg = NULL;

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

	double y = get_y_position(value, di);

	double ret = h - y * h  + m_screen_margins.topmargin;

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

	if ( GetShowArrowsChecked() == true) {

		wxBitmap left_arrow;
		wxBitmap right_arrow;

#ifndef __MINGW32__
		left_arrow.LoadFile("../../resources/wx/images/left-arrow.png", wxBITMAP_TYPE_PNG);
		right_arrow.LoadFile("../../resources/wx/images/right-arrow.png", wxBITMAP_TYPE_PNG);
#else
		wxString left_arrow_path = wxGetApp().GetSzarpDir() + _T("\\resources\\wx\\images\\left-arrow.png");
		wxString right_arrow_path = wxGetApp().GetSzarpDir() + _T("\\resources\\wx\\images\\right-arrow.png");
		left_arrow.LoadFile(left_arrow_path, wxBITMAP_TYPE_PNG);
		right_arrow.LoadFile(right_arrow_path, wxBITMAP_TYPE_PNG);
#endif

		dc.DrawBitmap(left_arrow, 0, 0, 40, h);
		dc.DrawBitmap(right_arrow, w - 40, 0, 40, h);
	}

	size_t i = 0;
	int c = 1;
	double x = m_screen_margins.leftmargin + 1;

	while (i < pc) {
		double x1 = GetX(i);

		i += TimeIndex::PeriodMult[pt];
	
		double x2 = GetX(i);

		if (c > 0)
			dc.SetBrush(wxBrush(back1_col, wxSOLID));
		else
			dc.SetBrush(wxBrush(back2_col, wxSOLID));
	
		c *= -1; 

		dc.DrawRectangle(x, m_screen_margins.topmargin, x2 - x1, h - m_screen_margins.topmargin + 1);

		x = x + x2 - x1;
	}

}

wxDragResult GCDCGraphs::SetSetInfo(wxCoord x, wxCoord y, wxString window, wxString prefix, time_t time, PeriodType pt, wxDragResult def) {
	return m_draws_wdg->SetSetInfo(window, prefix, time, pt, def);
}

void GCDCGraphs::StartDrawingParamName() {
	if (!m_draw_param_name) {
		m_draw_param_name = true;
		Refresh();
	}
}

void GCDCGraphs::DrawSelected(Draw *draw) {
	Refresh();
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

	if (m_draws.size() < 1)
		return;

	int w, h;
	GetClientSize(&w, &h);

	DrawInfo *info = m_draws[0]->GetDrawInfo();
	wxString name = info->GetSetName().c_str();

	double namew, nameh, th, tsel;
	dc.GetTextExtent(name, &namew, &nameh, &th, &tsel);

	dc.DrawText(name, info_left_marg, m_screen_margins.infotopmargin);

	int xpos = info_left_marg + namew + param_name_shift;

	for (int i = 0; i < (int)m_draws.size(); ++i) {

		if (!m_draws[i]->GetEnable())
			continue;

		DrawInfo *info = m_draws[i]->GetDrawInfo();

		dc.SetFont(GetFont(), info->GetDrawColor());

		name = info->GetShortName().c_str();
		if (!name.IsEmpty()) {
			dc.GetTextExtent(name, &namew, &nameh, &th, &tsel);
			dc.DrawText(name, xpos, m_screen_margins.infotopmargin);
			xpos += namew + param_name_shift;
		} else {
			xpos += param_name_shift;
		}
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
    
	wxDateTime prev_date;
	while (i < size) {
		int x = GetX(i);
		dc.StrokeLine(x, h - m_screen_margins.bottommargin + 1, x, h - m_screen_margins.bottommargin - 1);

		wxDateTime date = draw->GetTimeOfIndex(i);
	
		/* Print date */
		wxString datestring = get_date_string(draw->GetPeriod(), prev_date, date);

		double textw, texth, td, tel;

		dc.GetTextExtent(datestring, &textw, &texth, &td, &tel);
		dc.DrawText(datestring, x - textw / 2, h - m_screen_margins.bottommargin + 1);

		i += TimeIndex::PeriodMult[draw->GetPeriod()];
		prev_date = date;
	}
    
}

void GCDCGraphs::DrawYAxisVals(wxGraphicsContext& dc) {
	Draw* draw = m_draws_wdg->GetSelectedDraw();
	if (draw == NULL) 
		return;
	DrawInfo *di = draw->GetDrawInfo();

	if (!di->IsValid())
		return;

	double min = di->GetMin();
	double max = di->GetMax();
	if( max <= min  ) {
		// FIXME:  Draw3 should atomaticly detect axes in that case, but
		//         it currently doesnt :(
		wxLogWarning(_T("Parameter %s has invalid min/max values: min %f, max %f. Min is set to 0, and max to 100."), di->GetName().c_str(), min, max);
		min = 0;
		max = 100;
	}

	//procedure for calculating distance between marks stolen from SzarpDraw2
	double x = max - min;
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

	for (size_t i = 0; i <= m_draws.size(); i++) {
		 
		size_t j = i;
		if ((int) j == sel) 
			continue;
		if (j == m_draws.size())	
			j = sel;

		Draw* d = m_draws[j];

		DrawGraph(dc, d);

		if ((int) j == sel)
			DrawCursor(dc, d);
	}

}


void GCDCGraphs::DrawGraph(wxGraphicsContext &dc, Draw* d) {
	static const int ellipse_size = 5;

	if (d->GetEnable() == false)
		return;

	DrawInfo *di = d->GetDrawInfo();

	bool double_cursor = d->GetSelected() && d->GetDoubleCursor();

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

	std::vector<std::pair<double,double> > p1circles;
	std::vector<std::pair<double,double> > p2circles;
	std::vector<std::pair<double,double> >* pcircles = &p1circles;

	bool draw_circle =
		d->GetPeriod() != PERIOD_T_DAY
		&& d->GetPeriod() != PERIOD_T_30MINUTE
		&& d->GetPeriod() != PERIOD_T_5MINUTE
		&& d->GetPeriod() != PERIOD_T_MINUTE
		&& d->GetPeriod() != PERIOD_T_30SEC;

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
			pcircles = &p2circles;

			if (draw_circle || (!prev_data && ((i + 1) < pc) && !d->GetValuesTable().at(i + 1).IsData())) 
				pcircles->push_back(std::make_pair(x, y));

			path->MoveToPoint(x, y);
			switched_to_alternate = true;
			drawn = true;
		}

		if (i >= dce && switched_to_alternate && !switched_back) {
			if (prev_data)
				path->AddLineToPoint(x, y);

			std::vector<std::pair<double, double> > *p;
			if (i == dce)
				p = &p2circles;
			else
				p = &p1circles;

			if (draw_circle || (!prev_data && ((i + 1) < pc) && !d->GetValuesTable().at(i + 1).IsData())) 
				p->push_back(std::make_pair(x, y));

			path = &path1;
			pcircles = &p1circles;
			path->MoveToPoint(x, y);

			switched_back = true;

			drawn = true;
		}

		if (!drawn) {
			if (prev_data)
				path->AddLineToPoint(x, y);
			else
				path->MoveToPoint(x, y);

			if (draw_circle || (!prev_data && ((i + 1) < pc) && !d->GetValuesTable().at(i + 1).IsData())) 
				pcircles->push_back(std::make_pair(x, y));
		}
		
		prev_data = true;
	}

	for (std::vector<std::pair<double, double> >::iterator i = p1circles.begin();
			i != p1circles.end();
			i++)
		if (draw_circle)
			path1.AddEllipse(i->first - ellipse_size / 2, i->second - ellipse_size / 2, ellipse_size, ellipse_size);
		else
			path1.AddCircle(i->first, i->second, 1);

	dc.StrokePath(path1);

	if (double_cursor) {
		dc.SetPen(wxPen(*wxWHITE, 4, wxSOLID));
		for (std::vector<std::pair<double, double> >::iterator i = p2circles.begin();
				i != p2circles.end();
				i++)
			if (draw_circle)
				path2.AddEllipse(i->first - ellipse_size / 2, i->second - ellipse_size / 2, ellipse_size, ellipse_size);
			else
				path2.AddCircle(i->first, i->second, 1);
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

void GCDCGraphs::DrawParamName(wxGraphicsContext &dc) {
	DrawInfo *di = m_draws_wdg->GetCurrentDrawInfo();
	if (di == NULL)
		return;

	dc.SetBrush(*wxBLACK_BRUSH);
	dc.SetPen(*wxWHITE_PEN);

	wxFont f = GetFont();
	int ps = f.GetPointSize();
	int fw = f.GetWeight();
	f.SetWeight(wxFONTWEIGHT_BOLD);
	f.SetPointSize(ps * 1.25);
	dc.SetFont(f, di->GetDrawColor());

	int w, h;
	GetSize(&w, &h);

	wxString text = m_cfg_mgr->GetConfigTitles()[di->GetBasePrefix()] + _T(":") + di->GetParamName();

	double tw, th, _th, _ts;
	dc.GetTextExtent(text, &tw, &th, &_th, &_ts);

	dc.DrawRectangle(w / 2 - tw / 2 - 1, h / 2 - th / 2 - 1, tw + 2, th + 2);
	dc.DrawText(text, w / 2 - tw / 2, h / 2 - th / 2);

	f.SetPointSize(ps);
	f.SetWeight(fw);
}

void GCDCGraphs::OnPaint(wxPaintEvent& e) {
	if (m_draws_wdg->GetSelectedDraw() == NULL)
		return;

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
	if (m_draws_wdg->GetNoData() == true)
		DrawNoData(*dc);
	else
		DrawGraphs(*dc);
	DrawRemarksBitmaps(*dc);

	if (m_draw_param_name) 
		DrawParamName(*dc);

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

void GCDCGraphs::SetMarginsRecalculable() {
	m_recalculate_margins = true;
}

void GCDCGraphs::RecalculateMargins(wxGraphicsContext &dc) {
	if (!m_recalculate_margins)
		return;

	double leftmargin = 36;
	double bottommargin = 12;
	double topmargin = 24;

	for (size_t i = 0; i < m_draws.size(); i++) {
		double tw, th, td, tel;
		DrawInfo *di = m_draws[i]->GetDrawInfo();
		wxString sval = di->GetValueStr(di->GetMax(), _T(""));

		dc.GetTextExtent(sval, &tw, &th, &td, &tel);
		if ( isShowArrowsChecked == true) {
			tw += 42;
			m_screen_margins.rightmargin = 40;
		} else if ( isShowArrowsChecked == false) {
			m_screen_margins.rightmargin = 10;
		}
		if (leftmargin < tw + 1)
			leftmargin = tw + 1;
		if (bottommargin < th + 1)
			bottommargin = th + 1;

		if (topmargin < th + m_screen_margins.infotopmargin)
			topmargin = th + m_screen_margins.infotopmargin;

		dc.GetTextExtent(di->GetUnit(), &tw, &th, &td, &tel);
		if (leftmargin < tw + 6)
			leftmargin = tw + 6;
	}

	m_screen_margins.leftmargin = leftmargin;
	m_screen_margins.bottommargin = bottommargin;
	m_screen_margins.topmargin = topmargin;

	m_recalculate_margins = false;
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

		if (!std::isfinite(d1) && !std::isfinite(d2))
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

	VoteInfo infos[m_draws.size()];

	int x = event.GetX();
	int y = event.GetY();

	int remark_idx = GetRemarkClickedIndex(x, y);
	if (remark_idx != -1) {
		m_draws_wdg->ShowRemarks(remark_idx);
		return;
	}

	for (size_t i = 0; i < m_draws.size(); ++i) {

		VoteInfo & in = infos[i];
		in.dist = -1;

		if (m_draws[i]->GetEnable())
			GetDistance(i, x, y, in.dist, in.time);
	}

	double min = INFINITY;
	int j = -1;

	int selected_draw = m_draws_wdg->GetSelectedDrawIndex();

	for (size_t i = 1; i <= m_draws.size(); ++i) {
		size_t k = (i + selected_draw) % m_draws.size();

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
	if (m_draws.size() == 0)
		return;

	if (m_draws_wdg->GetNoData())
		return;

#ifdef __WXMSW__
	SetFocus();
#endif

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

void GCDCGraphs::OnMouseMove(wxMouseEvent &event) {
	if (!m_right_down)
		return;

	if (m_draws_wdg->GetNoData())
		return;

	int index = m_draws_wdg->GetSelectedDrawIndex();
	if (index == -1)
		return;

	int w, h;
	GetClientSize(&w, &h);

	if (event.GetX() < m_screen_margins.leftmargin) {
		m_draws_wdg->SetKeyboardAction(CURSOR_LEFT_KB);
		return;
	}

	if (event.GetX() > (w - m_screen_margins.rightmargin)) {
		m_draws_wdg->SetKeyboardAction(CURSOR_RIGHT_KB);
		return;
	}

	m_draws_wdg->SetKeyboardAction(NONE);

	int x = event.GetX();
	int y = event.GetY();
	double dist;
	wxDateTime time;

	GetDistance(index, x, y, dist, time);
	m_draws_wdg->SelectDraw(index, true, time);
}

void GCDCGraphs::OnMouseRightDown(wxMouseEvent& event) {
	if (m_right_down)
		return;
	m_right_down = true;
	m_draws_wdg->DoubleCursorSet(true);
	OnMouseMove(event);
}

void GCDCGraphs::OnMouseRightUp(wxMouseEvent& event) {
	if (!m_right_down)
		return;

	m_right_down = false;
	m_draws_wdg->SetKeyboardAction(NONE);
	m_draws_wdg->DoubleCursorSet(false);
}

void GCDCGraphs::OnMouseLeave(wxMouseEvent& event) {
	OnMouseLeftUp(event);
	OnMouseRightUp(event);
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

void GCDCGraphs::NoData(DrawsController *d) {
	m_refresh = true;
}

void GCDCGraphs::AverageValueCalculationMethodChanged(Draw *d) {
	m_refresh = true;
}

void GCDCGraphs::ResetGraphs(DrawsController *controller) {
	m_draws.resize(0);
	for (size_t i = 0; i < controller->GetDrawsCount(); i++)
		m_draws.push_back(controller->GetDraw(i));

	m_recalculate_margins = true;
	Refresh();
	SetFocus();
}

void GCDCGraphs::DrawsSorted(DrawsController* d) {
	ResetGraphs(d);
}

void GCDCGraphs::DrawInfoChanged(Draw *draw) { 
	if (draw->GetSelected())
		ResetGraphs(draw->GetDrawsController());
}

bool GCDCGraphs::GetShowArrowsChecked() {
	return isShowArrowsChecked;
}

void GCDCGraphs::SetShowArrowsChecked(bool setArrow) {
	isShowArrowsChecked = setArrow;
}

GCDCGraphs::~GCDCGraphs() {

}

BEGIN_EVENT_TABLE(GCDCGraphs, wxWindow) 
	EVT_PAINT(GCDCGraphs::OnPaint)
	EVT_IDLE(GCDCGraphs::OnIdle)
	EVT_LEFT_DOWN(GCDCGraphs::OnMouseLeftDown)
	EVT_LEFT_DCLICK(GCDCGraphs::OnMouseLeftDClick)
	EVT_RIGHT_DOWN(GCDCGraphs::OnMouseRightDown)
	EVT_RIGHT_UP(GCDCGraphs::OnMouseRightUp)
	EVT_LEAVE_WINDOW(GCDCGraphs::OnMouseLeave)
	EVT_MOTION(GCDCGraphs::OnMouseMove)
	EVT_LEFT_UP(GCDCGraphs::OnMouseLeftUp)
	EVT_SIZE(GCDCGraphs::OnSize)
	EVT_SET_FOCUS(GCDCGraphs::OnSetFocus)
	EVT_ERASE_BACKGROUND(GCDCGraphs::OnEraseBackground)
END_EVENT_TABLE()

