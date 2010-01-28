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
 *  draw3 SZARP
 
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 *
 * $Id$
 * Widget for printing info about selected draw(s).
 */

#include <algorithm>
#include <math.h>

#include "cconv.h"

#include "ids.h"
#include "classes.h"
#include "drawobs.h"
#include "infowdg.h"
#include "dbinquirer.h"
#include "database.h"
#include "drawtime.h"
#include "coobs.h"
#include "drawsctrl.h"
#include "draw.h"
#include "timeformat.h"
#include "cfgmgr.h"

BEGIN_EVENT_TABLE(InfoWidget, wxPanel)
	EVT_IDLE(InfoWidget::OnIdle)
END_EVENT_TABLE()

InfoWidget::InfoWidget(wxWindow *parent, wxWindowID id) :
	wxPanel(parent, id, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS)
{
	SetHelpText(_T("draw3-base-win"));
#ifdef MINGW32
	value_panel = new wxPanel(this, -1, wxDefaultPosition);
	value_panel2 = new wxPanel(this, -1, wxDefaultPosition);
#else
	value_panel = new wxPanel(this, -1, wxDefaultPosition,
			wxDefaultSize, wxSIMPLE_BORDER);
	value_panel2 = new wxPanel(this, -1, wxDefaultPosition,
			wxDefaultSize, wxSIMPLE_BORDER);
#endif
	current_value = value_text = new wxStaticText(value_panel, -1,
			_T(" "), wxDefaultPosition, wxSize(130, -1),
			wxALIGN_CENTER | wxST_NO_AUTORESIZE);
	other_value = value_text2 = new wxStaticText(value_panel2, -1,
			_T(" "), wxDefaultPosition, wxSize(130, -1),
			wxALIGN_CENTER | wxST_NO_AUTORESIZE);
	
	wxBoxSizer *panel_sizer = new wxBoxSizer(wxVERTICAL);
	panel_sizer->Add(value_text, 1, wxEXPAND | wxALL, 2);
	value_panel->SetSizer(panel_sizer);
	panel_sizer->SetSizeHints(value_panel);

	wxBoxSizer *panel_sizer2 = new wxBoxSizer(wxVERTICAL);
	panel_sizer2->Add(value_text2, 1, wxEXPAND | wxALL, 2);
	value_panel2->SetSizer(panel_sizer2);
	panel_sizer2->SetSizeHints(value_panel2);

	current_date = date_text = new wxStaticText(this, -1,
			_T(" "), wxDefaultPosition);
	other_date = date_text2 = new wxStaticText(this, -1,
			_T(" "), wxDefaultPosition);

	avg_text = new wxStaticText(this, -1,
			_T(" "), wxDefaultPosition); 
		
	sizer1 = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *sizer1_1 = new wxBoxSizer(wxHORIZONTAL);
	sizer1_2 = new wxBoxSizer(wxHORIZONTAL);

	sizer1_1->Add(value_panel, 0);//, wxALIGN_CENTRE_VERTICAL);
	sizer1_1->Add(date_text, 0);//, wxALIGN_CENTRE_VERTICAL | wxALIGN_LEFT);

	sizer1_2->Add(value_panel2, 0); //, wxALIGN_CENTRE_VERTICAL);
	sizer1_2->Add(date_text2, 0); //, wxALIGN_CENTRE_VERTICAL | wxALIGN_LEFT);

	sizer1->Add(sizer1_1, 0, wxEXPAND);
	sizer1->Add(sizer1_2, 0, wxEXPAND);
	sizer1->Add(avg_text, 0, wxALIGN_LEFT | wxTOP, 2);
	SetSizer(sizer1);

	ShowExtraPanel(false);
	
	m_update_time = 
		m_update_values =
		m_update_current_value = false;

	m_double_cursor = false;

	//sizer1->SetSizeHints(this);

	m_draw = NULL;
}

InfoWidget::~InfoWidget()
{
}

void InfoWidget::DrawInfoChanged(Draw *draw) {
	if (draw->GetSelected() == false)
		return;

	m_draw = draw;
	SetDrawInfo(draw);

	m_update_values = true;
	m_update_time = true;
	m_update_current_value = true;
	m_update_values = true;
	DoubleCursorMode(false);
}

void InfoWidget::PeriodChanged(Draw *draw, PeriodType period) {
	m_period = period;
	m_update_values = true;
	m_update_current_value = true;
	DoubleCursorMode(false);
}

void InfoWidget::SetDrawInfo(Draw *draw) {
	DrawInfo *info = draw->GetDrawInfo();
	if (info == NULL)
		return;
	SetUnit(info->GetUnit());
	SetColor(info->GetDrawColor());
	SetPrec(info->GetPrec());
	SetPeriod(draw->GetPeriod());
}

void InfoWidget::NewData(Draw *draw, int idx) {
	if (draw->GetSelected() == false)
		return;
	if (idx == draw->GetCurrentIndex()) 
		m_update_current_value = true;
}

void InfoWidget::FilterChanged(Draw *draw) {
	if (draw->GetSelected() == false)
		return;
	m_update_values = true;
	m_update_current_value = true;
}

void InfoWidget::CurrentProbeChanged(Draw* draw, int pi, int ni, int d) {
	if (draw->GetSelected() == false)
		return;

	const Draw::VT& vt = draw->GetValuesTable();

	m_update_current_value = true;
	
	m_current_time = draw->GetCurrentTime();
	m_update_time = true;

	if (m_double_cursor) {
		bool forward = vt.m_stats.m_start <= vt.m_stats.m_end;

		if ((forward && other_value != value_text2) ||
				(!forward && other_value != value_text)) {

			wxString tmp;
			tmp = other_value->GetLabel();
			other_value->SetLabel(current_value->GetLabel());
			current_value->SetLabel(tmp);

			tmp = other_date->GetLabel();
			other_date->SetLabel(current_date->GetLabel());
			current_date->SetLabel(tmp);

			if (forward) {
				other_value = value_text2;
				other_date = date_text2;

				current_value = value_text;
				current_date = date_text;
			} else {
				other_value = value_text;
				other_date = date_text;

				current_value = value_text2;
				current_date = date_text2;
			}
		}
	}

}

void InfoWidget::SetTime(const wxDateTime &time) {
	wxString str;
	str = FormatTime(time, m_period);
	current_date->SetLabel(str);
}

void InfoWidget::SetOtherTime(const wxDateTime &time) {
	wxString str;
	str = FormatTime(time, m_period);
	other_date->SetLabel(str);
}

void InfoWidget::SetColor(const wxColour& col) {
	wxColour c = col;
	if (c == *wxBLUE) {
		/* on some displays, black text is poorly
		 * visible on blue background, so we make
		 * background color a little lighter */
		c = wxColour(60, 60, 255);
	}
#ifdef MINGW32
	value_text->SetBackgroundColour(c);
	value_text2->SetBackgroundColour(c);
#else
	value_panel->SetBackgroundColour(c);
	value_panel2->SetBackgroundColour(c);
	value_panel->Refresh();
	value_panel2->Refresh();
#endif

}

void InfoWidget::SetPrec(int prec) {
	m_prec = prec;
}

void InfoWidget::SetPeriod(PeriodType period) {
	m_period = period;
}

void InfoWidget::SetUnit(const wxString &unit) {
	m_unit = unit;
}

void InfoWidget::DrawSelected(Draw *draw) {
	m_draw = draw;
	SetDrawInfo(draw);

	m_update_time = 
		m_update_values =
		m_update_current_value = true;

	if (m_double_cursor) {
		const Draw::VT& vt = m_draw->GetValuesTable();

		int end = vt.m_stats.m_end;

		double val = vt.Get(end).val;
		
		int idx = end - vt.m_view.Start();

		SetOtherValue(val);

		SetOtherTime(m_draw->GetTimeOfIndex(idx));
	}

}

void InfoWidget::UpdateValues() {

	const Draw::VT& vt = m_draw->GetValuesTable();

	if (vt.m_count == 0) {
		avg_text->SetLabel(_T(""));
		return;
	}
	
	wxString info_string = wxString(_("min.=")) + m_draw->GetDrawInfo()->GetValueStr(vt.m_min, _T("- -"))
			+ _T(" ; ") + _("avg.=") +  m_draw->GetDrawInfo()->GetValueStr(vt.m_sum / vt.m_count, _T("- -"))
			+ _T(" ; ") + _("max.=") + m_draw->GetDrawInfo()->GetValueStr(vt.m_max, _T("- -"));
	if (!std::isnan(vt.m_data_probes_ratio))
		info_string += _T("  ") + wxString::Format(_T("(%%%.2f)"), vt.m_data_probes_ratio * 100);

	avg_text->SetLabel(info_string);
	avg_text->Refresh();
}

void InfoWidget::SetValue(double val) {
	wxString s = m_draw->GetDrawInfo()->GetValueStr(val, _T("- -"));
	s += _T(" ") + m_unit;
	current_value->SetLabel(s);
}

void InfoWidget::SetOtherValue(double val) {
	wxString s = m_draw->GetDrawInfo()->GetValueStr(val, _T("- -"));
	s += _T(" ") + m_unit;
	other_value->SetLabel(s);
}


void InfoWidget::SetEmpty()
{
#ifdef MINGW32
	value_text->SetBackgroundColour(DRAW3_BG_COLOR);
	value_text2->SetBackgroundColour(DRAW3_BG_COLOR);
#else
	value_panel->SetBackgroundColour(DRAW3_BG_COLOR);
	value_panel2->SetBackgroundColour(DRAW3_BG_COLOR);
#endif	
	SetEmptyVals();
}

void InfoWidget::SetEmptyVals() {
	value_text->SetLabel(_T("- -"));
	
	date_text->SetLabel(_T(""));

	value_text2->SetLabel(_T("- -"));
	
	date_text2->SetLabel(_T(""));
	
	avg_text->SetLabel(_T(""));
}

void InfoWidget::StatsChanged(Draw *draw) {
	if (draw->GetSelected() == false)
		return;

	m_update_values = true;
}

void InfoWidget::OnIdle(wxIdleEvent &event) {
	if (m_update_current_value) {
		int idx = m_draw->GetCurrentIndex();
		if (idx >= 0)
			m_val = m_draw->GetValuesTable().at(idx).val;
		else
			m_val = nan("");
		SetValue(m_val);
		m_update_current_value = false;
	}

	if (m_update_time) {
		SetTime(m_current_time);
		m_update_time = false;
	}

	if (m_update_values) {
		UpdateValues();
		m_update_values = false;
	}

}

void InfoWidget::ShowExtraPanel(bool show) {
	sizer1->Show(sizer1_2, show, true);
	sizer1->Layout();
	GetParent()->Layout();
}

void InfoWidget::DoubleCursorChanged(DrawsController *draws_controller) {
	DoubleCursorMode(draws_controller->GetDoubleCursor());
	m_update_values = true;
}

void InfoWidget::NumberOfValuesChanged(DrawsController *draws_controller) {
	DoubleCursorMode(false);
	m_update_values = true;
}

void InfoWidget::DoubleCursorMode(bool set) {
	ShowExtraPanel(set);
	m_double_cursor = set;
	if (set) {
		value_text2->SetLabel(current_value->GetLabel());
		date_text2->SetLabel(current_date->GetLabel());
	} else {
		value_text->SetLabel(current_value->GetLabel());
		date_text->SetLabel(current_date->GetLabel());

		current_value = value_text;
		current_date = date_text;

		other_value = value_text2;
		other_date = date_text2;
	}
}
