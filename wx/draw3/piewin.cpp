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


#include <wx/statline.h>
#include <algorithm>
#include <sstream>
#include <wx/dcbuffer.h>

#include "cconv.h"

#include "szhlpctrl.h"

#include "ids.h"
#include "classes.h"
#include "cfgmgr.h"
#include "coobs.h"

#include "drawobs.h"

#include "drawtime.h"
#include "database.h"
#include "draw.h"
#include "dbinquirer.h"
#include "drawsctrl.h"

#include "timeformat.h"
#include "drawpnl.h"
#include "piewin.h"

using std::max;


BEGIN_EVENT_TABLE(PieWindow::GraphControl, wxControl)
    EVT_PAINT(PieWindow::GraphControl::OnPaint)
END_EVENT_TABLE()

PieWindow::GraphControl::GraphControl(PieWindow *pie_window) : wxControl(pie_window, wxID_ANY), m_pie_window(pie_window) {}

void PieWindow::GraphControl::SetBestSize(int w, int h) {
	m_best_size = wxSize(w, h);
}

void PieWindow::GraphControl::OnPaint(wxPaintEvent &event) {
	wxBufferedPaintDC dc(this);
	m_pie_window->PaintGraphControl(dc);
}

BEGIN_EVENT_TABLE(PieWindow, wxFrame)
    EVT_IDLE(PieWindow::OnIdle)
    EVT_BUTTON(wxID_HELP, PieWindow::OnHelpButton)
    EVT_CLOSE(PieWindow::OnClose)
END_EVENT_TABLE()

PieWindow::ObservedDraw::ObservedDraw(Draw *_draw) : draw(_draw), 
	piedraw(false)
{}

PieWindow::PieWindow(wxWindow *parent, DrawPanel *panel) :
	wxFrame(parent, wxID_ANY, _("Percentage distribution"), wxDefaultPosition, wxSize(400, 100)),
	m_update(false),
	m_active(false),
	m_panel(panel),
	m_proper_draws_count(0)
{
	SetHelpText(_T("draw3-percentagedistribution"));

	m_graph = new GraphControl(this);
	m_draws.SetCount(MAX_DRAWS_COUNT, NULL);
	wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(new wxStaticText(this, wxID_ANY,_("Average percentage distribution")), 0, wxALIGN_CENTER);
	sizer->Add(new wxStaticLine(this), 0, wxEXPAND);
	sizer->Add(m_graph, 1, wxEXPAND);
	sizer->Add(new wxStaticLine(this), 0, wxEXPAND);
	sizer->Add(new wxButton(this, wxID_HELP), 0, wxALIGN_CENTER | wxALL, 5);

	SetBackgroundColour(DRAW3_BG_COLOR);

	SetSizer(sizer);

}

void PieWindow::OnIdle(wxIdleEvent &event) {

	if (!m_active)
		return;

	if (!m_update)
		return;

	if (m_requested_size.GetWidth() > 0 
			&& m_requested_size.GetHeight() > 0) {
		m_graph->SetBestSize(m_requested_size.GetWidth(), 
				m_requested_size.GetHeight());
		GetSizer()->Fit(this);
		m_requested_size = wxSize();
	}

	m_graph->Refresh();

	m_update = false;
}

void PieWindow::PaintGraphControl(wxDC &dc) {
	dc.Clear();
	
	const int top_margin = 5;

	wxFont f = GetFont();
#ifdef __WXGTK__
	f.SetPointSize(f.GetPointSize() - 1);
#endif
	dc.SetFont(f);

	int width, height;

	double sum = 0.0;

	wxString unit;

	m_graph->GetClientSize(&width, &height);

	int radius = wxMin(width, height) / 3;
	int prec = 0;

	std::vector<int> to_paint;

	for (size_t i = 0; i < m_draws.Count(); ++i) {
		ObservedDraw* od = m_draws[i];
		if (od == NULL)
			continue;

		if (od->piedraw == false)
			continue;

		Draw* draw = od->draw;
		DrawInfo* info = draw->GetDrawInfo();
		assert(info);

		const ValuesTable& vt = draw->GetValuesTable();
		if (vt.m_count == 0) 
			continue;	

		prec = info->GetPrec();
		if (vt.m_sum < prec)
			continue;

		to_paint.push_back(i);
		sum += vt.m_sum;

		unit = info->GetUnit();

	}

	double start = 0;

	for (size_t i = 0; i < to_paint.size(); ++i) {
		ObservedDraw* od = m_draws[to_paint[i]];
		Draw* draw = od->draw;
		const ValuesTable& vt = draw->GetValuesTable();

		DrawInfo* info = draw->GetDrawInfo();
		wxColour color = info->GetDrawColor();
		
		dc.SetBrush(wxBrush(color));
		dc.SetPen(wxPen(color));

		double end;

		if (i < to_paint.size() - 1)
			end = start + vt.m_sum / sum * 360;
		else 
			end = 360;

		dc.DrawEllipticArc(width / 2 - radius,
				height / 2 - radius,
				2 * radius,
				2 * radius,
				start, 
				end);

		//fill the missing line
		if (end == 360)
			dc.DrawLine(width / 2, height / 2, width / 2 + radius + 1, height / 2);

		double mid = (start + end) / 2;
		double rmid = mid / 360 * 2 * M_PI;

		std::wstringstream wss;
		wss.precision(2);
		wss << (100 * vt.m_sum / sum);
		wss << _T(" %");
		wxString text = wss.str();

		int textw, texth;
		dc.GetTextExtent(text, &textw, &texth);

		int textx, texty;
		textx = int((radius + 5) * cos(rmid));
		texty = - int((radius + 5) * sin(rmid));

		textx += width / 2;
		texty += height / 2;


		const double margin = M_PI_2 / 36;

		if (rmid > - margin && rmid < margin) {
			texty -= texth / 2 + 2;
			textx += 2;
		} else if (rmid > M_PI_2 - margin && rmid < M_PI_2 + margin)  {
			texty -= texth + 2;
			textx -= textw / 2 + 2;
		} else if (rmid > 2 * M_PI_2 - margin && rmid < 2 * M_PI_2 + margin) {
			textx -= textw + 2;
			texty -= texth / 2 + 2;	
		} else if (rmid > 3 * M_PI_2 - margin && rmid < 3 * M_PI_2 + margin) {
			textx -= textw / 2 + 2;
			texty += 2;
		} else if (rmid < M_PI_2) {
			texty -= texth + 2;
			textw += 2;
		} else if (rmid < M_PI) {
			texty -= texth + 2;
			textx -= textw + 2;
		} else if (rmid < 3 * M_PI_2) {
			textx -= textw + 2;
			texty += 2 ;
		} else {
			textx += 2;
			texty += 2;
		}

		dc.DrawText(text, textx, texty);

		start = end;

	}

	int textw, texth;
	if (to_paint.size()) {
		wxString sum_text = wxString::Format(_("Sum: %.*f %s"), prec, sum, unit.c_str());
		dc.GetTextExtent(sum_text, &textw, &texth);
		dc.DrawText(sum_text, (width - textw) / 2, dc.MaxY() + 5);
		if (dc.MinX() <= 5 || dc.MaxX() > width - 5
				|| (radius * 2 + 4 * texth + top_margin + 10 > height)) 
			m_requested_size = wxSize(wxMax(width, dc.MaxX() - dc.MinX() + 20), 
				wxMax(height, (radius * 2 + 4 * texth + top_margin + 10)));
	} else {
		wxString no;
		if (m_proper_draws_count == 0)
			no = _("There are no draws in this sets for which percentage distribution is calculated.");
		else
			no = _("All values equal zero.");
		dc.GetTextExtent(no, &textw, &texth);
		dc.DrawText(no, (width - textw) / 2, dc.MaxY() + 5);
		int w, h;
		GetSize(&w, &h);
		int dw = wxMax(textw + 40, 400);
		if (w < dw || h < 100)
			m_requested_size = wxSize(dw, 100);
	}

}

void PieWindow::Activate() {
	if (m_active)
		return;

	m_active = true;
	m_update = true;

	m_draws_controller->AttachObserver(this);

       	for (size_t i = 0; i < m_draws_controller->GetDrawsCount(); ++i) {
		SetDraw(m_draws_controller->GetDraw(i));
		ObservedDraw* od = m_draws[i];
		if (od && od->piedraw) {
			if (od->piedraw)
				m_proper_draws_count++;
		}
	}

}

void PieWindow::Deactivate() {
	if (m_active == false) 
		return;

	m_active = false;
	
	m_draws_controller->DetachObserver(this);

       	for (size_t i = 0; i < m_draws.Count(); ++i) {
		ObservedDraw* od = m_draws[i];
		if (od != NULL) {
			ResetDraw(od->draw);	
		}
	}

}

void PieWindow::SetDraw(Draw *draw) {
	int no = draw->GetDrawNo();

	assert(m_draws[no] == NULL);

	if (draw->GetDrawInfo() == NULL)
		return;

	m_draws[no] = new ObservedDraw(draw);
	m_draws[no]->piedraw = draw->GetDrawInfo()->GetSpecial() == TDraw::PIEDRAW; 

	if (m_draws[no]->piedraw)
		m_proper_draws_count++;
	m_update = true;

}

void PieWindow::ResetDraw(Draw *draw) {
	assert(draw);

	int no = draw->GetDrawNo();

	if (m_draws[no] == NULL)
		return;

	if (m_draws[no]->piedraw)
		m_proper_draws_count--;

	delete m_draws[no];
	m_draws[no] = NULL;

	m_update = true;
}

void PieWindow::Attach(DrawsController *draw_ctrl) {
	m_draws_controller = draw_ctrl;
}

void PieWindow::Detach(DrawsController *draw_ctrl) {
	DrawObserver::Detach(draw_ctrl);
}

void PieWindow::DrawInfoChanged(Draw *draw) {
	ResetDraw(draw);
	SetDraw(draw);
}

void PieWindow::UpdateDraw(Draw *draw) {
	int no = draw->GetDrawNo();

	ObservedDraw *od = m_draws[no];

	if (od == NULL)
		return;

	if (od->piedraw == false)
		return;

	m_update = true;
}

void PieWindow::DoubleCursorChanged(DrawsController *draws_controller) {
	for (size_t i = 0; i < draws_controller->GetDrawsCount(); i++)
		UpdateDraw(draws_controller->GetDraw(i));
}

void PieWindow::StatsChanged(Draw *draw) {
	UpdateDraw(draw);
}

void PieWindow::PeriodChanged(Draw *draw, PeriodType period) {
	UpdateDraw(draw);
}

void PieWindow::EnableChanged(Draw *draw) {
	int no = draw->GetDrawNo();

	ObservedDraw *od = m_draws[no];

	assert(od);

	m_update = true;
}

void PieWindow::OnClose(wxCloseEvent &event) {
	if (event.CanVeto()) {
		event.Veto();
		m_panel->ShowPieWindow(false);
	} else
		Destroy();
}

PieWindow::~PieWindow() { 
	for (size_t i = 0; i < m_draws.size(); ++i)
		delete m_draws[i];
}

void PieWindow::OnHelpButton(wxCommandEvent &event) {
	wxHelpProvider::Get()->ShowHelp(this);
}

bool PieWindow::Show(bool show) {
	if (show == IsShown())
		return false;

	if (show) 
		Activate();
	else
		Deactivate();

	bool ret = wxFrame::Show(show);

	return ret;
}


