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
/**
 * Widget do wyboru daty i godziny.

 * $Id$
 */

#include <limits>
#include "datechooser.h"
#include <wx/string.h>
#include <fonts.h>

#ifndef DRAW3_BG_COLOR
#ifdef MINGW32
#define DRAW3_BG_COLOR wxColour(200,200,200)
#else
#define DRAW3_BG_COLOR wxSystemSettings::GetColour(wxSYS_COLOUR_BACKGROUND)
#endif
#endif // DRAW3_BG_COLOR


enum {
	ID_DateChooserWidget = wxID_HIGHEST, 
	ID_SpinCtrlHour,
	ID_SpinCtrlMinute,
	ID_SpinCtrlSecond
};



DateChooserWidget::DateChooserWidget(wxWindow *parent,
		wxString caption,
		int minute_quantump,
		time_t min_datep,
		time_t max_datep,
		int second_quantump)


	: wxDialog(parent, ID_DateChooserWidget, caption, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxTAB_TRAVERSAL)

{
	szSetDefFont(this);
	#ifdef MINGW32
       	SetBackgroundColour(DRAW3_BG_COLOR);
	#endif

	minute_quantum = minute_quantump;
	min_date = min_datep;
	max_date = max_datep;

	//wxDateTime chosen_date have_string;

	// g³ówny sizer
        sizer1 = new wxBoxSizer(wxVERTICAL);

	//////////
	// g³ówny podzia³ na 3 czê¶ci: Data (1_1_1) + Godzina (1_1_2) + Przyciski (1_2)
	//////////

	// Data + czas (1_1)
        sizer1_1 = new wxBoxSizer(wxHORIZONTAL);
	sizer1->Add(sizer1_1, 0, wxALL, 10); // 1_1

        date_control = new wxCalendarCtrl(this, -1, wxDefaultDateTime, 
			wxDefaultPosition, wxDefaultSize, 
			wxCAL_SHOW_HOLIDAYS| wxCAL_MONDAY_FIRST);

		wxSize calSize = date_control->GetSize();
		date_control->SetSizeHints(calSize.GetWidth() * 1.1, calSize.GetHeight());

	sizer1_1->Add(date_control, 0, wxALL, 5); // 1_1_1

	date_control->SetFocus();

	// Czas (1_1_2)
	sizer1_1_2 = new wxBoxSizer(wxVERTICAL); // sizer dla daty i godziny 
	sizer1_1->Add(sizer1_1_2, 0, wxLEFT, 15); // dodanie do 1_1

	sizer1_1_2->Add(new wxStaticText(this, -1, 
				_("Hour:")), 0, wxTOP, 35); // the 'hour' text (1_1_2_1)
	hour_control = new wxSpinCtrl(this, ID_SpinCtrlHour, 
			_T("12"), // initial value
			wxDefaultPosition, wxDefaultSize, 
			wxSP_ARROW_KEYS | wxSP_WRAP); // the hour spin control (1_1_2_2)
	sizer1_1_2->Add(hour_control, 0, wxALL, 0); // (1_1_2_2)
	hour_control->SetRange(0, 23);

	sizer1_1_2->Add(new wxStaticText(this, -1, 
				_("Minute:")), 0, wxTOP, 5); // the 'minute' text (1_1_2_3)
	minute_control = new wxSpinCtrl(this, ID_SpinCtrlMinute, 
			_T("00"), // initial value
			wxDefaultPosition, wxDefaultSize, 
			wxSP_ARROW_KEYS | wxSP_WRAP); // the minute spin control (1_1_2_4)

	sizer1_1_2->Add(minute_control, 0, wxALL, 0); 
	minute_control->SetRange(0, 59);

	if (second_quantump != -1) {
		sizer1_1_2->Add(new wxStaticText(this, -1, 
				_("Second:")), 0, wxTOP, 5); // the 'second' text (1_1_2_5)
		second_control = new wxSpinCtrl(this, ID_SpinCtrlSecond, 
				_T("00"), // initial value
				wxDefaultPosition, wxDefaultSize, 
				wxSP_ARROW_KEYS | wxSP_WRAP); // the second spin control (1_1_2_6)
		sizer1_1_2->Add(second_control, 0, wxALL, 0); 
		second_control->SetRange(0, 59);
	} else {
		second_control = NULL;
	}
	second_quantum = second_quantump;

	// OK & Cancel buttons sizer (1_2)
        sizer1_2 = new wxBoxSizer(wxHORIZONTAL);
	sizer1->Add(sizer1_2, 0, wxALL, 10); // 1_2

	// OK & Cancel
	wxButton* ok_button = new wxButton(this, wxOK, _("OK"));
	wxButton* cancel_button = new wxButton(this, wxCANCEL, _("Cancel"));

	sizer1_2->Add(ok_button , 0, wxLEFT, 50);
	sizer1_2->Add(cancel_button, 0, wxLEFT, 20);

	SetSizer(sizer1);

        sizer1->SetSizeHints(this);
	
}

bool DateChooserWidget::GetDate(wxDateTime &date)
{
	int ret = wxID_OK;
	
	date_control->SetDate(date);
	hour_control->SetValue(date.GetHour());
	minute_control->SetValue(date.GetMinute());

	current_minute = date.GetMinute();
	current_second = date.GetSecond();
	
	while( (ret = ShowModal()) == wxID_OK){ 

		date = date_control->GetDate();
		
		// ustawiamy godzine
		date.SetHour(hour_control->GetValue());
		date.SetMinute(minute_control->GetValue());
		if (second_control)
			date.SetSecond(second_control->GetValue());

		wxDateTime tmin(time_t(0));
		wxDateTime tmax(MAX_TIME_T_FOR_WX_DATE_TIME);
		
		if (date <= tmin || date >= tmax) {
			wxMessageBox(_("Invalid (too large/small) date"), _("Error!"), wxOK);
			return false;
		}

		if (min_date == -1 && max_date == -1)
			return true;

		if (min_date == -1)
			min_date = 0;

		if(date.GetTicks() >= min_date && date.GetTicks() <= max_date)
			return true;
		else {
      			wxString buf;
			buf = _("Please choose the date from between: ");
			buf += wxDateTime(min_date).Format(_T("%Y-%m-%d %H:%M "));
			buf += _("and");
			buf += wxDateTime(max_date).Format(_T(" %Y-%m-%d %H:%M\n"));
			wxMessageDialog *mesg = new wxMessageDialog(this, buf, _("Incorrect date range"), wxOK);
			mesg->ShowModal();
			delete mesg;
		}
	} 
	return false;
}

void DateChooserWidget::onOk(wxCommandEvent &event)
{
	EndModal(wxID_OK);
}

void DateChooserWidget::onCancel(wxCommandEvent &event)
{
	EndModal(wxID_CANCEL);
}

void DateChooserWidget::onHourChange(wxSpinEvent &event)
{
}

int DateChooserWidget::MoveTimeWithQuantum(int current, int quantum, int position)
{
	if (abs(position - current) == 1 || abs(position - current) == 59) {
		if (position == current)
			return current; // do nothing
		else if (current == 0 && position == 59) 
			current = (60 / quantum - 1) * quantum;
		else if (position > current && position < 60 - quantum + 1) 
			current += quantum;
		else if (position < current && position != 0) 
			current -= quantum;
		else
			return 0;

	} else {
		current = position % quantum <= quantum / 2 ? 
				(position / quantum) * quantum : 
				(position / quantum + 1) * quantum;
		if (current >= 60)
			current = 0;
	}
	return current;
}

void DateChooserWidget::onMinuteChange(wxSpinEvent &event)
{
	current_minute = MoveTimeWithQuantum(current_minute, minute_quantum, event.GetPosition());

	minute_control->SetValue(current_minute);
}

void DateChooserWidget::onSecondChange(wxSpinEvent &event)
{
	current_second = MoveTimeWithQuantum(current_second, second_quantum, event.GetPosition());

	second_control->SetValue(current_second);
}

DateChooserWidget::~DateChooserWidget()
{
	Close(TRUE);
}


/////////////////////////
// mywxTimeCtrl::mywxTimeCtrl
/*
mywxTimeCtrl::mywxTimeCtrl(wxWindow *parent, int initial_hour, int initial_minute) 
	//: wxWindow(parent, -1)
{

	top_sizer = new wxBoxSizer(wxHORIZONTAL);

	SetSizer(top_sizer);

        //top_sizer->SetSizeHints(this);
}
*/


BEGIN_EVENT_TABLE(DateChooserWidget, wxDialog)
	EVT_BUTTON(wxOK, DateChooserWidget::onOk)
	EVT_BUTTON(wxCANCEL, DateChooserWidget::onCancel)
	EVT_SPINCTRL(ID_SpinCtrlHour, DateChooserWidget::onHourChange)
	EVT_SPINCTRL(ID_SpinCtrlMinute, DateChooserWidget::onMinuteChange)
	EVT_SPINCTRL(ID_SpinCtrlSecond, DateChooserWidget::onSecondChange)
END_EVENT_TABLE()


