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

#ifndef __STATDIAG_H__
#define __STATDIAG_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

typedef std::vector<DrawInfo*> DrawInfoList;

/**Window calculating statistical values of a param*/
class StatDialog : public szFrame, public DBInquirer, public DrawInfoDropReceiver, public ConfigObserver {
	/**Type of period to fetch probes*/
	PeriodType m_period;
	/**Start of time range*/
	DTime m_start_time;
	/**Time of lastest value the request has been sent for*/
	DTime m_current_time;
	/**End of time range*/
	DTime m_end_time;

	wxDateTime m_min_time;

	wxDateTime m_max_time;

	/**Found minimum value*/
	double m_min;
	/**Calculated sum*/
	double m_sum;
	/**Calculate hoursum value*/
	double m_hsum;
	/**Found maximum value*/
	double m_max;
	/**Numbe of no no-data values recevied*/
	int m_count;

	/**Number of value to be fetched*/
	int m_tofetch;
	/**Number of fetched probes*/
	int m_totalfetched;
	/**Number of values which has not yet been fetched*/
	int m_pending;

	/**Current draw for which values are calculted*/
	DrawInfo* m_draw;
	/**Control allowing user to choose type of period*/
	wxChoice *m_period_choice;
	/**Starting configuration, i.e. configuration from which user initially chooses draw*/
	wxString m_default_configuration;
	/**@see ConfigManager*/
	ConfigManager* m_config_manager;

	DatabaseManager* m_database_manager;
	/**@see IncSearch - widgets for choosing draw*/
	IncSearch* m_draw_search;
	/**Widget showing maximum value*/
	wxStaticText *m_max_value_text;
	/**Widget showing minimum value*/
	wxStaticText *m_min_value_text;
	/**Widget showing average value*/
	wxStaticText *m_avg_value_text;
	/**Widget showing hoursum value*/
	wxStaticText *m_hsum_value_text;

	wxString m_param_prefix;

	/**@see ProgressFrame*/
	ProgressFrame* m_progress_frame;

	/**Updates one of range dates*/
	void OnDateChange(wxCommandEvent &event);

	/**Displays @see m_draw_search window*/
	void OnDrawChange(wxCommandEvent &event);

	/**Changes period*/
	void OnPeriodChange(wxCommandEvent &event);

	/**Starts data retrival*/
	void OnCalculate(wxCommandEvent &event);

	/**Closes window*/
	void OnCloseButton(wxCommandEvent &event);

	/**Begins data fetch operation*/
	void StartFetch();

	/**Send certain amount of requests to database*/
	void ProgressFetch();

	void OnHelpButton(wxCommandEvent &event);

	public:	
	StatDialog(wxWindow *parent, wxString prefix, DatabaseManager *db, ConfigManager *cfg, TimeInfo time, DrawInfoList user_draws);

	/**not significant in this case: -1 is returned*/
	virtual time_t GetCurrentTime();

	/**@return current param*/
	virtual DrawInfo* GetCurrentDrawInfo();

	/**Handles response from database. Updates statistical values, progresses fetch process.*/
	virtual void DatabaseResponse(DatabaseQuery *query);

	/**Sets @see DrawInfo window is to be acted upon*/
	void SetDrawInfo(DrawInfo *draw);

	/**Hides window and if event cannot be vetoed - destroys it as weel.*/
	void OnClose(wxCloseEvent &event);

	/**Sets focus on 'first' button in window (so that tab works as expected*/
	void OnShow(wxShowEvent &event);

	void ConfigurationIsAboutToReload(wxString prefix);

	void ConfigurationWasReloaded(wxString prefix);

	/**Handles drop operation. Sets @see m_draw_info to received object.*/
	virtual wxDragResult SetDrawInfo(wxCoord x, wxCoord y, DrawInfo *draw_info, wxDragResult def);

	virtual ~StatDialog();

	DECLARE_EVENT_TABLE()

};

#endif
