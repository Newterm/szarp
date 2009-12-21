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

#include "ids.h"
#include "classes.h"
#include "drawobs.h"
#include "drawtime.h"
#include "dbinquirer.h"
#include "database.h"
#include "draw.h"
#include "drawsctrl.h"
#include "drawswdg.h"
#include "drawtime.h"
#include "timewdg.h"
#include <wx/intl.h>


BEGIN_EVENT_TABLE(TimeWidget, wxRadioBox)
        EVT_RADIOBOX(wxID_ANY, TimeWidget::OnRadioSelected)
	EVT_SET_FOCUS(TimeWidget::OnFocus)
END_EVENT_TABLE()

TimeWidget::TimeWidget(wxWindow* parent, DrawsWidget *draws_widget, PeriodType pt)
        : wxRadioBox(), m_draws_widget(draws_widget), m_previous(0)
{
	SetHelpText(_T("draw3-base-range"));

        wxString time_wdg_choices[] = {
                _("YEAR"),
                _("MONTH"),
                _("WEEK"),
                _("DAY"),
                _("SEASON")
        };
        Create(parent, wxID_ANY, 
			_T(""), // label
			wxDefaultPosition, wxDefaultSize,
			5, // number of options
			time_wdg_choices, // options strings array
			1, // number of columns
			wxRA_SPECIFY_COLS | // vertical
			wxSUNKEN_BORDER | 
			wxWANTS_CHARS);
	switch (pt) {
		case PERIOD_T_MONTH:
			m_selected = 1;
			break;
		case PERIOD_T_WEEK:
			m_selected = 2;
			break;
		case PERIOD_T_DAY:
			m_selected = 3;
			break;
		case PERIOD_T_SEASON: 
			m_selected = 4;
			break;				     
		default:
		case PERIOD_T_YEAR:
		        m_selected = 0;
	}
        SetSelection(m_selected);
	SetToolTip(_("Select period to display"));
}

int TimeWidget::SelectPrev()
{
	int p = m_previous;
	Select(p);
	return p;
}

void TimeWidget::Select(int item, bool refresh)
{
	m_previous = m_selected;
	SetSelection(item);
	if (refresh)
		m_draws_widget->SetPeriod((PeriodType)item);
        m_selected = item; 
}

void TimeWidget::OnRadioSelected(wxCommandEvent& event)
{
        if (event.GetSelection() != m_selected) {
		Select(event.GetSelection());
		GetParent()->SetFocus();
	}
}
void TimeWidget::OnFocus(wxFocusEvent &event) {
	GetParent()->SetFocus();
}

void TimeWidget::PeriodChanged(Draw *draw, PeriodType pt) {
	if (draw->GetSelected())
		SetSelection(pt);
}

void TimeWidget::DrawInfoChanged(Draw *draw) {
	if (draw->GetSelected())
		SetSelection(draw->GetPeriod());
}
