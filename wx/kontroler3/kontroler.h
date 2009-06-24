/* $Id$
# SZARP: SCADA software 
# 
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 * kontroler3 program
 * SZARP

 * ecto@praterm.com.pl
 */

#ifndef _KONTROLER_H
#define _KONTROLER_H

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/listctrl.h>
#include <wx/statusbr.h>
#include <wx/list.h>
#include <wx/colour.h>
#include <szarp_config.h>
#include <vector>

#include "fetchparams.h"
#include "serverdlg.h"

#define SZ_CONTROL_NS_URI _T("http://www.praterm.com.pl/SZARP/control")
#define SZ_REPORTS_NS_URI _T("http://www.praterm.com.pl/SZARP/reports")

class szKontrolerAddParam;
class szAlarmWindow;

enum AlarmStatus {
	RAISED, 
	CONFIRMED
};

WX_DECLARE_STRING_HASH_MAP(AlarmStatus, AlarmsHash);
WX_DECLARE_STRING_HASH_MAP(szAlarmWindow*, AlarmWindowsHash);

/**
 * Okienko kontrolera
 */
class szKontroler : public wxFrame {
  DECLARE_CLASS(szKontroler)
  DECLARE_EVENT_TABLE()
  /** kontrolka wy¶wietlaj±ca parametry */
  wxListCtrl *params_listc;
  wxStatusBar *stat_sb;
  szParamFetcher *m_pfetcher;
  /** */
  int m_period;
  int m_log_len;
  bool m_editing;
  bool m_remote_mode;
  bool m_operator_mode;
  szProbeList m_probes;
  /** */
  szKontrolerAddParam *m_apd;

  AlarmsHash m_alarms;
  AlarmWindowsHash m_alarms_windows;

  std::vector<double> m_values;

  wxString m_username;
  wxString m_password;

  void RefreshListControl();

public:
  /** czy wystartowali¶my */ //XXX
  bool loaded;
  /** konstruktor */
  szKontroler(wxWindow *par, bool remote_mode, bool operator_mode, wxString server);
  /** */
  virtual ~szKontroler();

  void ConfirmAlarm(wxString param_name);
protected:
  void RegisterControlRaport(szParList &parlist);
  /** menu: param->load */
  void OnParamLoad(wxCommandEvent &ev);
  /** menu: param->save */
  void OnParamSave(wxCommandEvent &ev);
  /** menu: param->add */
  void OnParamAdd(wxCommandEvent &ev);
  /** menu: param->change */
  void OnParamChange(wxCommandEvent &ev);
  void OnListSel(wxListEvent &ev);

  /** menu: param->del */
  void OnParamDel(wxCommandEvent &ev);
  /** menu: param->list */
  void OnParamList(wxCommandEvent &ev);
  /** menu: param->exit */
  void OnExit(wxCommandEvent &ev);
  /** menu: rapopt->group 5..1 */
  void OnRapoptGroup(wxCommandEvent &ev);
  /** menu: rapopt->options */
  void OnRapoptOpt(wxCommandEvent &ev);
  void OnRapoptSer(wxCommandEvent &ev);
  void OnAuthData(wxCommandEvent &event);
  /** menu: help->kontroler */
  void OnHelpKon(wxCommandEvent &ev);
  /** menu: help->about */
  void OnHelpAbout(wxCommandEvent &ev);
  /** ipk config */
  void ProcessValue(double& val, szProbe *probe);
  TSzarpConfig *ipk;
  szHTTPCurlClient *m_http;
  wxString m_server;
  wxColour * yellowcol;
  bool LoadIPK(const wxString &ipk_fname);
  /** event: xsltd: fresh data  */
  void OnRapdata(wxCommandEvent &ev);
  void RefreshReport(bool complete = true);
  void LoadKonFile(const wxString &fname);
  void SaveKonFile(const wxString &fname);
  void SuckParListToProbes(szParList& parlist, bool values);
  void OnClose(wxCloseEvent &event);
  szParList GetParList();
};

/** identyfikatory kontrolek */
enum {
  ID_M_PARAM_LOAD = wxID_HIGHEST + 1,
  ID_M_PARAM_SAVE,
  ID_M_PARAM_ADD,
  ID_M_PARAM_CHANGE,
  ID_M_PARAM_DEL,
  ID_M_PARAM_LIST,
  ID_M_PARAM_EXIT,
  
  ID_M_RAPOPT_GR1,
  ID_M_RAPOPT_GR2,
  ID_M_RAPOPT_GR3,
  ID_M_RAPOPT_GR4,
  ID_M_RAPOPT_GR5,
  ID_M_RAPOPT_OPT,
  ID_M_RAPOPT_SER,

  ID_M_HELP_KON,
  ID_M_HELP_ABOUT,
  ID_M_AUTH,
  LIST_CTRL
};

#endif //_KONTROLER_H
