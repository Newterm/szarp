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

#include "wxlogging.h"

typedef std::vector<DrawInfo*> DrawInfoList;

typedef std::pair<std::vector<double>, std::vector<DTime> > XYPoint;

/**Struct describing parametrs of XY graph*/
struct XYGraph {
	DrawInfoList m_di;

	std::vector<double> m_dmin;

	std::vector<double> m_dmax;

	std::vector<double> m_min;

	std::vector<double> m_max;

	std::vector<double> m_avg;

	std::vector<double> m_standard_deviation;

	double m_xy_correlation;
	/**Rank correlation of x and y*/
	double m_xy_rank_correlation;

	/**Flag indicates if calculated values were averaged*/
	bool m_averaged;

	/**Start of time range*/
	wxDateTime m_start;
	/**End of time range*/
	wxDateTime m_end;

	/**Period type of probes fetched from db*/
	PeriodType m_period;
	
	/**Graph values*/
	std::deque<XYPoint> m_points_values; 

	std::deque<size_t> m_visible_points;

	XYPoint& ViewPoint(size_t i) {
		return m_points_values.at(m_visible_points.at(i));
	}

	std::deque<std::pair<wxRealPoint, wxRealPoint> > m_zoom_history;

	/**Maps point to the group of points that are drawn on the same pixel*/
	std::map<int, int> point2group;

	/**Groups of points on the same pixel*/
	std::vector< std::list<int> > points_groups;

};

/**Fetches and associated corresponding values with each other*/
class DataMangler : public DBInquirer {
	DrawInfoList m_di;
	/**Type of probes fetches from database*/
	PeriodType m_period;
	/**Start of time range*/
	DTime m_start_time;
	/**Time of least recently fetched probe*/
	DTime m_current_time;
	/**End of time range*/
	DTime m_end_time;
	/**Paired values of params*/
	std::deque<std::pair<std::vector<ValueInfo>, DTime> > m_draw_vals;
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
			const DrawInfoList& di,
			wxDateTime start_time, 
			wxDateTime end_time,
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


class XFrame {
public:
	virtual int GetDimCount() = 0;
	virtual void SetGraph(XYGraph *graph) = 0;
};

class XYDialog : public wxDialog, public ConfigObserver
{
	/**Period selected by user*/
	PeriodType m_period;
	/**Start of range selected by user*/
	DTime m_start_time;
	/**End of range selected by user*/
	DTime m_end_time;
	
	DrawInfoList m_di;

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
	XFrame *m_frame;

	XYGraph *m_graph;

	static const wxString period_choices[PERIOD_T_SEASON];

	/**Handles date change triggered by the user*/
	void OnDateChange(wxCommandEvent &event);
	/**Handles draw change triggered by the user*/
	void OnDrawChange(wxCommandEvent &event);
	/**Handles period change triggered by the user*/
	void OnPeriodChange(wxCommandEvent &event);

	void OnHelpButton(wxCommandEvent &event);

	public:	
	XYDialog(wxWindow *parent, wxString prefix, ConfigManager *cfg, DatabaseManager *db, TimeInfo time, DrawInfoList user_draws,  XFrame *frame);
	/**Called by @see DataMangler when data has been fetched*/
	void DataFromMangler(XYGraph *graph);
	/**Start data fetching*/
	void OnOK(wxCommandEvent &event);
	/**Hides window*/
	void OnCancel(wxCommandEvent &event);
	/**Sets focus on first button in windows, allowing for tab traversal*/
	void OnShow(wxShowEvent &event);
	/**@return selected X graph DrawInfo*/
	DrawInfo* GetXDraw() const { return m_di[0]; }
	/**@return selected X graph DrawInfo*/
	DrawInfo* GetYDraw() const { return m_di[1]; }
	/**@return selected period*/
	PeriodType GetPeriod() const { return m_period; }
	/**@return selected start time*/
	wxDateTime GetStartTime() const { return m_start_time.GetTime(); }
	/**@return selected end time*/
	wxDateTime GetEndTime() const { return m_end_time.GetTime(); }

	void ConfigurationIsAboutToReload(wxString prefix);

	void ConfigurationWasReloaded(wxString prefix);

	XYGraph* GetGraph();

	virtual ~XYDialog();

	DECLARE_LOGGER("xydialog\n")

	DECLARE_EVENT_TABLE()
};

#endif
