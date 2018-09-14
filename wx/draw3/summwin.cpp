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


BEGIN_EVENT_TABLE(SummaryWindow, wxFrame)
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
	wxColour bg = GetBackgroundColour();

	dc.SetBackground(bg);
	dc.Clear();

	dc.SetFont(GetFont());

	/*  Note: This algorithm is taken from a formula for converting RGB
	 *  to YIQ values. Brightness value gives a perceived brightness for
	 *  a color. (por. infowdg.cpp) */
	int brightness = ((bg.Red() * 299) + (bg.Green() * 587) + (bg.Blue() * 114)) / 1000;

	if (brightness >= 100)
		dc.SetTextForeground(GetForegroundColour());
	else
		dc.SetTextForeground(*wxWHITE);

	int w, h, tw, th;

	GetClientSize(&w, &h);

	dc.GetTextExtent(value, &tw, &th);

	dc.DrawText(value, w - tw - 2, h / 2 - th / 2);

	dc.GetTextExtent(unit, &tw, &th);

	dc.DrawText(unit, 2, h / 2 - th / 2);

}


SummaryWindow::ObservedDraw::ObservedDraw(Draw *_draw) : draw(_draw), 
	update(false), tooltip(false)
{}

SummaryWindow::SummaryWindow(DrawPanel *draw_panel, wxWindow *parent) : 
	wxFrame(NULL, drawID_SUMMWIN, _("Summary values"), wxDefaultPosition, wxSize(100, 100), 
			wxDEFAULT_DIALOG_STYLE | wxDIALOG_NO_PARENT ),
	m_summary_draws_count(0),
	m_update(false),
	m_active(false),
	m_tooltip(false)
{
	SetHelpText(_T("draw3-ext-summaryvalues"));

	wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	wxStaticText *txt = new wxStaticText(this, wxID_ANY, wxString(_("Summary values")) + _T(":"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	sizer->Add(txt, 0, wxALIGN_CENTER | wxALL, 5);
	sizer->Add(new wxStaticLine(this, wxID_ANY), 0, wxEXPAND);

	m_values_sizer = new wxBoxSizer(wxVERTICAL);

	m_no_draws_label = new TTLabel(this, wxID_ANY);
	m_no_draws_label->Show(true);
	m_no_draws_label->SetForegroundColour(*wxBLACK);
	m_no_draws_label->SetValueText(_("This set does not contain draws for which summary values are calculated."));
	m_values_sizer->Add(m_no_draws_label, 1, wxEXPAND);

	m_no_draws_line = new wxStaticLine(this, wxID_ANY);
	m_values_sizer->Add(m_no_draws_line, 0, wxEXPAND);

	sizer->Add(m_values_sizer, 1, wxEXPAND);

	//sizer->Add(new wxStaticLine(this, wxID_ANY), 0, wxEXPAND);
	sizer->Add(new wxButton(this, wxID_HELP), 0, wxALIGN_CENTER | wxALL, 5);

	sizer->Fit(this);

	this->draw_panel = draw_panel;

	this->draws_controller = NULL;
}

void SummaryWindow::OnIdle(wxIdleEvent &event) {

	if (!m_active)
		return;

	bool resize = false;

#define SHOW(wdg, show) \
	wdg->Show(show); \
	m_values_sizer->Show(wdg, show, true);


	if (m_update) {
		if (m_summary_draws_count == 0) {
			SHOW(m_no_draws_label, true)
			SHOW(m_no_draws_line, true)
			resize = true;
		} else { 
			for (size_t i = 0; i < m_draws.Count(); ++i) {
	
				ObservedDraw* od = m_draws[i];
				if (od == NULL)
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
		
				if (!std::isnan(data_percentage) && !(data_percentage > 0.9999))
					text += wxString::Format(_T(" (%.2f%%)"), data_percentage * 100);
		
				l->SetValueText(text);
		
				resize = true;

			}
			if (m_no_draws_label->IsShown()) {
				SHOW(m_no_draws_label, false);
				SHOW(m_no_draws_line, false);
			}
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
		tooltip += _T("\n\n") + wxString::Format(_("Data contains %.2f%% probes from current period"), data_percentage * 100);

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

	m_draws.resize(draws_controller->GetDrawsCount(), NULL);
	m_labels.resize(draws_controller->GetDrawsCount(), NULL);
	m_lines.resize(draws_controller->GetDrawsCount(), NULL);
       	for (size_t i = 0; i < draws_controller->GetDrawsCount(); ++i)
		SetDraw(draws_controller->GetDraw(i));

	Resize();
}

void SummaryWindow::Resize() {
	m_values_sizer->Layout();
	GetSizer()->Layout();
	GetSizer()->Fit(this);
}
	
void SummaryWindow::Deactivate() {
	if (m_active == false) 
		return;

	draws_controller->DetachObserver(this);

       	for (size_t i = 0; i < m_draws.Count(); ++i) {
		ObservedDraw* od = m_draws[i];
		if (od != NULL)
			StopDisplaying(i);
	}

	m_draws.SetCount(0);

	m_active = false;

	if (m_summary_draws_count != 0) 
		wxLogError(_T("Something is wrong with number of 'proper draws' in SummaryWindow"));

}

void SummaryWindow::StopDisplaying(int no) {
	assert(m_draws[no]);

	m_summary_draws_count--;
	assert(m_summary_draws_count >= 0);

	m_values_sizer->Detach(m_lines[no]);
	m_values_sizer->Detach(m_labels[no]);

	m_lines[no]->Destroy();
	m_labels[no]->Destroy();

	m_lines[no] = NULL;
	m_labels[no] = NULL;

	delete m_draws[no];
	m_draws[no] = NULL;

	m_update = true;
}

void SummaryWindow::Detach(DrawsController *draws_controller) {
	DrawObserver::Detach(draws_controller);
}

void SummaryWindow::StartDisplaying(int no) {
	const int default_draw_width = 400;
	const int default_draw_height = 20;

	assert(m_active);
	Draw* draw = m_draws[no]->draw;

	m_draws[no]->update = true;
	m_draws[no]->tooltip = true;

	m_summary_draws_count++;

	m_labels[no] = new TTLabel(this, wxID_ANY, wxDefaultPosition,
		   	wxSize(default_draw_width,default_draw_height));
	m_labels[no]->SetUnitText(draw->GetDrawInfo()->GetShortName() + _T(":"));
	m_labels[no]->SetBackgroundColour(draw->GetDrawInfo()->GetDrawColor());
	m_lines[no] = new wxStaticLine(this, wxID_ANY);

	m_values_sizer->Add(m_labels[no], 1, wxEXPAND);
	m_values_sizer->Add(m_lines[no], 0, wxEXPAND);

	m_tooltip = true;
	m_update = true;
}

void SummaryWindow::SetDraw(Draw *draw) {
	int no = draw->GetDrawNo();

	assert(m_draws[no] == NULL);

	if (draw->GetDrawInfo() == NULL)
		return;
	if (draw->GetDrawInfo()->GetSpecial() != TDraw::HOURSUM)
		return;

	m_draws[no] = new ObservedDraw(draw);

	StartDisplaying(no);
}

void SummaryWindow::Attach(DrawsController *draws_controller) {
	this->draws_controller = draws_controller;
}

void SummaryWindow::DrawInfoChanged(Draw *draw) {
	if (draw->GetSelected()) {
		Deactivate();
		Activate();
	}
}

void SummaryWindow::DrawsSorted(DrawsController *) {
	Deactivate();
	Activate();
}

void SummaryWindow::UpdateDraw(Draw *draw) {
	int no = draw->GetDrawNo();

	ObservedDraw *od = m_draws[no];

	if (od == NULL)
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

void SummaryWindow::AverageValueCalculationMethodChanged(Draw *draw) {
	UpdateDraw(draw);
}

void SummaryWindow::PeriodChanged(Draw *draw, PeriodType period) {
	UpdateDraw(draw);
}

void SummaryWindow::EnableChanged(Draw *draw) {
	int no = draw->GetDrawNo();

	ObservedDraw *od = m_draws[no];
	if (od == NULL)
		return;

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

	bool ret = wxFrame::Show(show);
	if (ret)
		Resize();

	return ret;
}


