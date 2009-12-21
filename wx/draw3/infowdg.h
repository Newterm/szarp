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
 * Widget for printing info about selected draw(s).
 */

#ifndef __INFOWDG_H__
#define __INFOWDG_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <wx/datetime.h>

/** Class for printing info about selected draw(s). */
class InfoWidget : public wxPanel, public DrawObserver {
	public:
		InfoWidget(wxWindow *parent, wxWindowID id = -1);
		virtual ~InfoWidget();
		/**Updates statistics*/
		virtual void DrawInfoChanged(Draw *draw);
		/**Updates @see m_val value and @see date_text*/
		virtual void CurrentProbeChanged(Draw *draw, int pi, int ni, int d);
		/**Updates @see m_val value*/
		virtual void FilterChanged(Draw *draw);
		/**Updates @see m_min, @see m_max, @see m_max and m_data_count values*/
		virtual void PeriodChanged(Draw *draw, PeriodType period);
		/**Updates @see m_min, @see m_max, @see m_max and m_data_count values*/
		virtual void StatsChanged(Draw* draw);

		virtual void DrawSelected(Draw* draw);

		virtual void NewData(Draw* draw, int idx);

		virtual void DoubleCursorChanged(DrawsController *draw);

		void DoubleCursorMode(bool set);
	protected:
		/**Clears all child widgets and values*/
		void SetEmpty();
		/**Clears all but @see date_text widgets*/
		void SetEmptyVals();
		/**Recalculates @see m_min, @see m_max, @see m_max and m_data_count values
		 * and correspoding widgets*/
		void RecalulateValues(Draw *draw);
		/**Updates @see m_val and @see value_text*/
		void SetCurrentValue(double val);
		/**Updates all displayed data*/
		void SetDrawInfo(Draw *draw);
		/**Sets background color of @see value_text widget*/
		void SetColor(const wxColour& color);
		/**Sets @see m_unit value*/
		void SetUnit(const wxString& unit);
		/**Sets precision of currently drawn param*/
		void SetPrec(int prec);
		/**Updates whole widget*/
		void SetPeriod(PeriodType period);
		/**Updates @see date_text widget*/
		void SetTime(const wxDateTime &time);
		/**Updates @see date_text widget*/
		void SetOtherTime(const wxDateTime &time);
		/**Copies content of m_min, m_max, etc values to corresponding child widgets*/
		void UpdateValues();
		/**Sets current value*/
		void SetValue(double val);
		/**Sets other value*/
		void SetOtherValue(double val);

		/**Shows/hides extra panel. This Additional panel is used in double cursor mode*/
		void ShowExtraPanel(bool show);

		/**Idle event handler. If neccesary updates child widgets*/
		void OnIdle(wxIdleEvent& event);

		/**Indicates if widget displaying current time shall be updated*/
		bool m_update_time;

		/**Indicates if min/max/avg widgets shall be updated*/
		bool m_update_values;

		/**Indicates if current value shall be updated*/
		bool m_update_current_value;

		/**Current value*/
		double m_val;
		/**Current probe time*/
		wxDateTime m_current_time;
		/**Currently observed draw*/
		Draw *m_draw;	
		/**Period of observerd @see Draw*/
		PeriodType m_period;
		/**Panel holding widgets showing value*/
		wxPanel* value_panel;
		/**Panel holding widgets showing second value (in double cursor mode)*/
		wxPanel* value_panel2;
		/**Widget displaying value*/
		wxStaticText* value_text;
		/**Widget displaying date*/
		wxStaticText* date_text;
		/**Widget displaying secon value (in double cursor mode)*/
		wxStaticText* value_text2;
		/**Widget displaying date (in double cursor mode)*/
		wxStaticText* date_text2;
		/**Widget displaying min, max, avg values*/
		wxStaticText* avg_text;
		/**Points to widget displaying current value*/
		wxStaticText* current_value;
		/**Points to widget displaying current date*/
		wxStaticText* current_date;
		/**Points to widget displaying other i.e. not current value (used is double cursor mode)*/
		wxStaticText* other_value;
		/**Points to widget displaying other i.e. not current value (used in double cursor mode)*/
		wxStaticText* other_date;
		/*Sizer holding extra panel*/
		wxBoxSizer* sizer1_2;
		/**Main sizer*/
		wxBoxSizer* sizer1;
		/**Period of observerd @see Draw*/
		int m_prec;
		/**Unit of current param*/
		wxString m_unit;
		/**True if is double cursor mode*/
		bool m_double_cursor;

		DECLARE_EVENT_TABLE()
};

#endif

