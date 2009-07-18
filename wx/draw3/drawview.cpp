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
#include <algorithm> 
#ifdef __WXGTK__
#include <gdk/gdk.h>
#else
#include <wx/msw/private.h>
#endif 

#include <wx/datetime.h>

#include "szarp_config.h"

#include "ids.h"

#include "drawview.h"
#include "drawswdg.h"
#include "cconv.h"
#include "draw.h"
#include "wxgraphs.h"
#include "bitmaps/remark_flag.xpm"

using std::max;
using std::min;

const int GraphView::cursorrectanglesize = 9;

const wxColour BackgroundView::timeaxis_col = wxColour(255, 255, 255);
const wxColour BackgroundView::back1_col = wxColour(128, 128, 128);
const wxColour BackgroundView::back2_col = wxColour(0, 0, 0);

const wxColour GraphDrawer::alt_color = wxColour(255, 255, 255);

const int View::PeriodMarkShift[PERIOD_T_LAST] = {0, 0, 1, 3, 0};

void View::ClearDC(wxMemoryDC *dc,wxBitmap *bitmap, int w, int h) {
	dc->SelectObject(*bitmap);
	dc->SetBackground(*wxBLACK_BRUSH);
	dc->Clear();
	dc->SetTextForeground(wxNullColour);
	dc->SetPen(*wxTRANSPARENT_PEN);
	dc->SetBrush(*wxTRANSPARENT_BRUSH);
}

long int View::GetX(int i) const
{
	int w, h;
	GetSize(&w, &h);

	size_t size = m_draw->GetValuesTable().size();

	double res = ((double) w - m_leftmargin - m_rightmargin) / size;

	double ret = i * res + m_leftmargin + res / 2;

	return (int)ret;
}

long int View::GetY(double value) const {
   	int w, h;
	GetSize(&w, &h);

	h -= m_bottommargin + m_topmargin;

	double max = m_draw->GetDrawInfo()->GetMax();
	double min = m_draw->GetDrawInfo()->GetMin();

	double dmax = max - min;
	double dmin = 0;

	double smin = 0;
	double smax = 0;
	double k = 0;

	int sc = m_draw->GetDrawInfo()->GetScale();
	if (sc) {
		smin = m_draw->GetDrawInfo()->GetScaleMin() - min;
		smax = m_draw->GetDrawInfo()->GetScaleMax() - min;
		assert(smax > smin);
		k = sc / 100. * (dmax - dmin) / (smax - smin);
		dmax += (k - 1) * (smax - smin);
	}

	double dif = dmax - dmin;
	if (dif <= 0) {
		wxLogInfo(_T("%s %f %f"), m_draw->GetDrawInfo()->GetName().c_str(), min, max);
		assert(false);
	}

	double dvalue = value - min;

	double scaled = 
		wxMax(dvalue - smax, 0) +
		wxMax(wxMin(dvalue - smin, smax - smin), 0) * k +
		wxMax(wxMin(dvalue - dmin, smin), 0);

	int ret = int(h - scaled * h / dif) + m_topmargin;

	if (ret < m_topmargin)
		ret = m_topmargin;

	if (ret > h + m_topmargin) 
		ret = h + m_topmargin;
		
	return ret;
}


void View::GetPointPosition(wxDC* dc, int i, int *x, int *y) const {
	*x = GetX(i);

	double value = m_draw->GetValuesTable().at(i).val;
	*y = GetY(value);
}

BackgroundView::BackgroundView(WxGraphs *graphs, ConfigManager *cfg) : 
		m_graphs(graphs), m_dc(NULL), m_bmp(NULL), m_cfg_mgr(cfg)
{
	m_remark_flag_bitmap = wxBitmap(remark_flag_xpm);
}

void BackgroundView::Attach(Draw *draw) {
	m_draw = draw;
	draw->AttachObserver(this);

	int w, h;
	m_graphs->GetClientSize(&w, &h);
	SetSize(w, h);
}

void BackgroundView::PeriodChanged(Draw *draw, PeriodType period) {
	DoDraw(m_dc);
}

void BackgroundView::DrawInfoChanged(Draw *draw) {
	DoDraw(m_dc);
}

void BackgroundView::ScreenMoved(Draw *draw, const wxDateTime &start_date) {
	DoDraw(m_dc);
	m_graphs->Refresh();
}

void BackgroundView::NewRemarks(Draw *draw) {
	DoDraw(m_dc);
	m_graphs->Refresh();
};

void BackgroundView::SetSize(int w, int h) {
	delete m_dc;
	delete m_bmp;
	m_dc = new wxMemoryDC();
	m_dc->SetFont(m_graphs->GetFont());

	m_bmp = new wxBitmap(w, h);

	ClearDC(m_dc, m_bmp, w, h);

	DoDraw(m_dc);
}

void BackgroundView::Detach(Draw *draw) {
	delete m_dc;
	delete m_bmp;
	
	m_dc = NULL;
	m_bmp = NULL;

	draw->DetachObserver(this);
	m_draw = NULL;
}

BackgroundDrawer::BackgroundDrawer() {}

void BackgroundView::DrawSeasonLimitInfo(wxDC *dc, int i, int month, int day, bool summer) {

	const int stripe_width = 2; 

	long int x1 = GetX(i);
	long int x2 = GetX(i + 1);

	long int x = (x1 + x2) / 2;

	int w, h;
	GetSize(&w, &h);

	wxColour color = summer ? *wxBLUE : *wxRED;

	dc->SetTextForeground(color);

	wxPen open = dc->GetPen();
	wxPen pen = dc->GetPen();

	pen.SetColour(color);
	dc->SetPen(pen);

	wxBrush obrush = dc->GetBrush();
	wxBrush brush(color);
	dc->SetBrush(brush);
	dc->DrawRectangle(x + 1, m_topmargin, stripe_width, w - m_topmargin - m_bottommargin);

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

	int tw, th;
	dc->GetTextExtent(_T("W"), &tw, &th);

	int ty = m_topmargin + 1;
	for (size_t i = 0; i < str.Len(); ++i) {
		int lw, lh;
		wxString letter = str.Mid(i, 1);
		dc->GetTextExtent(letter, &lw, &lh);
		dc->DrawText(letter, x + stripe_width + 2 + (tw - lw) / 2, ty);
		ty += th + 2;
	}

	dc->SetPen(open);
	dc->SetBrush(obrush);
	
}

void BackgroundView::DrawSeasonLimits(wxDC *dc) {

	DrawsSets *cfg = m_cfg_mgr->GetConfigByPrefix(m_draw->GetDrawInfo()->GetBasePrefix());
	IPKConfig *ipk = dynamic_cast<IPKConfig*>(cfg);

	if (ipk == NULL)
		return;

	const TSSeason* s = ipk->GetTSzarpConfig()->GetSeasons();

	wxDateTime pt = m_draw->GetTimeOfIndex(0);
	wxDateTime::Tm tm = pt.GetTm(wxDateTime::Local);
	int year = tm.year;

	TSSeason::Season season = s->GetSeason(year);
	bool is_summer = s->CheckSeason(season, tm.mon - wxDateTime::Jan + 1, tm.mday);

	for (size_t i = 1; i < m_draw->GetValuesTable().size() - 1; ++i) {
		wxDateTime t = m_draw->GetTimeOfIndex(i + 1);
		tm = t.GetTm(wxDateTime::Local);
		if (tm.year != year) {
			year = tm.year;
			season = s->GetSeason(year);
		}
		bool is = s->CheckSeason(season, (tm.mon - wxDateTime::Jan) + 1, tm.mday);
		if (is != is_summer) {
			int index;
			switch (m_draw->GetPeriod()) {
				case PERIOD_T_DAY:
				case PERIOD_T_MONTH:
				case PERIOD_T_WEEK:
					index = i;
					break;
				case PERIOD_T_LAST:
				case PERIOD_T_OTHER:
				case PERIOD_T_SEASON:
				case PERIOD_T_YEAR:
					if (is)
						if (tm.mday == season.day_start)
							index = i;
						else
							index = i - 1;
					else
						if (tm.mday  == season.day_end)
							index = i;
						else
							index = i - 1;

					break;
				default:
					index = 0;
					assert(false);
			}
			DrawSeasonLimitInfo(dc, index, is ? season.month_start : season.month_end, is ? season.day_start : season.day_end, is);
		}
		is_summer = is;
		pt = t;
	}
}

void BackgroundView::DrawRemarksFlags(wxDC *dc) {
	const ValuesTable& vt = m_draw->GetValuesTable();
	for (size_t i = 0; i < m_draw->GetValuesTable().size(); i++)
		if (vt.at(i).m_remark) { 
			int x = GetX(i);
			dc->DrawBitmap(m_remark_flag_bitmap, x, m_topmargin - 7, true);
		}
}

void BackgroundView::DoDraw(wxDC *dc) {

	dc->Clear();

	DrawBackground(dc);

	DrawSeasonLimits(dc);

	DrawTimeAxis(dc, 8, 4, 6);

	DrawYAxis(dc, 7, 5);

	DrawUnit(dc, 6, 8);

    	DrawYAxisVals(dc, 4);

	DrawRemarksFlags(dc);
}

wxFont BackgroundView::GetFont() const {
	return m_graphs->GetFont();
}

void BackgroundDrawer::DrawYAxisVals(wxDC *dc, int tick_len, int line_width) {
	double min = m_draw->GetDrawInfo()->GetMin();
	double max = m_draw->GetDrawInfo()->GetMax();
	double dif = max - min;

	if (dif <= 0) {
		wxLogInfo(_T("%s %f %f"), m_draw->GetDrawInfo()->GetName().c_str(), min, max);
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

	int prec = m_draw->GetDrawInfo()->GetPrec();
				

	for (int p = prec; p > 0; --p)
		acc /= 10;

	double factor  = (i > 0) ? 10 : .1;

	for (;i; i -= i / abs(i))
		step *= factor;

	if (step < acc)
		step = acc;

    	dc->SetPen(wxPen(GetTimeAxisCol(), line_width, wxSOLID));

	int w, h;
	GetSize(&w, &h);

	h -= m_bottommargin + m_topmargin;

	for (double val = max; (min - val) < acc; val -= step) {
	//for (double val = min; (val - max) < acc; val += step) {

		int y = GetY(val);

		dc->DrawLine(m_leftmargin - tick_len, y, m_leftmargin, y);

		wxString sval = m_draw->GetDrawInfo()->GetValueStr(val, _T("- -"));
		int textw, texth;
		dc->GetTextExtent(sval, &textw, &texth);
		dc->DrawText(sval, m_leftmargin - textw - 1, y + line_width / 2 + 1 );
	}

	dc->SetPen(wxNullPen);

}

void BackgroundDrawer::DrawBackground(wxDC *dc) {
	assert(dc);

	if (!dc->Ok())
		return;

	int w, h;
	GetSize(&w, &h);

	dc->SetPen(GetBackCol2());
	dc->SetBrush(wxBrush(GetBackCol2(), wxSOLID));
	
	int c = 1;
	int x = m_leftmargin + 1;

	size_t pc = m_draw->GetValuesTable().size();

	size_t i = 0;
	while (i < pc) {
		int x1 = GetX(i);

		i += Draw::PeriodMult[m_draw->GetPeriod()];
	
		int x2 = GetX(i);

		if (c > 0)
			dc->SetBrush(wxBrush(GetBackCol1(), wxSOLID));
		else
			dc->SetBrush(wxBrush(GetBackCol2(), wxSOLID));
	
		c *= -1; 

		int xn; 
		if (i < pc)
			xn = x + x2 - x1;
		else
			xn = w - m_rightmargin;

		dc->DrawRectangle(x, m_topmargin, x2 - x1, h - m_topmargin + 1);

		x = x + x2 - x1;
	}
	
	dc->SetPen(wxNullPen);
	dc->SetBrush(wxNullBrush);

}

void BackgroundDrawer::DrawUnit(wxDC *dc, int x, int y) {
	int textw, texth;
	wxString unit = m_draw->GetDrawInfo()->GetUnit();
	dc->GetTextExtent(unit, &textw, &texth);
	dc->DrawText(unit, m_leftmargin - x - textw, y);
}
	

void BackgroundDrawer::DrawYAxis(wxDC *dc, int arrow_height, int arrow_width, int line_width) {
	int w, h;
	GetSize(&w, &h);

	dc->SetPen(wxPen(GetTimeAxisCol(), line_width, wxSOLID));

	dc->DrawLine(m_leftmargin, 0, m_leftmargin, h);
	dc->DrawLine(m_leftmargin - arrow_width, arrow_height, m_leftmargin, 0);
	dc->DrawLine(m_leftmargin, 0, m_leftmargin + arrow_width, arrow_height);
}

/* Draw time axis (horizontal). */
void BackgroundDrawer::DrawTimeAxis(wxDC *dc, int arrow_width, int arrow_height, int tick_height, int line_width) {
	/* Get size. */
	int w, h;
	GetSize(&w, &h);

	/* Set solid white pen. */
	dc->SetPen(wxPen(GetTimeAxisCol(), line_width, wxSOLID));
	dc->SetBrush(*wxTRANSPARENT_BRUSH);
	/* Draw line with arrow. */
	dc->DrawLine(0, h - m_bottommargin, w - m_rightmargin, h - m_bottommargin);
	dc->DrawLine(w - m_rightmargin - arrow_width, h - m_bottommargin - arrow_height,
		w - m_rightmargin, h - m_bottommargin);
	dc->DrawLine(w - m_rightmargin - arrow_width, h - m_bottommargin + arrow_height,
		w - m_rightmargin, h - m_bottommargin);
    
	/* Draw ticks on axis. */
	int i = PeriodMarkShift[m_draw->GetPeriod()];
	int size = m_draw->GetValuesTable().size();
    
	while (i < size) {
		int x = GetX(i);
		dc->DrawLine(x, h - m_bottommargin + tick_height / 2, x, h - m_bottommargin - tick_height / 2);

		wxDateTime date = m_draw->GetTimeOfIndex(i);
	
		/* Print date */
		wxString datestring;

		switch (m_draw->GetPeriod()) {
			case PERIOD_T_YEAR :
				datestring = wxString::Format(_T("%02d"), date.GetMonth() + 1);
				break;
			case PERIOD_T_MONTH :
				datestring = wxString::Format(_T("%02d"), date.GetDay());
				break;
			case PERIOD_T_WEEK :
				datestring = GetShortDayName(date.GetWeekDay());
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
	
		int textw, texth;

		dc->GetTextExtent(datestring, &textw, &texth);
		dc->SetTextForeground(GetTimeAxisCol());
		dc->DrawText(datestring, x - textw / 2, h - m_bottommargin + 1);

		i += Draw::PeriodMult[m_draw->GetPeriod()];
	}
    
	dc->SetPen(wxNullPen);
}

wxDC* BackgroundView::GetDC() {
	return m_dc;
}

wxString BackgroundDrawer::GetShortDayName(wxDateTime::WeekDay day)
{
    switch (day) {
	case 0 : return _("Su");
	case 1 : return _("Mo");
	case 2 : return _("Tu");
	case 3 : return _("We");
	case 4 : return _("Th");
	case 5 : return _("Fr");
	case 6 : return _("Sa");
	default :
		 break;
    }
    return _T("?");
}

const wxColour& BackgroundView::GetTimeAxisCol() {
	return timeaxis_col;
}

const wxColour& BackgroundView::GetBackCol1() {
	return back1_col;
}

const wxColour& BackgroundView::GetBackCol2() {
	return back2_col;
}

void BackgroundView::GetSize(int *w, int *h) const {
	m_dc->GetSize(w, h);
}

int BackgroundView::GetRemarkClickedIndex(int x, int y) {
	if (y > m_topmargin || y < m_topmargin - 7)
		return -1;

	int sri = -1;
	int sdx = -1;

	const ValuesTable& vt = m_draw->GetValuesTable();

	for (size_t i = 0; i < vt.size(); i++)
		if (vt.at(i).m_remark) { 
			int rx = GetX(i);

			if (rx > x)
				break;
			int d = x - rx;

			if (d < m_remark_flag_bitmap.GetWidth()) {	//width of flag bitmap
				if (sri == -1 || sdx > d) {
					sri = i;
					sdx = d;
				}
			}
		}

	return sri;
}

BackgroundView::~BackgroundView() {
	delete m_dc;
	delete m_bmp;
}

GraphView::GraphView(WxGraphs* widget) : GraphDrawer(2), m_graphs(widget), m_dc(NULL), m_bmp(NULL), m_gc(NULL),
#ifdef __WXGTK__
	m_gdkbmp(NULL)
#else
	m_mswbmp(NULL)
#endif
{}

void GraphView::Attach(Draw *draw) {
	m_draw = draw;
	draw->AttachObserver(this);

	int w, h;
	m_graphs->GetClientSize(&w, &h);

	SetSize(w, h);
}

wxDC *GraphView::GetDC() {
#ifdef __WXMSW__
	m_sdc.Deselect();
#endif
	return m_dc;
}

void GraphView::SetSize(int w, int h) {

#ifdef __WXGTK__
	if (m_gc) 
		gdk_gc_unref(m_gc);
#else
	m_sdc.Deselect();
	if (m_gc)
		DeleteDC(m_gc);
#endif
	delete m_dc;
	delete m_bmp;

	m_bmp = new wxBitmap(w, h);
	m_dc = new wxMemoryDC();

	wxMask *mask = new wxMask();
	mask->Create(wxBitmap(w, h, 1), *wxBLACK);
	m_bmp->SetMask(mask);
#ifdef __WXGTK__
	m_gdkbmp = m_bmp->GetMask()->GetBitmap();
	m_gc = gdk_gc_new(m_gdkbmp);
	m_sdc.SetObjects(m_gc, m_gdkbmp);
#else
	m_mswbmp = (HBITMAP) m_bmp->GetMask()->GetMaskBitmap();
	m_gc = CreateCompatibleDC(NULL);
	m_sdc.SetObjects(m_gc, m_mswbmp);
#endif

	m_dc->SetFont(m_graphs->GetFont());
	ClearDC(m_dc, m_bmp, w, h);

}

void GraphView::Detach(Draw *draw) {
	if (m_gc) 
#ifdef __WXGTK__
		gdk_gc_unref(m_gc);
#else
		DeleteDC(m_gc);
#endif
	m_gc = NULL;

	delete m_dc;
	m_dc = NULL;

	delete m_bmp;
	m_bmp = NULL;

	draw->DetachObserver(this);
	m_draw = NULL;
}

void GraphView::DrawInfoChanged(Draw *draw) {
	DrawAll();
}

void GraphView::PeriodChanged(Draw *draw, PeriodType period) {
	DrawAll();
}

void GraphView::ScreenMoved(Draw *draw, const wxDateTime& time) {
	DrawAll();
	m_graphs->Refresh();
}

void GraphView::Refresh() {
	DrawAll();
}

void GraphView::CurrentProbeChanged(Draw *draw, int pi, int ni, int d) {

	wxRegion reg;

	DrawCursor(pi, true, &reg);

	int i = d;
	if (i)
		i = i > 0 ? -1 : 1;

	if (m_draw->IsDoubleCursor() && ni >= 0) {
		int vd = m_draw->GetValuesTable().size();

		int s, e;

		if (d >= 0) {
			s = std::max(0, ni - d);
			e = ni;
		} else {
			s = ni;
			e = std::min(vd - 1, ni - d);
		}

		for (int i = s; i <= e; ++i) 
			DrawPoint(i, &reg, false);
	}

	DrawCursor(ni, false, &reg);

	m_graphs->UpdateArea(reg);

}

void GraphView::NewData(Draw *draw, int i) {

	wxRegion reg;

	DrawPoint(i, &reg);

	if (i == draw->GetCurrentIndex())
		DrawCursor(i, false, &reg);

	m_graphs->UpdateArea(reg);

}

bool GraphView::AlternateColor(int idx) {
	if (m_draw->GetSelected() == false)
		return false;

	if (m_draw->IsDoubleCursor() == false)
		return false;

	const Draw::VT& vt = m_draw->GetValuesTable();

	int ss = idx + vt.m_view.Start();

	bool result = (ss >= vt.m_stats.Start()) && (ss <= vt.m_stats.End());

	//wxLogInfo(_T("Alternate color for index: %d, stats start: %d, stats end:%d, result %d"), ss, vt.m_stats.Start(), vt.m_stats.End(), r);

	return result;

}

void GraphView::EnableChanged(Draw *draw) {
	m_graphs->Refresh();
}

void GraphView::FilterChanged(Draw *draw) {
	DrawAll();
	m_graphs->Refresh();
}

void GraphView::DrawDot(int x, int y, wxDC *dc, SDC *sdc, wxRegion* region) {
	dc->DrawPoint(x, y);
	m_sdc.DrawPoint(x, y);

	if (m_draw->GetPeriod() != PERIOD_T_DAY) {
		//dc->DrawCircle(x, y, wide ? 4 : 2);
		//m_sdc.DrawCircle(x, y, wide ? 4 : 2);

		dc->DrawCircle(x, y, 2);
		m_sdc.DrawCircle(x, y, 2);

		if (region) 
			region->Union(x - 2, y - 2, 5, 5); 
	}
}



void GraphView::DrawPoint(int index, wxRegion *region, bool maybe_repaint_cursor) {

	bool repaint_cursor = false;
	int current_index = 0;

	if (index < 0)
		return;

	if (!m_draw->GetValuesTable().at(index).IsData())
		return;

	wxDC* dc = m_dc;
	if (!dc->Ok())
		return;

	int x, y;

	GetPointPosition(dc, index, &x , &y);

	bool wide = m_draw->GetSelected() && m_draw->IsDoubleCursor();

	int line_width = wide ? 3 : 1;

	m_sdc.SetLineWidth(wide ? 3 : 1);
	m_sdc.SetWhiteForeground();	

	wxPen pen1(m_draw->GetDrawInfo()->GetDrawColor(), line_width, wxSOLID);
	wxPen pen2(alt_color, line_width, wxSOLID);
	dc->SetPen(pen1);

    	dc->SetBrush(wxBrush(wxColour(0,0,0), wxTRANSPARENT));

	int i = index;

	const Draw::VT& vt = m_draw->GetValuesTable();

	bool is_alternate = AlternateColor(index);

	if (i - 1 >= 0 && vt.at(i-1).IsData()) {
		int x1, y1;

		GetPointPosition(dc, i - 1, &x1, &y1);

		bool ac = AlternateColor(i - 1);

		if (ac && is_alternate)  {
			dc->SetPen(pen2);
		} else {
			dc->SetPen(pen1);
		}

		dc->DrawLine(x1, y1, x, y);
		m_sdc.DrawLine(x1, y1, x, y);

		if (ac)
			dc->SetPen(pen2);
		else
			dc->SetPen(pen1);

		DrawDot(x1, y1, dc, &m_sdc, region);

		if (region)
			region->Union(x1 , min(y, y1) - line_width, abs(x - x1) + 2 * line_width, abs(y - y1) + 2 * line_width);

		if (maybe_repaint_cursor && 
			m_draw->GetCurrentIndex() != -1  &&
			m_draw->GetCurrentIndex() == i) {

			repaint_cursor = true;
			current_index = i - 1;
		}

	}

	if ((i + 1) < (int)vt.size() && vt.at(i + 1).IsData()) {
		int x2, y2;

		bool ac = AlternateColor(i + 1);
		
		if (ac && is_alternate) {
			dc->SetPen(pen2);
		} else {
			dc->SetPen(pen1);
		}

		GetPointPosition(dc, i + 1, &x2 , &y2);

		dc->DrawLine(x, y, x2, y2);
		m_sdc.DrawLine(x, y, x2, y2);

		if (ac)
			dc->SetPen(pen2);
		else
			dc->SetPen(pen1);

		DrawDot(x2, y2, dc, &m_sdc, region);

		if (region)
			region->Union(x, min(y, y2) - line_width, abs(x - x2) + 2 * line_width, abs(y - y2) + 2 * line_width);

		if (maybe_repaint_cursor && 
			m_draw->GetCurrentIndex() != -1 &&
			m_draw->GetCurrentIndex() == i) {
			repaint_cursor = true;
			current_index = i + 1;
		}

	}
	

	if (is_alternate) 
		dc->SetPen(pen2);
	else
		dc->SetPen(pen1);

	DrawDot(x, y, dc, &m_sdc, region);

	m_sdc.SetLineWidth(1);
	pen1.SetWidth(1);
	pen2.SetWidth(1);


	if (repaint_cursor)
		DrawCursor(current_index, region);

	dc->SetPen(wxNullPen);
	dc->SetBrush(wxNullBrush);

}

void GraphView::DrawCursor(int index, bool clear, wxRegion *region) {

	if (index < 0)
		return;

	int x, y;
	wxDC* dc = m_dc;

	const Draw::VT& vt = m_draw->GetValuesTable();

	if (!vt.at(index).IsData())
		return;

	GetPointPosition(dc, index, &x, &y);

	wxRect rect(x - cursorrectanglesize / 2, 
			y - cursorrectanglesize / 2, 
			cursorrectanglesize,
			cursorrectanglesize
			);
	dc->SetBrush(wxBrush(wxColour(0,0,0), wxTRANSPARENT));

	if (!clear) {
		dc->SetPen(wxPen(*wxWHITE, 1, wxSOLID));
		wxLogInfo(_T("Drawing cursor at position %d , %d"), x, y);
		m_sdc.SetWhiteForeground();

		dc->DrawRectangle(rect);
		m_sdc.DrawRectangle(rect);

	} else {
		dc->SetPen(wxPen(*wxBLACK, 1, wxSOLID));
		wxLogInfo(_T("Clearing cursor at position %d , %d"), x, y);
		m_sdc.SetBlackForeground();

		dc->DrawRectangle(rect);
		m_sdc.DrawRectangle(rect);

		//repaint points damaged by cursor
		int x1, x2;

		x1 = GetIndex(x - cursorrectanglesize / 2);
		x2 = GetIndex(x + cursorrectanglesize / 2);

		for (int i = x1; i <= x2; i++) if (i > 0 && i < (int)vt.size())
			DrawPoint(i, region, false);
	}

	if (region)
		region->Union(rect);


	dc->SetPen(wxNullPen);
	dc->SetBrush(wxNullBrush);

}

void GraphView::DrawDeselected(Draw *draw) {
	if (m_draw->IsDoubleCursor()) {
		DrawAll();
		m_graphs->Refresh();
	}
}

void GraphView::DrawSelected(Draw *draw) {
	if (m_draw->IsDoubleCursor()) {
		DrawAll();
		m_graphs->Refresh();
	}
}

void GraphView::DrawAll() {
	m_dc->SetBrush(*wxTRANSPARENT_BRUSH);
	m_dc->Clear();
	m_sdc.Clear();

	int line_width = (m_draw->GetSelected() && m_draw->IsDoubleCursor()) ? 3 : 1;

	DrawAllPoints(m_dc, &m_sdc, line_width);

	DrawCursor(m_draw->GetCurrentIndex(), false);
}


GraphDrawer::GraphDrawer(int circle_radius) : 
	m_circle_radius(circle_radius) {}


void GraphDrawer::DrawAllPoints(wxDC *dc, SDC *sdc, int point_width) {

	if (!dc->Ok())
		return;

	if (m_draw->GetDrawInfo() == NULL)
		return;

	bool prev_no_data = true;

	wxPen pen1;
	wxPen pen2;
	pen1.SetStyle(wxSOLID);
	pen2.SetStyle(wxSOLID);
	pen1.SetWidth(point_width);
	pen2.SetWidth(point_width);
	pen1.SetColour(m_draw->GetDrawInfo()->GetDrawColor());
	pen2.SetColour(alt_color);

	dc->SetPen(pen1);

	if (sdc) {
		sdc->SetWhiteForeground();
		sdc->SetLineWidth(point_width);
	}

	const Draw::VT& vt = m_draw->GetValuesTable();

	bool is_alt = false;

	for (size_t i = 0; i < vt.size(); i++) {

		if (!vt.at(i).IsData()) {
			prev_no_data = true;
			continue;
		}

		int x, y;
		GetPointPosition(dc, i, &x, &y);

		bool pa = AlternateColor(i);

		if (!prev_no_data) {
			int x1, y1;

			if (is_alt && pa)
				dc->SetPen(pen2);
			else
				dc->SetPen(pen1);

			GetPointPosition(dc, i - 1, &x1, &y1);
			dc->DrawLine(x1, y1, x, y);
			if (sdc)
				sdc->DrawLine(x1, y1, x, y);

			if (is_alt)
				dc->SetPen(pen2);
			else
				dc->SetPen(pen1);

			dc->DrawPoint(x1, y1);
			if (sdc)
				sdc->DrawPoint(x1, y1);

			if (m_draw->GetPeriod() != PERIOD_T_DAY) {
				dc->DrawCircle(x1, y1, m_circle_radius);
				if (sdc)
					sdc->DrawCircle(x1, y1, m_circle_radius);
			}

		}


		if ((i + 1) >= vt.size() || !vt.at(i + 1).IsData()) {
			if (pa) 
				dc->SetPen(pen2);
			else
				dc->SetPen(pen1);

			dc->DrawPoint(x, y);
			if (sdc)
				sdc->DrawPoint(x, y);

			if (m_draw->GetPeriod() != PERIOD_T_DAY) {
				dc->DrawCircle(x, y, m_circle_radius);
				if (sdc)
					sdc->DrawCircle(x, y, m_circle_radius);
			}
		}

		is_alt = pa;

		prev_no_data = false;
	}

	dc->SetBrush(wxNullBrush);
	dc->SetPen(wxNullPen);
	if (sdc)
		sdc->SetLineWidth(1);

}

int GraphView::GetIndex(int x) const {
	int w, h;
	GetSize(&w, &h);
    
	x -= m_leftmargin;
	w -= m_leftmargin + m_rightmargin;
    
	int index = x * m_draw->GetValuesTable().size() / w;

	return index;

}

void GraphView::GetDistance(int x, int y, double &d, wxDateTime &time) const {
	d = INFINITY;
	time = wxInvalidDateTime;
	/* this defines search radius for 'direct hit' */

	int w,h;
	m_dc->GetSize(&w, &h);
	
	/* forget about top and bottom margins */
	h -= m_bottommargin + m_topmargin;
	/* get X coordinate */
	int index = GetIndex(x);

	const Draw::VT& vt = m_draw->GetValuesTable();
    
	int i, j;
	for (i = j = index; i >= 0 || j < (int)vt.size(); --i, ++j) {
		double d1 = INFINITY;
		double d2 = INFINITY;

		if (i >= 0 && i < (int)vt.size() && vt.at(i).IsData()) {
			int xi, yi;
			GetPointPosition(m_dc, i, &xi, &yi);
			d1 = (x - xi) * (x - xi) + (y - yi) * (y - yi);
		}

		if (j >= 0 && j < (int)vt.size() && vt.at(j).IsData()) {
			int xj, yj;
			GetPointPosition(m_dc, j, &xj, &yj);
			d1 = (x - xj) * (x - xj) + (y - yj) * (y - yj);
		}

		if (!std::isfinite(d1) && !std::isfinite(d1))
			continue;

		if (!std::isfinite(d2) || d1 < d2) {
			d = d1;
			time = m_draw->GetTimeOfIndex(i);
			wxLogInfo(_T("Nearest index %d, time: %s"), i, time.Format().c_str());
		} else {
			d = d2;
			time = m_draw->GetTimeOfIndex(j);
			wxLogInfo(_T("Nearest index %d, time: %s"), j, time.Format().c_str());
		}

		break;

	}

}

void GraphView::GetSize(int *w, int *h) const {
	m_dc->GetSize(w, h);
}

GraphView::~GraphView() {

	if (m_gc) 
#ifdef __WXGTK__
		gdk_gc_unref(m_gc);
#else
		DeleteDC(m_gc);
#endif

	delete m_dc;
	delete m_bmp;
}

SDC::SDC() {
	m_gc = NULL;
#ifdef __WXGTK__
	m_gdkbmp = NULL;
#else
	m_mswbmp = NULL;
	m_selected = false;
#endif
}


#ifdef __WXGTK__
void SDC::SetObjects(GdkGC *gc, GdkBitmap *bmp) {
	m_gc = gc;
	m_gdkbmp = bmp;
	SetBlackForeground();
	gdk_gc_set_fill(m_gc, GDK_SOLID);
}
#else
void SDC::SetObjects(HDC gc, HBITMAP bmp) {
	Deselect();

	m_gc = gc;
	m_mswbmp = bmp;

	SelectObject(m_gc, GetStockObject(NULL_BRUSH));

	Select();
}
#endif

void SDC::SetWhiteForeground() {
#ifdef __WXGTK__
	GdkColor gcolor;
       	gcolor.red = gcolor.green = gcolor.blue = 65000;
	gcolor.pixel = 1;
	gdk_gc_set_foreground(m_gc, &gcolor);
#else
	int width = m_pen.GetWidth();
	wxPen npen = wxPen(*wxWHITE, width, wxSOLID);	
	(HPEN) SelectObject(m_gc, (HPEN)npen.GetResourceHandle());
	m_pen = npen;
#endif
}


void SDC::SetBlackForeground() {
#ifdef __WXGTK__
	GdkColor gcolor;
       	gcolor.red = gcolor.green = gcolor.blue = 0;
	gcolor.pixel = 0;
	gdk_gc_set_foreground(m_gc, &gcolor);
#else
	int width = m_pen.GetWidth();
	wxPen npen = wxPen(*wxBLACK, width, wxSOLID);	
	(HPEN) SelectObject(m_gc, (HPEN)npen.GetResourceHandle());
	m_pen = npen;
#endif
}

void SDC::DrawPoint(int x, int y) {
#ifdef __WXGTK__
	gdk_draw_point(m_gdkbmp, m_gc, x, y);
#else
	Select();
	LOGPEN lp;
	GetObject((HPEN)m_pen.GetResourceHandle(), sizeof(lp), &lp);
	SetPixel(m_gc, x, y, lp.lopnColor);
#endif
}

void SDC::DrawCircle(int x, int y, int r) {
#ifdef __WXGTK__
	gdk_draw_arc(m_gdkbmp, m_gc, FALSE, x - r, y - r, 2 * r, 2 * r, 0, 360 * 64);
#else
	Select();
	Ellipse(m_gc, x - r, y - r, x + r, y + r);
#endif
}

void SDC::DrawLine(int x1, int y1, int x2, int y2) {
#ifdef __WXGTK__
	gdk_draw_line(m_gdkbmp, m_gc, x1, y1, x2, y2);
#else
	Select();
	MoveToEx(m_gc, x1, y1, NULL); 
	LineTo(m_gc, x2, y2);
#endif
}

void SDC::DrawRectangle(int x, int y, int w, int h) {
#ifdef __WXGTK__
	gdk_draw_rectangle(m_gdkbmp, m_gc, FALSE, x, y, w - 1, h - 1);
#else
	Select();
	Rectangle(m_gc, x, y, x + w, y + h);
#endif
}

void SDC::DrawRectangle(const wxRect& rect) {
	DrawRectangle(rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight());
}

void SDC::Clear() {
	SetBlackForeground();
#ifdef __WXGTK__
	int w, h;
	gdk_drawable_get_size(m_gdkbmp, &w, &h);
	gdk_draw_rectangle(m_gdkbmp, m_gc, TRUE, 0, 0, w, h);
#else
	Select();
	BITMAP bi;
	GetObject(m_mswbmp, sizeof(bi), &bi);

	HBRUSH brush = CreateSolidBrush(0x0);

	RECT rect;
	rect.left = rect.top = 0;
	rect.right = bi.bmWidth;
	rect.bottom = bi.bmHeight;


	FillRect(m_gc, &rect, brush);

	DeleteObject(brush);
#endif
}

void SDC::SetLineWidth(int width) {
#ifdef __WXMSW__
	wxColour c = m_pen.GetColour();
	wxPen npen = wxPen(c, width == 1 ? 0 : width, wxSOLID);	
	(HPEN) SelectObject(m_gc, (HPEN)npen.GetResourceHandle());
	m_pen = npen;
#else
	GdkGCValues vals;
	gdk_gc_get_values(m_gc, &vals);
	vals.line_width = width == 1 ? 0 : width;
	gdk_gc_set_values(m_gc, &vals, GDK_GC_LINE_WIDTH);
#endif
}

#ifdef __WXMSW__
void SDC::Select() {
	if (m_selected)
		return;
	m_defbmp = (HBITMAP) SelectObject(m_gc, m_mswbmp);

	m_selected = true;
}

void SDC::Deselect() {
	if (!m_selected)
		return;

	m_mswbmp = (HBITMAP) SelectObject(m_gc, m_defbmp);
	m_selected = false;
}
#endif
