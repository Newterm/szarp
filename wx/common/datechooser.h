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

#ifndef __DATE_CHOOSER_H__
#define __DATE_CHOOSER_H__

#include <wx/wx.h>
#include <wx/window.h>
#include <wx/control.h>
#include <wx/datetime.h>
#include <wx/calctrl.h>
#include <wx/spinctrl.h>
#include <time.h>

class DateChooserWidget;
class mywxTimeCtrl;

/*
class mywxTimeCtrl : public wxControl// wxWindow //
{

//	top_sizer:
//	+----------++----------+
//	|sizer1    ||sizer2    |
//	|+--------+||+--------+|
//	||sizer1_1||||sizer2_1||
//	|+--------+||+--------+|
//	|+--------+||+--------+|
//	||sizer1_2||||sizer2_2||
//	|+--------+||+--------+|
//	+----------++----------+

	wxSizer *top_sizer;
	wxSizer *sizer1;
	wxSizer *sizer1_1;
	wxSizer *sizer1_2;
	wxSizer *sizer2;
	wxSizer *sizer2_1;
	wxSizer *sizer2_2;


	public:
		mywxTimeCtrl(wxWindow *parent, int initial_hour, int initial_minute);

		int GetHour();
		int GetMinute();
};
*/

#define MAX_TIME_T_FOR_WX_DATE_TIME (std::numeric_limits<time_t>::max() / ( sizeof(time_t) == 8 ? 1000 : 1 ))

class DateChooserWidget : public wxDialog
{
	wxDateTime chosen_date; // date picked by the user
	time_t min_date; // minimal possible date (user cannot choose date earlier than this)
	time_t max_date; // maximal possible date (user cannot choose date later than this)
	int minute_quantum; // a 'quantum' step while changing minutes
	int second_quantum; // a 'quantum' step while changing seconds
	int current_minute; // what is the current value of the 'minute' SpinCtrl
	int current_second; // what is the current value of the 'minute' SpinCtrl

        wxCalendarCtrl* date_control;
	mywxTimeCtrl* time_control;

	wxSizer *sizer1;
	  wxSizer *sizer1_1;
	    wxSizer *sizer1_1_1;
	    wxSizer *sizer1_1_2;
	      wxSizer *sizer1_1_2_1;
	      wxSizer *sizer1_1_2_2;
	      wxSizer *sizer1_1_2_3;
	      wxSizer *sizer1_1_2_4;
	  wxSizer *sizer1_2;
	    wxSizer *sizer1_2_1;
	      wxSizer *sizer1_2_1_1;
	      wxSizer *sizer1_2_1_2;
	        wxSizer *sizer1_2_1_2_1;
	        wxSizer *sizer1_2_1_2_2;
	          wxSizer *sizer1_2_1_2_2_2;
	    wxSizer *sizer1_2_2;
	      wxSizer *sizer1_2_2_2;
	        wxSizer *sizer1_2_2_2_2;
	  wxSizer *sizer1_3;

	//wxTextCtrl *hour;
	//wxTextCtrl *minute;

	wxSpinCtrl *hour_control;
	wxSpinCtrl *minute_control;
	wxSpinCtrl *second_control;

	public:
		DateChooserWidget(
			wxWindow *parent,
			wxString caption = _("Choose date"), // title string
			int quantum = 10, 
			time_t min_date = 0, 
			time_t max_date = time(NULL),
			int second_quantum = -1
		); 
		~DateChooserWidget();

		// Uwaga: 'date' jest uzywane jako wartosc zwracana ale tez jako wartosc
		// inicjujaca Widget 
		bool GetDate(wxDateTime &date);
	
		void onChar(wxKeyEvent &event);
		void onOk(wxCommandEvent &event);
		void onCancel(wxCommandEvent &event);
		void onHourChange(wxSpinEvent &event);
		void onMinuteChange(wxSpinEvent &event);
		void onSecondChange(wxSpinEvent &event);

	private:
    		int MoveTimeWithQuantum(int current, int quantum, int position);
		DECLARE_EVENT_TABLE()
};



#endif
