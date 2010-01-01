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
#include "szhlpctrl.h"

#include "ids.h"
#include "classes.h"
#include "coobs.h"
#include "drawtime.h"
#include "drawobs.h"
#include "drawpnl.h"
#include "summwin.h"
#include "dbinquirer.h"
#include "database.h"
#include "draw.h"
#include "cconv.h"
#include "timeformat.h"
#include "coobs.h"
#include "drawsctrl.h"
#include "drawpnl.h"
#include "cfgmgr.h"

#include <algorithm>
#include <wx/statline.h>


//BEGIN_EVENT_TABLE(SummaryWindow, wxFrame)
BEGIN_EVENT_TABLE(SummaryWindow, wxDialog)
    EVT_IDLE(SummaryWindow::OnIdle)
    EVT_BUTTON(wxID_HELP, SummaryWindow::OnHelpButton)
    EVT_CLOSE(SummaryWindow::OnClose)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(TTLabel, wxWindow)
	EVT_PAINT(TTLabel::OnPaint)
END_EVENT_TABLE()

TTLabel::TTLabel(wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size) :
		wxWindow(parent, id, pos, size, wxFULL_REPAINT_ON_RESIZE)
{}

wxSize TTLabel::DoGetBestSize() const {
	if (value.IsEmpty())
		return wxSize(0, 0);

	wxClientDC dc(const_cast<TTLabel*>(this));
	dc.SetFont(GetFont());
	int w1, h1;
	dc.GetTextExtent(value, &w1, &h1);

	int w2, h2;
	dc.GetTextExtent(unit, &w2, &h2);

	return wxSize(w1 + w2 + 6, std::max(h1, h2) + 4);
}

void TTLabel::SetValueText(const wxString &value) {
	this->value = value;
	Refresh();
}

void TTLabel::SetUnitText(const wxString &unit) {
	this->unit = unit;
	Refresh();
}

void TTLabel::OnPaint(wxPaintEvent &event) {

	wxPaintDC dc(this);
#ifdef __WXGTK__
#endif
	dc.SetBackground(GetBackgroundColour());
	dc.Clear();

	dc.SetFont(GetFont());
	dc.SetTextForeground(GetForegroundColour());

	int w, h, tw, th;

	GetClientSize(&w, &h);

	dc.GetTextExtent(value, &tw, &th);

	dc.DrawText(value, w - tw - 2, h / 2 - th / 2);

	dc.GetTextExtent(unit, &tw, &th);

	dc.DrawText(unit, 2, h / 2 - th / 2);

}


SummaryWindow::ObservedDraw::ObservedDraw(Draw *_draw) : draw(_draw), 
	update(false), tooltip(false), hoursum(false)
{}

SummaryWindow::SummaryWindow(DrawPanel *draw_panel, wxWindow *parent) : 
	wxDialog(parent, drawID_SUMMWIN, _("Summary values"), wxDefaultPosition, wxSize(100, 100), 
			wxDEFAULT_DIALOG_STYLE),
	m_summary_draws_count(0),
	m_update(false),
	m_active(false),
	m_tooltip(false)
{
	SetHelpText(_T("draw3-ext-summaryvalues"));

	m_draws.SetCount(MAX_DRAWS_COUNT, NULL);
	wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	wxStaticText *txt = new wxStaticText(this, wxID_ANY, wxString(_("Summary values")) + _T(":"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	sizer->Add(txt, 0, wxALIGN_CENTER | wxALL, 5);
	sizer->Add(new wxStaticLine(this, wxID_ANY), 0, wxEXPAND);

	for (size_t i = 0; i <= MAX_DRAWS_COUNT; ++i) {
		TTLabel *label = new TTLabel(this, wxID_ANY);
		sizer->Add(label, 0, wxEXPAND);
		m_labels.Add(label);
		label->Show(false);
		sizer->Show(label, false, true);

		wxStaticLine *line = new wxStaticLine(this, wxID_ANY);
		m_lines.Add(line);
		sizer->Add(line, 0, wxEXPAND);
		line->Show(false);
		sizer->Show(line, false, true);

	}

	m_labels[MAX_DRAWS_COUNT]->Show(true);
	m_labels[MAX_DRAWS_COUNT]->SetForegroundColour(*wxBLACK);
	m_labels[MAX_DRAWS_COUNT]->SetValueText(_("This set does not contain draws for which summary values are calculated."));
	sizer->Show(m_labels[MAX_DRAWS_COUNT], true, true);

	sizer->Add(new wxStaticLine(this, wxID_ANY), 0, wxEXPAND);
	sizer->Add(new wxButton(this, wxID_HELP), 0, wxALIGN_CENTER | wxALL, 5);

	sizer->Fit(this);

	this->draw_panel = draw_panel;

	this->draws_controller = NULL;
}

void SummaryWindow::OnIdle(wxIdleEvent &event) {

	if (!m_active)
		return;

	bool resize = false;

	wxSizer* sizer = GetSizer();

#define SHOW(wdg, show) \
	wdg->Show(show); \
	sizer->Show(wdg, show, true);


	if (m_update) {
		if (m_summary_draws_count == 0) {
			for (int i = 0; i < MAX_DRAWS_COUNT; ++i) {
				SHOW(m_labels[i], false);
				SHOW(m_lines[i], false);
			}
			SHOW(m_labels[MAX_DRAWS_COUNT], true)
			resize = true;
		} else { 
			for (size_t i = 0; i < m_draws.Count(); ++i) {
	
				ObservedDraw* od = m_draws[i];
				if (od == NULL)
					continue;
	
				if (!od->update)
					continue;
	
				TTLabel *l = m_labels[i];
				wxStaticLine *li = m_lines[i];
	
				double val = od->draw->GetValuesTable().m_hsum;
				double data_percentage = od->draw->GetValuesTable().m_data_probes_ratio;
	
				if (std::isnan(val) && l->IsShown()) {
					SHOW(l, false);
					SHOW(li, false);

					resize = true;
					continue;
				}
		
				if (od->draw->GetEnable() == false) {
					SHOW(l, false);
					SHOW(li, false);

					resize = true;
					continue;
				}
		
				if (!std::isnan(val) && !l->IsShown()) {
					SHOW(l, true);
					SHOW(li, true);

					l->SetBackgroundColour(od->draw->GetDrawInfo()->GetDrawColor());
					resize = true;
		
					od->tooltip = true;
					m_tooltip = true;
				}
		
				od->update = false;
		
				if (l->IsShown() == false)
					continue;
		
				Draw* draw = od->draw;
				assert(draw);
				DrawInfo* info = draw->GetDrawInfo();
				assert(info);

				//convert unit, idea taken from draw2 :)
				wxString sunit = info->GetSumUnit();
				wxString unit = info->GetUnit();
				wxString text;
				if (!sunit.IsEmpty()) {
					text = wxString(info->GetValueStr(val, _T(""))) + _T(" ") + sunit;
				} else if (unit == _T("kW")) {
					text = wxString(info->GetValueStr(val, _T(""))) + _T(" ") + _T("kWh") + 
						_T(" (") + wxString(info->GetValueStr(val * 3.6 / 1000, _T(""))) + _T(" GJ)");
				} else if (unit == _T("MW")) {
					text = wxString(info->GetValueStr(val, _T(""))) + _T(" ") + _T("MWh") + 
						_T(" (") + wxString(info->GetValueStr(val * 3.6, _T(""))) + _T(" GJ)");
				} else if (unit.Replace(_T("/h"), _T("")) == 0) {
					unit += _T("*h");
					text = wxString(info->GetValueStr(val, _T(""))) + _T(" ") + unit;
				} else {
					text = wxString(info->GetValueStr(val, _T(""))) + _T(" ") + unit;
				}
		
				if (!std::isnan(data_percentage) && data_percentage < 0.99)
					text += wxString::Format(_T(" (%.0f%%)"), data_percentage * 100);
		
				l->SetValueText(text);
		
				resize = true;

			}
			if (m_labels[MAX_DRAWS_COUNT]->IsShown())
				SHOW(m_labels[MAX_DRAWS_COUNT], false);
	
		}
	}

	if (m_tooltip) for (size_t i = 0; i < m_draws.Count(); ++i) {
		ObservedDraw* od = m_draws[i];
		if (od == NULL)
			continue;

		if (od->tooltip == false)
			continue;

		Draw* draw = od->draw;
		DrawInfo* info = draw->GetDrawInfo();

		wxString tooltip = _T("");
		tooltip += info->GetName();
		tooltip += _T("\n");

		PeriodType period = draw->GetPeriod();

		const Draw::VT& vt = draw->GetValuesTable();

		wxDateTime s = draw->GetTimeOfIndex(vt.m_stats.Start() - vt.m_view.Start());
		wxDateTime e = draw->GetTimeOfIndex(vt.m_stats.End() - vt.m_view.Start());

		tooltip += _("From:"); 
		tooltip += _T("\n") + FormatTime(s, period) + _T("\n") + _("To:") + _T("\n") + FormatTime(e, period);

		double data_percentage = od->draw->GetValuesTable().m_data_probes_ratio;
		if (std::isnan(data_percentage))
			data_percentage = 0;
		tooltip += _T("\n\n") + wxString::Format(_("Data contains %.0f%% probes from current period"), data_percentage * 100);

		m_labels[i]->SetToolTip(tooltip);

		od->tooltip = false;
	}

	if (resize) 
		Resize();

	m_update = m_tooltip = false;

}

void SummaryWindow::Activate() {

	if (m_active)
		return;

	draws_controller->AttachObserver(this);

	m_active = true;

       	for (size_t i = 0; i < draws_controller->GetDrawsCount(); ++i)
		SetDraw(draws_controller->GetDraw(i));

}

void SummaryWindow::Resize() {
	GetSizer()->Layout();
	GetSizer()->Fit(this);
}
	
void SummaryWindow::Deactivate() {
	if (m_active == false) 
		return;

	draws_controller->DetachObserver(this);

       	for (size_t i = 0; i < m_draws.Count(); ++i) {
		ObservedDraw* od = m_draws[i];
		if (od != NULL) {
			ResetDraw(draws_controller->GetDraw(i));
		}
	}

	m_active = false;

	if (m_summary_draws_count != 0) 
		wxLogError(_T("Something is wrong with number of 'proper draws' in SummaryWindow"));

}

void SummaryWindow::StopDisplaying(int no) {
	assert(m_draws[no]);

	if (m_draws[no]->hoursum) {
		m_summary_draws_count--;
		assert(m_summary_draws_count >= 0);
	}

	m_lines[no]->Show(false);
	m_labels[no]->Show(false);

	m_update = true;
}

void SummaryWindow::ResetDraw(Draw *draw) {
	int no = draw->GetDrawNo();
	if (m_draws[no] == NULL)
		return;

	if (m_active)
		StopDisplaying(no);

	delete m_draws[no];
	m_draws[no] = NULL;

}

void SummaryWindow::Detach(DrawsController *draws_controller) {
	DrawObserver::Detach(draws_controller);
}

void SummaryWindow::StartDisplaying(int no) {
	assert(m_active);
	Draw* draw = m_draws[no]->draw;

	m_draws[no]->hoursum = draw->GetDrawInfo()->GetSpecial() == TDraw::HOURSUM; 

	if (m_draws[no]->hoursum) {
		m_labels[no]->SetUnitText(draw->GetDrawInfo()->GetShortName() + _T(":"));
		m_draws[no]->update = true;
		m_draws[no]->tooltip = true;
		m_summary_draws_count++;
	} else
		m_labels[no]->SetUnitText(_T(""));

	m_tooltip = true;
	m_update = true;

}

void SummaryWindow::SetDraw(Draw *draw) {
	int no = draw->GetDrawNo();

	assert(m_draws[no] == NULL);

	if (draw->GetDrawInfo() == NULL)
		return;

	m_draws[no] = new ObservedDraw(draw);

	if (m_active)
		StartDisplaying(no);

}

void SummaryWindow::Attach(DrawsController *draws_controller) {
	this->draws_controller = draws_controller;
}

void SummaryWindow::DrawInfoChanged(Draw *draw) {
	ResetDraw(draw);
	SetDraw(draw);
}

void SummaryWindow::UpdateDraw(Draw *draw) {
	int no = draw->GetDrawNo();

	ObservedDraw *od = m_draws[no];

	if (od == NULL)
		return;

	if (od->hoursum == false)
		return;

	m_update = od->update = true;
	m_tooltip = od->tooltip = true;
}

void SummaryWindow::StatsChanged(Draw *draw) {
	UpdateDraw(draw);
}

void SummaryWindow::DoubleCursorChanged(DrawsController *draws_controller) {
	for (size_t i = 0; i < draws_controller->GetDrawsCount(); i++)
		UpdateDraw(draws_controller->GetDraw(i));
}

void SummaryWindow::NumberOfValuesChanged(DrawsController *draws_controller) {
	DoubleCursorChanged(draws_controller);
}

void SummaryWindow::PeriodChanged(Draw *draw, PeriodType period) {
	UpdateDraw(draw);
}

void SummaryWindow::EnableChanged(Draw *draw) {
	int no = draw->GetDrawNo();

	ObservedDraw *od = m_draws[no];

	assert(od);

	if (od->hoursum)
		m_update = od->update = true;
}

void SummaryWindow::OnClose(wxCloseEvent &event) {
	if (event.CanVeto()) {
		event.Veto();
		draw_panel->ShowSummaryWindow(false);
	} else
		Destroy();
}

SummaryWindow::~SummaryWindow() { 
	for (size_t i = 0; i < m_draws.size(); ++i)
		delete m_draws[i];
}

void SummaryWindow::OnHelpButton(wxCommandEvent &event) {
	wxHelpProvider::Get()->ShowHelp(this);
}

bool SummaryWindow::Show(bool show) {
	if (show == IsShown())
		return false;

	if (show) 
		Activate();
	else
		Deactivate();

#if 0
	bool ret = wxFrame::Show(show);
#else
	bool ret = wxDialog::Show(show);
#endif
	if (ret)
		Resize();

	return ret;
}


