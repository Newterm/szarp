/* $Id$
 *
 * kontroler3 program
 * SZARP

 * ecto@praterm.com.pl
 */

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <cmath>

#include <wx/listctrl.h>
#include <wx/list.h>

#include "libpar.h"

#include "htmlview.h"
#include "htmlabout.h"
#include "authdiag.h"

#include "fetchparams.h"
#include "parlist.h"

#include "kontradd.h"
#include "kontropt.h"

#include "kontr.h"
#include "kontroler.h"
#include "cconv.h"
#include "kontralarm.h"
#include "authdiag.h"
#include "szapp.h"


#include <wx/listimpl.cpp>
WX_DEFINE_LIST(szProbeList);

#define XKON_DEFAULT _T("./kon.xkon")

#define CONTROL_RAPORT _T("control_raport")

szKontroler::szKontroler(wxWindow *par, bool remote_mode, bool operator_mode, wxString server) :
  wxFrame(par, wxID_ANY, _("Kontroler")) {
  ipk = NULL;
  yellowcol= new wxColour(wxString(_("YELLOW")));

  m_http = new szHTTPCurlClient();

  loaded = false;
  bool ask_for_server=false;
  while (ipk == NULL) {
	server = szServerDlg::GetServer(server, _T("Kontroler"), ask_for_server);
	if (server == wxEmptyString ||
	    server.IsSameAs(_T("Cancel")))
		exit(0);
	ipk = szServerDlg::GetIPK(server, m_http);
	if (ipk == NULL)
		ask_for_server = true;
  }

  loaded = true;
  m_editing = false;
  m_server = server;
  m_period = 10;
  m_log_len = 100;
  m_pfetcher = NULL;
  m_apd = NULL;
  m_play_sounds = true;
  stat_sb = new wxStatusBar(this, wxID_ANY);
  const int widths[] = { 50, -1, 75 };
  stat_sb->SetFieldsCount(3, widths);

  SetStatusBar(stat_sb);

  wxMenu *menu_param = new wxMenu();
  menu_param->Append(ID_M_RAPOPT_SER, _("Ser&ver"));
  menu_param->AppendSeparator();
  menu_param->Append(ID_M_PARAM_LOAD, _("&Load"));
  menu_param->Append(ID_M_PARAM_SAVE, _("&Save"));
  menu_param->AppendSeparator();
  menu_param->Append(ID_M_PARAM_ADD, _("&Add"));
  menu_param->Append(ID_M_PARAM_CHANGE, _("&Change"));
  menu_param->Append(ID_M_PARAM_DEL, _("&Delete"));
#if 0
  menu_param->Append(ID_M_PARAM_LIST, _("&List"));
#endif
  if (remote_mode && operator_mode) {
  	menu_param->AppendSeparator();
  	menu_param->Append(ID_M_AUTH, _("Set &username and password"));
  }
  menu_param->AppendSeparator();
  menu_param->Append(ID_M_PARAM_EXIT, _("E&xit"));
  
  wxMenu *menu_rapopt = new wxMenu();
  menu_rapopt->AppendCheckItem(ID_M_RAPOPT_GR5, _("Type &5 (red)"));
  menu_rapopt->AppendCheckItem(ID_M_RAPOPT_GR4, _("Type &4 (yellow)"));
  menu_rapopt->AppendCheckItem(ID_M_RAPOPT_GR3, _("Type &3"));
  menu_rapopt->AppendCheckItem(ID_M_RAPOPT_GR2, _("Type &2"));
  menu_rapopt->AppendCheckItem(ID_M_RAPOPT_GR1, _("Type &1"));
  menu_rapopt->AppendSeparator();
  menu_rapopt->Append(ID_M_RAPOPT_OPT, _("&Options"));

  for (int i = ID_M_RAPOPT_GR1; i <= ID_M_RAPOPT_GR5; i++)
	  menu_rapopt->Check(i, true);

  wxMenu *menu_help = new wxMenu();
  menu_help->Append(ID_M_HELP_KON, _("&Kontroler"));
  menu_help->Append(ID_M_HELP_ABOUT, _("&About"));

  wxMenuBar *menu_bar = new wxMenuBar();
  if (!remote_mode || operator_mode)
     menu_bar->Append(menu_param, _("&Params"));
  menu_bar->Append(menu_rapopt, _("&Report Options"));
  menu_bar->Append(menu_help, _("&Help"));

  this->SetMenuBar(menu_bar);

  wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);

  params_listc = new wxListCtrl(this, LIST_CTRL, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);

  top_sizer->Add(params_listc,
    1, wxALIGN_TOP | wxGROW);

  //top_sizer->SetSizeHints(this);


  params_listc->InsertColumn(0, _("#"), wxLIST_FORMAT_LEFT, 30);
  params_listc->InsertColumn(1, _("Name"), wxLIST_FORMAT_LEFT, 500);
  params_listc->InsertColumn(2, _("Value"), wxLIST_FORMAT_LEFT, 80);
  params_listc->InsertColumn(3, _("Min"), wxLIST_FORMAT_LEFT, 70);
  params_listc->InsertColumn(4, _("Max"), wxLIST_FORMAT_LEFT, 70);
  params_listc->InsertColumn(5, _("Value"), wxLIST_FORMAT_LEFT, 80);
  params_listc->InsertColumn(6, _("Type"), wxLIST_FORMAT_LEFT, 50);
#if 0
  params_listc->InsertColumn(6, _("Level"), wxLIST_FORMAT_LEFT, 50);
  params_listc->InsertColumn(7, _("Group"), wxLIST_FORMAT_LEFT, 50);
#endif

  m_pfetcher = new szParamFetcher(server, this, m_http);
  szParList plist = GetParList();
  m_pfetcher->GetParams().RegisterIPK(ipk);
  m_pfetcher->SetPeriod(m_period);

  m_remote_mode = remote_mode;
  m_operator_mode = operator_mode;

  if (remote_mode)
       m_pfetcher->SetURL(CONTROL_RAPORT);
  else
       m_pfetcher->SetSource(m_probes, ipk);

  //FIXME:
  if (m_pfetcher->IsAlive())
    m_pfetcher->Resume();
  else
    m_pfetcher->Run();

  m_apd = new szKontrolerAddParam(this->ipk, this, wxID_ANY, _("Kontroler->AddParam"));
  
  SetSizer(top_sizer);
  SetSize(wxSize(900, 300));
  Show(true);

  /* Load sounds */
  wxString sound_dir = wxGetApp().GetSzarpDir() + _T("resources/sounds/");
  for (int i = 0; i < 3; i++) {
	  m_sounds[i].Create(sound_dir + SOUND_FILE_INFO);
  }
  m_sounds[3].Create(sound_dir + SOUND_FILE_WARN);
  m_sounds[4].Create(sound_dir + SOUND_FILE_ALARM);

}

szKontroler::~szKontroler() {
  //delete m_pfetcher;
  delete m_apd;
}

void szKontroler::ProcessValue(double& val, szProbe *probe) {

	bool is_alarm;

	if (probe->m_value_check)
		is_alarm = std::isnan(val);
	else {
		if (std::isnan(val))
			//TODO: DON"T WHAT TO DO HERE
			return;

		if (val < probe->m_value_min)
			is_alarm = true;
		else if (val > probe->m_value_max)
			is_alarm = true;
		else
			is_alarm = false;
	}


	if (is_alarm) {
		if (m_alarms.find(probe->m_parname) != m_alarms.end())
			return;

		szAlarmWindow* aw = new szAlarmWindow(this, val, probe);
		m_alarms_windows[probe->m_parname] = aw;
		aw->Show();
		if (m_play_sounds && m_sounds[probe->m_alarm_type - 1].IsOk()) {
			m_sounds[probe->m_alarm_type - 1].Play();
		}

		m_alarms[probe->m_parname] = RAISED;
	} else {
		AlarmsHash::iterator ai;
		if ((ai = m_alarms.find(probe->m_parname)) == m_alarms.end())
			return;

		AlarmStatus status = ai->second;
		if (status == RAISED) {
			szAlarmWindow* aw = m_alarms_windows[probe->m_parname];
			aw->Destroy();
			m_alarms_windows.erase(probe->m_parname);
		}

		m_alarms.erase(ai);

	}
}

void szKontroler::ConfirmAlarm(wxString param_name) {
	AlarmWindowsHash::iterator wi;
	if ((wi = m_alarms_windows.find(param_name)) != m_alarms_windows.end()) {
		szAlarmWindow* aw = wi->second;
		aw->Destroy();
		m_alarms_windows.erase(wi);
	}

	AlarmsHash::iterator ai;
	if ((ai = m_alarms.find(param_name)) != m_alarms.end())
		ai->second = CONFIRMED;

}

szParList szKontroler::GetParList() {
  szParList parlist;
  parlist.RegisterIPK(this->ipk);
  parlist.AddNs(_T("kon"), SZ_CONTROL_NS_URI);

  for (wxszProbeListNode *node = m_probes.GetFirst();
      node;
      node = node->GetNext()) {
    szProbe *probe = node->GetData();
    if (probe == NULL)
      continue;
    size_t i = parlist.Append( probe->m_param );

#if 0
    parlist.SetExtraProp(i, SZ_CONTROL_NS_URI, _T("name"),
        probe->m_parname);
#endif

    parlist.SetExtraProp(i, SZ_CONTROL_NS_URI, _T("formula"),
        probe->m_formula?_T("t"):_T("f"));
    parlist.SetExtraProp(i, SZ_CONTROL_NS_URI, _T("value_check"),
        probe->m_value_check?_("t"):_T("f"));

    parlist.SetExtraProp(i, SZ_CONTROL_NS_URI, _T("value_min"),
        wxString::Format(_T("%f"), probe->m_value_min));
    parlist.SetExtraProp(i, SZ_CONTROL_NS_URI, _T("value_max"),
        wxString::Format(_T("%f"), probe->m_value_max));

    parlist.SetExtraProp(i, SZ_CONTROL_NS_URI, _T("data_period"),
        wxString::Format(_T("%d"), probe->m_data_period));
    parlist.SetExtraProp(i, SZ_CONTROL_NS_URI, _T("precision"),
        wxString::Format(_T("%d"), probe->m_precision));
    parlist.SetExtraProp(i, SZ_CONTROL_NS_URI, _T("alarm_type"),
        wxString::Format(_T("%d"), probe->m_alarm_type));
#if 0
    parlist.SetExtraProp(i, SZ_CONTROL_NS_URI, _T("alarm_group"),
        wxString::Format(_T("%d"), probe->m_alarm_group));
#endif

    parlist.SetExtraProp(i, SZ_CONTROL_NS_URI, _T("alt_name"),
        probe->m_alt_name);
  }

#if 0
  xmlDocDump(stderr, parlist.GetXML());
#endif

  return parlist;

}

void szKontroler::SuckParListToProbes(szParList& parlist, bool values) {
  m_probes.Clear();

  for (unsigned int i = 0; i < parlist.Count(); i++ ) {
    TParam *par = parlist.GetParam(i);
    if (par == NULL)
      continue;

    bool
      formula = (parlist.GetExtraProp(i, SZ_CONTROL_NS_URI, _T("formula")) == _T("t")),
      value_check = (parlist.GetExtraProp(i, SZ_CONTROL_NS_URI, _T("value_check")) == _T("t"));

    double value_min, value_max;
    parlist.GetExtraProp(i, SZ_CONTROL_NS_URI, _T("value_min")).ToDouble(&value_min);
    parlist.GetExtraProp(i, SZ_CONTROL_NS_URI, _T("value_max")).ToDouble(&value_max);

    long data_period, precision, alarm_type, alarm_group;

    parlist.GetExtraProp(i, SZ_CONTROL_NS_URI, _T("data_period")).ToLong(&data_period);
    parlist.GetExtraProp(i, SZ_CONTROL_NS_URI, _T("precision")).ToLong(&precision);
    parlist.GetExtraProp(i, SZ_CONTROL_NS_URI, _T("alarm_type")).ToLong(&alarm_type);
    parlist.GetExtraProp(i, SZ_CONTROL_NS_URI, _T("alarm_group")).ToLong(&alarm_group);

    wxString
      parname = par->GetName(),
      alt_name = parlist.GetExtraProp(i, SZ_CONTROL_NS_URI, _T("alt_name"));

    m_probes.Append(
        new szProbe(parname, par,
          formula, value_check,
          value_min, value_max,
          data_period, precision,
          alarm_type, alarm_group,
          alt_name)
        );
  }
}

void szKontroler::OnAuthData(wxCommandEvent &event) {
	AuthInfoDialog *dlg = new AuthInfoDialog(this);
	if (dlg->ShowModal() == wxID_OK) {
		m_username = dlg->GetUsername();
		m_password = dlg->GetPassword();
	}
	dlg->Destroy();
}

void szKontroler::RegisterControlRaport(szParList &parlist) {
	if (m_username.IsEmpty() || m_password.IsEmpty()) {
		AuthInfoDialog *dlg = new AuthInfoDialog(this);
		if (dlg->ShowModal() != wxID_OK)
			return;

		m_username = dlg->GetUsername();
		m_password = dlg->GetPassword();
		dlg->Destroy();
	}

	std::basic_string<unsigned char> pwd = SC::S2U(m_username + _T(":") + m_password);
	bool ret = m_pfetcher->RegisterControlRaport(CONTROL_RAPORT, parlist, (char*)pwd.c_str());
	if (ret == false)
		wxMessageBox(_("Failed to set control raport (server connection error or bad username/password)"), _("Error"), wxICON_ERROR);

}

void szKontroler::LoadKonFile(const wxString &fname) {

  szParList parlist;
  parlist.RegisterIPK(this->ipk);
  if ( parlist.LoadFile(fname) == 0 ) {
    wxLogMessage(_T("kon: load failed"));
    return;
  }
  SuckParListToProbes(parlist, false);
  if (m_remote_mode)
     RegisterControlRaport(parlist);
  else
     m_pfetcher->SetSource(m_probes, ipk);

  RefreshReport();
}

void szKontroler::SaveKonFile(const wxString &fname) {
  szParList parlist = GetParList();;
  parlist.SaveFile(fname);
}

void szKontroler::OnParamLoad(wxCommandEvent &ev) {
  wxString fn = wxFileSelector(_("Load params: choose a file"),
    _T(""), _T(""), _T("*.xkon"), _("Kontorler params (*.xkon)|*.xkon|All files (*)|*"),
    wxOPEN, this);
  if (fn.empty()) {
    wxLogMessage(_T("kon: not loading"));
  } else {
    wxLogMessage(_T("kon: loading %s"), fn.c_str());
    LoadKonFile(fn);
  }
}

void szKontroler::OnParamSave(wxCommandEvent &ev) {
  wxString fn = wxFileSelector(_("Save params: choose a file"),
    _T(""), _T(""), _T("*.xkon"), _("Kontorler params (*.xkon)|*.xkon|All files (*)|*"),
    wxSAVE | wxOVERWRITE_PROMPT, this);

  if (fn.empty()) {
    wxLogMessage(_T("kon: not saveing"));
  } else {
    wxLogMessage(_T("kon: saveing %s"), fn.c_str());
    SaveKonFile(fn);
  }
}

void szKontroler::OnParamAdd(wxCommandEvent &ev) {
  //g_data = null
  szProbe *probe = new szProbe();
  m_apd->g_data.m_probe.Set(*probe);

  if ( m_apd->ShowModal() == wxID_OK ) {
    //list += g_data()

    probe->Set(m_apd->g_data.m_probe);
    m_probes.Append(probe);

    wxLogMessage(_T("par_add: ok (\"%s\", %s, %d, %d, %f, %f, %d, %d, %d, %d, \"%s\")"),
        probe->m_parname.c_str(),
        wxString((probe->m_param != NULL)?(probe->m_param->GetShortName()):L"(none)").c_str(),
        probe->m_formula, probe->m_value_check,
        probe->m_value_min, probe->m_value_max,
        probe->m_data_period, probe->m_precision,
        probe->m_alarm_type, probe->m_alarm_group,
        probe->m_alt_name.c_str());

     if (m_remote_mode) {
	szParList pl = GetParList();
        RegisterControlRaport(pl);
     } else
        m_pfetcher->SetSource(m_probes, ipk);
     RefreshReport();
  } else {
    wxLogMessage(_T("par_add: cancel"));
    delete probe;
  }
}

void szKontroler::OnListSel(wxListEvent &ev) {
  if (m_remote_mode && !m_operator_mode)
	  return;

  m_editing = true;

  long i = -1;
  while ( (i = params_listc->GetNextItem(i,
          wxLIST_NEXT_ALL,
          wxLIST_STATE_SELECTED)) != -1 ) {
    //act = list.get_selection()
    wxszProbeListNode *node = m_probes.Item(i);
    szProbe *probe = node->GetData();

    wxLogMessage(_T("par_ch: %s"), probe->m_param->GetName().c_str());
    m_apd->g_data.m_probe.Set(*probe);

    if (m_apd->ShowModal() == wxID_OK) {
      //act() = g_data
      probe->Set(m_apd->g_data.m_probe);
      wxLogMessage(_T("par_ch: ok (\"%s\", %s, %d, %d, %f, %f, %d, %d, %d, %d, \"%s\")"),
          probe->m_parname.c_str(),
          wxString((probe->m_param != NULL) ? (probe->m_param->GetShortName()) : L"(none)").c_str(),
          probe->m_formula, probe->m_value_check,
          probe->m_value_min, probe->m_value_max,
          probe->m_data_period, probe->m_precision,
          probe->m_alarm_type, probe->m_alarm_group,
          probe->m_alt_name.c_str());

      if (m_remote_mode) {
	szParList pl = GetParList();
        RegisterControlRaport(pl);
      } else
        m_pfetcher->SetSource(m_probes, ipk);
      RefreshReport();
    } else {
      wxLogMessage(_T("par_ch: cancel"));
    }

  }//while

  m_editing = false;

}

void szKontroler::OnParamChange(wxCommandEvent &ev) {

  long i = -1;
  m_editing = true;
  while ( (i = params_listc->GetNextItem(i,
          wxLIST_NEXT_ALL,
          wxLIST_STATE_SELECTED)) != -1 ) {
    //act = list.get_selection()
    wxszProbeListNode *node = m_probes.Item(i);
    szProbe *probe = node->GetData();

    wxLogMessage(_T("par_ch: %s"), probe->m_param->GetName().c_str());
    m_apd->g_data.m_probe.Set(*probe);

    if ( m_apd->ShowModal() == wxID_OK ) {
      //act() = g_data
      probe->Set(m_apd->g_data.m_probe);
      wxLogMessage(_T("par_ch: ok (\"%s\", %s, %d, %d, %f, %f, %d, %d, %d, %d, \"%s\")"),
          probe->m_parname.c_str(),
          wxString((probe->m_param != NULL)?(probe->m_param->GetShortName()):L"(none)").c_str(),
          probe->m_formula, probe->m_value_check,
          probe->m_value_min, probe->m_value_max,
          probe->m_data_period, probe->m_precision,
          probe->m_alarm_type, probe->m_alarm_group,
          probe->m_alt_name.c_str());

     if (m_remote_mode) {
         szParList pl = GetParList();
         RegisterControlRaport(pl);
     } else
         m_pfetcher->SetSource(m_probes, ipk);
      RefreshReport();
    }else {
      wxLogMessage(_T("par_ch: cancel"));
    }

  }//while
  m_editing = false;
}

void szKontroler::OnParamDel(wxCommandEvent &ev) {
  //wxLogMessage("par_del: %d selected", params_listc->GetSelectedItemCount());
  long i = -1;
  while ( (i = params_listc->GetNextItem(i,
          wxLIST_NEXT_ALL,
          wxLIST_STATE_SELECTED)) != -1 ) {
    //act = list.get_selection()
    wxszProbeListNode *node = m_probes.Item(i);
    szProbe *probe = node->GetData();

    wxLogMessage(_T("par_del: %s"),
        wxString(probe->m_param->GetName()).c_str());

    //list -= act
    delete probe;
    node->SetData(NULL);
  }

  wxszProbeListNode *node = m_probes.GetFirst();
  while (node)
    if (node->GetData() == NULL) {
      wxszProbeListNode *tnode = node->GetNext();
      m_probes.DeleteNode(node);
      node = tnode;
    } else {
      node = node->GetNext();
    }
  
  if (m_remote_mode) {
      szParList pl = GetParList();
      RegisterControlRaport(pl);
  } else
      m_pfetcher->SetSource(m_probes, ipk);
  RefreshReport();
}

void szKontroler::OnParamList(wxCommandEvent &ev) {
  //????
}

void szKontroler::OnExit(wxCommandEvent &ev) {
  if (wxMessageBox(_("You really wan't end?"), _("Kontroler->End"),
        wxOK | wxCANCEL, this) == wxOK) {
    SaveKonFile(XKON_DEFAULT);
    this->Destroy();
  }
}

void szKontroler::OnRapoptGroup(wxCommandEvent &ev) {
	RefreshListControl();
}

void szKontroler::OnClose(wxCloseEvent &event) {
	if (event.CanVeto()) {
		if (m_remote_mode && !m_operator_mode) {
			event.Veto();
			return;
		}
		
		if (wxMessageBox(_("You really wan't end?"), _("Kontroler->End"), wxOK | wxCANCEL, this) == wxOK) {
			if (!m_remote_mode)
				SaveKonFile(XKON_DEFAULT);
			Destroy();
  		} else {
			event.Veto();
		}
	} else {
	    Destroy();
	}

}


void szKontroler::OnRapoptSer(wxCommandEvent &ev) {
    wxString server;
    TSzarpConfig* local_ipk = NULL;
    while (local_ipk == NULL) {
        server = szServerDlg::GetServer(m_server, _T("Kontroler"), true);
        if (server == _T("Cancel")) return;
        local_ipk = szServerDlg::GetIPK(server, m_http);
    }

    this->ipk = local_ipk;
    m_pfetcher->Pause();
    m_pfetcher->ResetBase(m_server);

    m_pfetcher = new szParamFetcher(server, this, m_http);

	m_pfetcher->SetPeriod(m_period);
	m_pfetcher->SetSource(m_probes,ipk);
	m_probes.Clear();
	m_pfetcher->Run();
	//params_listc->Freeze();
	params_listc->DeleteAllItems();
	delete m_apd;
        m_apd = new szKontrolerAddParam(ipk, this, wxID_ANY, _("Kontroler->AddParam"));
}

void szKontroler::OnRapoptOpt(wxCommandEvent &ev) {
  szKontrolerOpt kod(this, wxID_ANY, _("Kontroler->Options"));
  kod.g_data.m_per_sec = m_period%60;
  kod.g_data.m_per_min = m_period/60;
  kod.g_data.m_log_len = m_log_len;
  if ( kod.ShowModal() == wxID_OK ) {
    m_period = kod.g_data.m_per_sec + kod.g_data.m_per_min*60;
    m_log_len = kod.g_data.m_log_len;
    m_pfetcher->SetPeriod(m_period);
    wxLogMessage(_T("kon_opt: ok (%d, %d)"), m_period, m_log_len);
  }else {
    wxLogMessage(_T("kon_opt: cancel (%d, %d)"), m_period, m_log_len);
  }
}

void szKontroler::OnHelpKon(wxCommandEvent &ev) {
  HtmlViewerFrame *f = new HtmlViewerFrame(
    _T("/opt/szarp/resources/documentation/new/kontroler/html/kontroler.html"),
    this, _("Raporter - help"),
    wxDefaultPosition, wxSize(600,600));
  f->Show();
}

void szKontroler::OnHelpAbout(wxCommandEvent &ev) {
  HtmlAbout(this, _T("/opt/szarp/resources/wx/html/kontrolerabout.html"));
}

void szKontroler::OnRapdata(wxCommandEvent &ev) {
  wxLogMessage(_T("rap: data"));
  if (m_editing)
     return;

  RefreshReport(true);
}

void szKontroler::RefreshListControl() {
  long selected_item = params_listc->GetNextItem(-1, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);

  params_listc->Freeze();
  params_listc->DeleteAllItems();

  wxszProbeListNode *node = m_probes.GetFirst();
  size_t i = 0;

  for (i = 0; i < m_values.size() && node; node = node->GetNext()) {
    szProbe *probe = node->GetData();
    if (probe == NULL)
      continue;

    if (!GetMenuBar()->IsChecked(probe->m_alarm_type - 1 + ID_M_RAPOPT_GR1))
      continue;

    wxString value_name;
    switch (probe->m_data_period) {
	case  1:
	    value_name = _("minute average");
	    break;
	case  2:
	    value_name = _("10-minutes average");
	    break;
	case  3:
	    value_name = _("hour average");
	    break;
	default:
	    value_name = _("instantenous");
	    break;
    }

    params_listc->InsertItem(i, wxString::Format(_T("%d"), i + 1));
    if (probe->m_alt_name.IsEmpty()) 
	params_listc->SetItem(i, 1, probe->m_parname);
    else
	params_listc->SetItem(i, 1, probe->m_alt_name);

    double a = m_values[i];
    if (std::isnan(a)) 
	params_listc->SetItem(i, 2, _("no data"));
    else
        params_listc->SetItem(i, 2, wxString::Format(_T("%.*f"), probe->m_param->GetPrec(), a));

    params_listc->SetItem(i, 3,
        wxString::Format(_T("%.*f"), probe->m_param->GetPrec(), probe->m_value_min));
    params_listc->SetItem(i, 4,
        wxString::Format(_T("%.*f"), probe->m_param->GetPrec(), probe->m_value_max));
    params_listc->SetItem(i, 5,
        value_name);
    params_listc->SetItem(i, 6,
        wxString::Format(_T("%d"), probe->m_alarm_type));

    if ((probe->m_value_check && std::isnan(a)) || (!std::isnan(a) && (a < probe->m_value_min || a > probe->m_value_max))) {
	if (probe->m_alarm_type == 4)
	    	params_listc->SetItemBackgroundColour(i, (*yellowcol));
	if (probe->m_alarm_type == 5)
	    	params_listc->SetItemBackgroundColour(i, *wxRED);
    }

    i++;
  }

  if (selected_item >= 0) {
	params_listc->SetItemState(selected_item, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
	params_listc->EnsureVisible(selected_item);
  }

  params_listc->Thaw();

}

void szKontroler::RefreshReport(bool complete) {
  if (m_pfetcher->IsValid() == false)
	  return;

  m_pfetcher->Lock();
  if (m_remote_mode)
      SuckParListToProbes(m_pfetcher->GetParams(), true);

  wxszProbeListNode *node = m_probes.GetFirst();

  m_values.clear();

  for (size_t i = 0; i < m_pfetcher->GetParams().Count() && node; i++, node = node->GetNext()) {
    szProbe *probe = node->GetData();
    if (probe == NULL)
      continue;

    wxString data_period = m_pfetcher->GetParams().GetExtraProp(i, SZ_CONTROL_NS_URI, _T("data_period"));
    wxString val_prop = _T("value");
    if (data_period == _T("1")) {
	val_prop = _T("value_minute");
    } else if (data_period == _T("2")) {
	val_prop = _T("value_10minutes");
    }
    else if (data_period == _T("3")) {
	val_prop = _T("value_hour");
    }

    wxString val = m_pfetcher->GetParams().GetExtraProp(i, SZ_REPORTS_NS_URI, val_prop);

    double a;
    if (val.IsSameAs(_T("unknown"))) {
	a = nan("");
    } else {
	val.Replace(_T("."),_T(","),true);
        val.ToDouble(&a);
    }

    ProcessValue(a, probe);
    m_values.push_back(a);
  }

  m_pfetcher->Unlock();

  RefreshListControl();

}

IMPLEMENT_CLASS(szKontroler, wxFrame)
BEGIN_EVENT_TABLE(szKontroler, wxFrame)
  EVT_MENU(ID_M_PARAM_LOAD, szKontroler::OnParamLoad)//
  EVT_MENU(ID_M_PARAM_SAVE, szKontroler::OnParamSave)//
  EVT_MENU(ID_M_PARAM_ADD, szKontroler::OnParamAdd)//*
  EVT_MENU(ID_M_PARAM_CHANGE, szKontroler::OnParamChange)//*
  EVT_MENU(ID_M_PARAM_DEL, szKontroler::OnParamDel)//*
  EVT_MENU(ID_M_PARAM_LIST, szKontroler::OnParamList)//*
  EVT_MENU(ID_M_PARAM_EXIT, szKontroler::OnExit)
  EVT_MENU_RANGE(ID_M_RAPOPT_GR1, ID_M_RAPOPT_GR5, szKontroler::OnRapoptGroup)//*
  EVT_MENU(ID_M_RAPOPT_OPT, szKontroler::OnRapoptOpt)//*
  EVT_MENU(ID_M_RAPOPT_SER, szKontroler::OnRapoptSer)//*
  EVT_MENU(ID_M_HELP_KON, szKontroler::OnHelpKon)
  EVT_MENU(ID_M_HELP_ABOUT, szKontroler::OnHelpAbout)
  EVT_MENU(ID_M_AUTH, szKontroler::OnAuthData)
  EVT_SZ_FETCH(szKontroler::OnRapdata)
  EVT_CLOSE(szKontroler::OnClose)
  EVT_LIST_ITEM_ACTIVATED(LIST_CTRL, szKontroler::OnListSel)
END_EVENT_TABLE()

