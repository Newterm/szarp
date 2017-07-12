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
/* $Id$
 *
 * reporter4 program
 * SZARP

 * ecto@praterm.com.pl
 * pawel@praterm.com.pl
 */

#ifndef __REPORTER_VIEW_H__
#define __REPORTER_VIEW_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <map>
#include <mutex>

#include <wx/wxprec.h>
#include <wx/listctrl.h>
#include <wx/event.h>
#include <wx/statusbr.h>

#include "../common/szhlpctrl.h"

#include "report.h"

/** Number of different bitmaps drawn on main button. */
#define NUM_OF_BMPS	7
#define MAX_REPORTS_NUMBER 1024

class ReportControllerBase;
class ReportDataResponse;

BEGIN_DECLARE_EVENT_TYPES()
DECLARE_EVENT_TYPE(REPORT_DATA_RESP, -1);
DECLARE_EVENT_TYPE(START_EVENT, -1);
DECLARE_EVENT_TYPE(STOP_EVENT, -1);
END_DECLARE_EVENT_TYPES()

/** Main Rporter window */
class ReporterWindow: public wxFrame {
	DECLARE_CLASS(ReporterWindow)
	DECLARE_EVENT_TABLE()

	// this has to be type-erased as wx won't accept templated calss
	std::shared_ptr<ReportControllerBase> reporter;

public:
	ReporterWindow();
	void AssignController(std::shared_ptr<ReportControllerBase> controller);

protected: // Event handling
	/** event: menu: report->ipk->* */
	void OnSystemReport(wxCommandEvent &ev);

	/** event: menu: report->exit */
	void OnExit(wxCommandEvent& WXUNUSED(ev));

	/** OnExit() without arguments */
	void OnExitDo();

	/** EVT_CLOSE handler, calls OnExit() */
	void OnClose(wxCloseEvent &ev);

	/** event: menu: template->save */
	void OnReportSave(wxCommandEvent &ev);

	/** event: menu: template->load */
	void OnReportLoad(wxCommandEvent &ev);

	/** event: menu: template->new */
	void OnReportNew(wxCommandEvent &ev);

	/** event: menu: template->delete */
	void OnReportDelete(wxCommandEvent &ev);

	/** event: menu: template->edit */
	void OnReportEdit(wxCommandEvent &ev);

	/** event: menu: option->increase/decrease font size */
	void OnFontSize(wxCommandEvent &ev);

	/** Increase/decrease params list font size by dif */
	void ChangeFontSize(int dif);

	/** event: menu: help->reporter */
	void OnHelpRep(wxCommandEvent &ev);

	/** event: menu: help->about */
	void OnHelpAbout(wxCommandEvent &ev);

	/** event: button: start/stop */
	void OnStartStop(wxCommandEvent &ev);

	void OnData(ReportDataResponse&);

	/** Modify size of widget to show all items currently in list. 
	 * Should be called only when list is properly initialized - 
	 * use SetFitSize(). */
	void FitSize();

	const wxString GetFilePath() const;

	void SetReportListFont();

	void AddParam(const ParamInfo& param, unsigned int i);

	void Start(wxCommandEvent &ev);
	void Stop(wxCommandEvent &ev);

public:
	void Start();
	void Stop(const std::wstring& msg = L"");

	bool AskOverwrite();

	void AddUserReport(ReportInfo& report);

	void LoadUserReports(std::vector<ReportInfo>&);
	void LoadSystemReports(std::vector<ReportInfo>&);

	void ShowReport(ReportInfo& report);

	void onSubscriptionError(const std::wstring& error = L"");
	void onSubscriptionStarted();

	void UpdateValue(const std::wstring& param, const double val);

	void SetStatusText(const std::wstring& rname);
	void ClearParams();
	void DrawParamsList(const ReportInfo& report);

	void ShowError(const char* error) const;

	void OnTimer(wxTimerEvent &ev);

private:
	/**< menus */
	wxMenu *system_reports;
	wxMenu *m_menu_template;
	wxMenu *user_reports;
	wxMenu *m_report_menu;
	szHelpController* m_help;
	wxTimer* m_timer;
	std::mutex list_mutex;

	wxBitmap m_but_bmps[NUM_OF_BMPS];
				/**< array of bitmaps drawn on main button */
	int m_cur_bmp;		/**< index of currenlty displayed bitmap */
#ifdef MINGW32
	bool m_skip_onsize;	/**< windows hack - skip OnSize event handler because
				were are not ready for it... */
#endif

	wxListCtrl *params_listc;
				/**< list control for viewing params */
	wxStatusBar *stat_sb;	/**< status bar */
	std::map<std::wstring, unsigned int> params;

	bool is_running{false};

	ReportInfo current_report{};
};

struct ReportDataResponse: public wxCommandEvent {
	ReportDataResponse(const std::wstring& name, const double val): wxCommandEvent(REPORT_DATA_RESP, wxID_ANY), param_name(name), value(val) {}

	const std::wstring param_name = L"";
	const double value = std::numeric_limits<double>::quiet_NaN();

	/**Clones object*/
	virtual wxEvent* Clone() const { return new ReportDataResponse(*this); }
	virtual ~ReportDataResponse() = default;
};

/** IDs */
enum {
  ID_M_REPORT_SAVE = wxID_HIGHEST+1,
  ID_MY_WINDOW,
  ID_M_REPORT_LOAD,
  ID_M_REPORT_DEL,
  ID_M_REPORT_PRINT,
  ID_M_REPORT_STARTSTOP,
  ID_M_REPORT_START,
  ID_M_REPORT_STOP,
  ID_M_REPORT_EXIT,

  ID_M_TEMPLATE_SAVE,
  ID_M_TEMPLATE_LOAD,
  ID_M_TEMPLATE_NEW,
  ID_M_TEMPLATE_EDIT,
  ID_M_TEMPLATE_DELETE,

  ID_M_OPTION_PERIOD,
  ID_M_OPTION_BUF,
  ID_M_OPTION_INCFONT,
  ID_M_OPTION_DECFONT,

  ID_M_HELP_REP,
  ID_M_HELP_ABOUT,

  ID_B_STARTSTOP,

  ID_TIMER,

  ID_L_ITEMSELECT,
  ID_SYSTEM_REPORTS
};

#endif //_REPORTER_VIEW_H

