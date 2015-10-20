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
#include "coobs.h"
#include "drawsctrl.h"
#include "drawswdg.h"
#include "drawtime.h"
#include "timewdg.h"
#include <wx/intl.h>

#include "wxlogging.h"


BEGIN_EVENT_TABLE(TimeWidget, wxScrolledWindow)
        EVT_RADIOBOX(wxID_ANY, TimeWidget::OnRadioSelected)
#ifndef __WXMSW__
	EVT_SET_FOCUS(TimeWidget::OnFocus)
#endif
END_EVENT_TABLE()

class RadioButtonValidator : public wxValidator {
	
	public:
	RadioButtonValidator(DrawsWidget *drawswdg) : m_draws_widget(drawswdg) {}

	RadioButtonValidator(const RadioButtonValidator& validator) : m_draws_widget(validator.m_draws_widget) {}
		
	~RadioButtonValidator() {}
		
	/** Make a clone of this validator (or return NULL) - currently necessary
	 * if you're passing a reference to a validator. */
	virtual wxObject *Clone() const { return new RadioButtonValidator(*this); }
	
	bool Copy(const RadioButtonValidator& val) { wxValidator::Copy(val); m_draws_widget = val.m_draws_widget; return true;}
	
	/** Event handler - tries to get rid of focus. */
	void OnFocus(wxFocusEvent &event) { m_draws_widget-> SetFocus(); }
	
	protected:
	DECLARE_EVENT_TABLE()

	DrawsWidget *m_draws_widget;/** pointer to draws widget, we have to communicate with this object */
};

BEGIN_EVENT_TABLE(RadioButtonValidator, wxValidator)
#ifndef __WXMSW__
	EVT_SET_FOCUS(RadioButtonValidator::OnFocus)
#endif
END_EVENT_TABLE()


TimeWidget::TimeWidget(wxWindow* parent, DrawsWidget *draws_widget, PeriodType pt)
        : wxScrolledWindow(parent, wxID_ANY), m_draws_widget(draws_widget), m_previous(0)
{
	SetHelpText(_T("draw3-base-range"));

        wxString time_wdg_choices[] = {
		_("DECADE"),
                _("YEAR"),
                _("MONTH"),
                _("WEEK"),
                _("DAY"),
                _("30 MINUTES"),
		_("5 MINUTES"),
		_("MINUTE"),
		_("30 SECONDS"),
                _("SEASON")
        };

	m_radio_box = new wxRadioBox();
        m_radio_box->Create(this, wxID_ANY, 
			_T(""), // label
			wxDefaultPosition, wxDefaultSize,
			sizeof(time_wdg_choices) / sizeof(time_wdg_choices[0]),
			time_wdg_choices, // options strings array
			1, // number of columns
			wxRA_SPECIFY_COLS | // vertical
			wxSUNKEN_BORDER | 
			wxWANTS_CHARS,
			RadioButtonValidator(m_draws_widget));
	switch (pt) {
		case PERIOD_T_DECADE:
		        m_selected = 0;
		case PERIOD_T_MONTH:
			m_selected = 2;
			break;
		case PERIOD_T_WEEK:
			m_selected = 3;
			break;
		case PERIOD_T_DAY:
			m_selected = 4;
			break;
		case PERIOD_T_30MINUTE:
			m_selected = 5;
			break;
		case PERIOD_T_5MINUTE:
			m_selected = 6;
			break;
		case PERIOD_T_MINUTE:
			m_selected = 8;
			break;
		case PERIOD_T_30SEC:
			m_selected = 9;
			break;
		case PERIOD_T_SEASON: 
			m_selected = 10;
			break;				     
		default:
		case PERIOD_T_YEAR:
		        m_selected = 1;
	}
        m_radio_box->SetSelection(m_selected);
	SetToolTip(_("Select period to display"));

	SetScrollRate(10, 10);

	wxBoxSizer * sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(m_radio_box, 0, wxEXPAND);
	sizer->SetVirtualSizeHints(this);
	sizer->Layout();
	SetSizer(sizer);
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
	m_radio_box->SetSelection(item);

	char buf[128];
	snprintf(buf,128,"timewdg:%s",(const char*)period_names[item].mb_str(wxConvUTF8));
	UDPLogger::LogEvent( buf );

	if (refresh) {
		m_draws_widget->SetPeriod((PeriodType)item);
	}
        m_selected = item; 
}

void TimeWidget::OnRadioSelected(wxCommandEvent& event)
{
        if (event.GetSelection() != m_selected) {
		Select(event.GetSelection());
#ifdef __WXMSW__
		GetParent()->SetFocus();
#endif
	}
}

void TimeWidget::OnFocus(wxFocusEvent &event)
{
#ifdef __WXGTK__
	GetParent()->SetFocus();
#endif
}

void TimeWidget::PeriodChanged(Draw *draw, PeriodType pt) {
	if (draw->GetSelected()) {
		m_selected = pt;
		m_radio_box->SetSelection(pt);
	}
}

void TimeWidget::DrawInfoChanged(Draw *draw) {
	if (draw->GetSelected()) {
		m_selected = draw->GetPeriod();
		m_radio_box->SetSelection(draw->GetPeriod());
	}
}

int TimeWidget::GetSelection() {
	return m_radio_box->GetSelection();
}
