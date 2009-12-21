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
/* draw3 
 * SZARP
 
 *
 * $Id$
 */

#ifndef __XYDIAG_H__
#define __XYDIAG_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <boost/tuple/tuple.hpp>


/**Fetches and associated corresponding values with each other*/
class DataMangler : public DBInquirer {
	/**@see DrawInfo of X param*/
	DrawInfo *m_dx;
	/**@see DrawInfo of Y param*/
	DrawInfo *m_dy;
	/**Type of probes fetches from database*/
	PeriodType m_period;
	/**Start of time range*/
	DTime m_start_time;
	/**Time of least recently fetched probe*/
	DTime m_current_time;
	/**End of time range*/
	DTime m_end_time;
	/**Paired values of params*/
	std::deque<boost::tuple<ValueInfo, ValueInfo, DTime> > m_draw_vals;
	/**Number of fetches probes*/
	int m_fetched;
	/**Number of remaing probes to fetch*/
	int m_tofetch;
	/**Number of pending probes*/
	int m_pending;
	/**Flags denoting if recevied values shall be averaged*/
	bool m_average;
	/**@see XYDialog*/
	XYDialog *m_dialog;
	/**@see ProgressFrame*/
	ProgressFrame *m_progress;
	/**Avergaes values received from db*/
	void AverageValues(XYGraph *graph);
	/**Pairs received values together*/
	void AssociateValues(XYGraph *graph);
	/**Find 'statistical' (min/max/avg) values*/
	void FindMaxMinRange(XYGraph *graph);
	/**Send a certain amount of request to db*/
	void ProgressFetch();
	/**Applay a PP(non-expandable acronym) algorithm to min, max values. Rounds them and makes them 'nice'.
	 * @param min mininum value
	 * @parma max maximum value
	 * @param prec precision of the value*/
	void ApplyPPAlgorithm(double &min, double &max, int prec);

	public:
	DataMangler(DatabaseManager *db, 
			DrawInfo *dx, 
			DrawInfo* dy, 
			wxDateTime start_time, 
			wxDateTime 
			end_time, 
			PeriodType pt, 
			XYDialog *dialog,
			bool average);

	/**Handles response from database.*/
	void DatabaseResponse(DatabaseQuery *query);

	/**Initiates fetch of data from db*/
	void Go();
	/**Non-significant in this case. Always return NULL*/
	virtual DrawInfo* GetCurrentDrawInfo();
	/**Non-significant in this case. Always return -1*/
	virtual time_t GetCurrentTime();
};


class XYDialog : public wxDialog, public ConfigObserver
{
	/**Period selected by user*/
	PeriodType m_period;
	/**Start of range selected by user*/
	DTime m_start_time;
	/**End of range selected by user*/
	DTime m_end_time;
	/**@see DrawInfo of graph to be drawn on horizontal axis*/
	DrawInfo* m_xdraw;
	/**@see DrawInfo of graph to be drawn on vertical axis*/
	DrawInfo* m_ydraw;
	/**@see DataMangler. Object for data retrival and manipulaton*/
	DataMangler *m_mangler;
	/**Widget for selectin probe types*/
	wxChoice *m_period_choice;
	/**Starting configuration, i.e. configuration from which user initially chooses draws*/
	wxString m_default_configuration;
	/**@see ConfigManager*/
	ConfigManager* m_config_manager;
	/**@see DatabaseManager*/
	DatabaseManager* m_database_manager;
	/**Checkbox for choosing if data on X axis shall be aveaged*/
	wxCheckBox* m_avg_check;
	/**@see IncSearch. Window for choosing search*/
	IncSearch* m_draw_search;
	/**@see XYFrame. Frame for displaying XY graph*/
	XYFrame *m_frame;

	static const wxString period_choices[PERIOD_T_SEASON];

	/**Handles date change triggered by the user*/
	void OnDateChange(wxCommandEvent &event);
	/**Handles draw change triggered by the user*/
	void OnDrawChange(wxCommandEvent &event);
	/**Handles period change triggered by the user*/
	void OnPeriodChange(wxCommandEvent &event);

	void OnHelpButton(wxCommandEvent &event);

	public:	
	XYDialog(wxWindow *parent, wxString prefix, ConfigManager *cfg, DatabaseManager *db, XYFrame *frame);
	/**Called by @see DataMangler when data has been fetched*/
	void DataFromMangler(XYGraph *graph);
	/**Start data fetching*/
	void OnOK(wxCommandEvent &event);
	/**Hides window*/
	void OnCancel(wxCommandEvent &event);
	/**Sets focus on first button in windows, allowing for tab traversal*/
	void OnShow(wxShowEvent &event);
	/**@return selected X graph DrawInfo*/
	DrawInfo* GetXDraw() const { return m_xdraw; }
	/**@return selected X graph DrawInfo*/
	DrawInfo* GetYDraw() const { return m_ydraw; }
	/**@return selected period*/
	PeriodType GetPeriod() const { return m_period; }
	/**@return selected start time*/
	wxDateTime GetStartTime() const { return m_start_time.GetTime(); }
	/**@return selected end time*/
	wxDateTime GetEndTime() const { return m_end_time.GetTime(); }

	void ConfigurationIsAboutToReload(wxString prefix);

	void ConfigurationWasReloaded(wxString prefix);

#if 0
	virtual wxDragResult SetDrawInfo(wxCoord x, wxCoord y, DrawInfo *draw_info, wxDragResult def);
#endif

	virtual ~XYDialog();

	DECLARE_EVENT_TABLE()
};

#endif
