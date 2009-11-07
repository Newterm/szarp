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
 * raporter3 program
 * SZARP

 * ecto@praterm.com.pl
 */

#ifndef __RAPORTER_H__
#define __RAPORTER_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/listctrl.h>
#include <wx/dynarray.h>
#include <wx/timer.h>
#include <wx/event.h>
#include <wx/statusbr.h>

#include "szarp_config.h"

#include "fetchparams.h"
#include "szhlpctrl.h"

#include "filedump.h"

/** URI of namespace for extra raporter properties in params lists. */
#define SZ_REPORTS_NS_URI _T("http://www.praterm.com.pl/SZARP/reports")

/** Number of different bitmaps drawn on main button. */
#define NUM_OF_BMPS	7

/** User report entry */
class szRapEntry {
public:
	TParam *m_param;	/**< pointer to parameter */
	wxString m_desc;	/**< description of parameter */
	wxString m_scut;	/**< short name for parameter */
	
	/** */
	szRapEntry() 
	{ 
		Set(NULL, wxEmptyString, wxEmptyString); 
	}
	/** */
	szRapEntry(const szRapEntry &s) 
	{ 
		Set(s); 
	}
	szRapEntry& operator=(const szRapEntry& s)
	{
		if (this != &s) {
			Set(s);
		}
		return *this;
	}
	~szRapEntry()
	{
	}
	/** */
	szRapEntry(TParam *_param, wxString _desc, wxString _scut) 
	{
		Set(_param, _desc, _scut);
	}
	/** */
	inline void Set(TParam *_param, wxString _desc, wxString _scut) 
	{
		m_param = _param; m_desc = _desc; m_scut = _scut;
	}
	/** */
	inline void Set(const szRapEntry &s)
	{
		Set(s.m_param, s.m_desc, s.m_scut);
	}
};

//WX_DECLARE_LIST(szRapEntry, szRapList);

/** Main Raporter window */
class szRaporter : public wxFrame {
	DECLARE_CLASS(szRaporter)
	DECLARE_EVENT_TABLE()
public:
	/**
	 * @param parent parent window, may be NULL
	 * @param server address of server to connect to
	 * @param title title of report to display, may be wxEmptyString
	 */
	szRaporter(wxWindow *parent, wxString server, wxString title);
	/** destructor */
	virtual ~szRaporter();
protected:
	/** event: menu: raport->ipk->* */
	void OnRapIPK(wxCommandEvent &ev);
	/** event: menu: raport->save */
	void OnRapSave(wxCommandEvent &ev);
	/** event: menu: raport->load */
	void OnRapLoad(wxCommandEvent &ev);
	/** event: menu: raport->delete */
	void OnRapDelete(wxCommandEvent &ev);
	/** event: menu: raport->print */
	void OnRapPrint(wxCommandEvent &ev);
	/** event: menu: raport->exit */
	void OnExit(wxCommandEvent& WXUNUSED(ev));
	/** OnExit() without arguments */
	void OnExitDo();
	/** EVT_CLOSE handler, calls OnExit() */
	void OnClose(wxCloseEvent &ev);
	/** event: menu: template->save */
	void OnTemplateSave(wxCommandEvent &ev);
	/** event: menu: template->load */
	void OnTemplateLoad(wxCommandEvent &ev);
	/** event: menu: template->new */
	void OnTemplateNew(wxCommandEvent &ev);
	/** event: menu: template->edit */
	void OnTemplateEdit(wxCommandEvent &ev);
	/** event: menu: option->file dump */
	void OnOptFileDump(wxCommandEvent &ev);
	/** event: menu: option->server */
	void OnOptServer(wxCommandEvent &ev);
	/** event: menu: option->period */
	void OnOptPeriod(wxCommandEvent &ev);
	/** event: menu: option->buffer */
	void OnOptBuf(wxCommandEvent &ev);
	/** event: menu: option->increase/decrease font size */
	void OnFontSize(wxCommandEvent &ev);
	/** Increase/decrease params list font size by dif */
	void ChangeFontSize(int dif);
	/** event: menu: help->raporter */
	void OnHelpRap(wxCommandEvent &ev);
	/** event: menu: help->about */
	void OnHelpAbout(wxCommandEvent &ev);
	/** event: button: start/stop */
	void OnStartStop(wxCommandEvent &ev);
	/** called on resize - sets last column width properly */
	void OnSize(wxSizeEvent &ev);
	/** event: xsltd: fresh data  */
	void OnRapdata(wxCommandEvent &ev);
	/** event: timer */
	void OnTimer(wxTimerEvent &ev);
	/**
	 * Load params.xml from server.
	 * @return true on success, false on error
	 */
	bool LoadIPK();
	/** load report from ipk
	 * @return true if report title is not empty*/
	bool LoadReportIPK(const wxString &rname);
	/** load report from file FIXME:nonimplemented */
	void LoadReportFile(const wxString &fname);
	/** save report to file FIXME:nonimplemented */
	void SaveReportFile(const wxString &fname);
	/** creates param list from raport items list */
	//szParList CreateParamList(szRapList& list);
	void SetIsTestRaport(wxString report_name);
	/**   
	 * Refresh viewed report
	 * @param force true to refresh even if not running
	 */
	void RefreshReport(bool force = false);
	
public:
	bool m_loaded;	/**< is frame initialized */
protected:
	/** Modify size of widget to show all items currently in list. 
	 * Should be called only when list is properly initialized - 
	 * use SetFitSize(). */
	void FitSize();
	/** Set mark to call FitSize() when list info is available. */
	void SetFitSize();
	wxListCtrl *params_listc;
				/**< list control for viewing params */
	wxStatusBar *stat_sb;	/**< status bar */
	bool m_running;		/**< are we refershing? */
	int m_bufsize;		/**< raporter history length 
				FIXME:nonimplemented */
	int m_per_type;		/**< type of data fetched (10s/1m/10m/1h) 
				FIXME:nonimplemented */
	int m_per_per;		/**< report refresh rate */
	wxString m_report_name;	/**< current report name 
				FIXME:what for custom? */
	bool m_report_ipk;	/**< true for report from configuration,
				  false for custom report */
	szParamFetcher* m_pfetcher;
				/**< XSLTD client thread */
	szHTTPCurlClient *m_http;	/**< HTTP client, used also by XSLT client */
	szParList m_raplist;	/**< Current list of raports item. */
	wxString m_server;	/**< path to server */
	TSzarpConfig *ipk;	/**< ipk config */
	wxArrayString ipk_raps;	/**< list of reports provided by ipk */
	wxMenu *template_ipk_menu;
				/**< menu: templates->ipk->* */
	wxMenu *m_menu_template;
				/**< menu: templates->ipk */
	wxMenu *m_raport_menu;	/**< menu: raport */
	szHelpController* m_help;
				/**< help controller */
	bool m_fitsize;		/**< FitSize() mark - if true window size
				  should be adapted. */
	wxTimer* m_timer;	/**< timer - used to update icon on main button */
	wxBitmap m_but_bmps[NUM_OF_BMPS];
				/**< array of bitmaps drawn on main button */
	int m_cur_bmp;		/**< index of currenlty displayed bitmap */
	time_t m_last_data;	/**< time of last successfull fetch, reset on start */
#ifdef MINGW32
	bool m_skip_onsize;	/**< windows hack - skip OnSize event handler because
				were are not ready for it... */
#endif
	bool m_test_window;	/**< is set true if window is a test window */
};

/** IDs */
enum {
  ID_M_RAPORT_SAVE = wxID_HIGHEST+1,
  ID_M_RAPORT_LOAD,
  ID_M_RAPORT_DEL,
  ID_M_RAPORT_PRINT,
  ID_M_RAPORT_START,
  ID_M_RAPORT_EXIT,

  ID_M_TEMPLATE_SAVE,
  ID_M_TEMPLATE_LOAD,
  ID_M_TEMPLATE_NEW,
  ID_M_TEMPLATE_EDIT,

  ID_M_OPTION_FILE_DUMP,
  ID_M_OPTION_SERVER,
  ID_M_OPTION_PERIOD,
  ID_M_OPTION_BUF,
  ID_M_OPTION_INCFONT,
  ID_M_OPTION_DECFONT,

  ID_M_HELP_RAP,
  ID_M_HELP_ABOUT,

  ID_B_STARTSTOP,

  ID_TIMER,

  ID_M_TEMPLATE_IPK
};

/** data type */
enum {
  szPER_ACT =0,
  szPER_1M,
  szPER_10M,
  szPER_1H,
  szPER_1D
};

#endif //_RAPORTER_H
