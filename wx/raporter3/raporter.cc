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
 *
 * ecto@praterm.com.pl
 * pawel@praterm.com.pl
 */

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/listctrl.h>
#include <wx/event.h>
#include <wx/progdlg.h>
#include <wx/config.h>

#include "htmlabout.h"

#include "raporter.h"
#include "raporterperiod.h"
#include "raporteredit.h"
#include "rap.h"
#include "fetchparams.h"
#include "parlist.h"
#include "cconv.h"
#include "serverdlg.h"
#include "szframe.h"
#include "userreports.h"

#define KEY_FONTSIZE	_T("Raporter/FontSize")
const int MIN_FONTSIZE	= 6;
const int MAX_FONTSIZE = 16;

#include "bmp_start.xpm"
#include "bmp_grey.xpm"
#include "bmp_green1.xpm"
#include "bmp_green2.xpm"
#include "bmp_yellow1.xpm"
#include "bmp_yellow2.xpm"
#include "bmp_red1.xpm"
#include "bmp_red2.xpm"

/** A simple HTTP client subclass that performs locking before attempting to
 * make a transfer.
 */
class szHTTPLockingClient : public szHTTPCurlClient {
public:
	virtual xmlDocPtr GetXML(char* uri, char *userpwd, int timeout = -1)
	{
		wxCriticalSectionLocker locker(m_csec);
		return szHTTPCurlClient::GetXML(uri, userpwd, timeout);
	}
	virtual char* Get(char *uri, size_t* ret_len)
	{
		wxCriticalSectionLocker locker(m_csec);
		return szHTTPCurlClient::Get(uri, ret_len);
	}
	virtual char* Put(char *uri, char *buffer, size_t len,
			size_t* ret_len)
	{
		wxCriticalSectionLocker locker(m_csec);
		return szHTTPCurlClient::Put(uri, buffer, len, NULL, ret_len);
	}
	virtual ~szHTTPLockingClient()
	{
	}
protected:
	wxCriticalSection m_csec;	/* Critical section to lock in */
};


szRaporter::szRaporter(wxWindow *parent, wxString server, wxString title) 
	: wxFrame(parent, wxID_ANY, _("Raporter"))
{
#ifdef MINGW32
	m_skip_onsize = true;
#endif
	ipk = NULL;
	m_server = server;
	stat_sb = new wxStatusBar(this, wxID_ANY);
	const int widths[] = { 75, -1, 50, 75};
	stat_sb->SetFieldsCount(4, widths);
	m_fitsize = false;

	if (szFrame::default_icon.IsOk()) {
		SetIcon(szFrame::default_icon);
	}
	
	SetStatusBar(stat_sb);
	
	m_raport_menu = new wxMenu();  
	m_raport_menu->Append(ID_M_RAPORT_START, _("&Start\tSpace"));
	m_raport_menu->Append(ID_M_OPTION_FILE_DUMP, _("&File dump"));
	m_raport_menu->AppendSeparator();
	m_raport_menu->Append(ID_M_RAPORT_EXIT, _("E&xit\tAlt-X"));
	
	template_ipk_menu = new wxMenu();
	m_menu_user_templates = new wxMenu();
	m_menu_template = new wxMenu();
	m_menu_template->Append(ID_M_TEMPLATE_NEW, _("&New\tCtrl-N"));
	m_menu_template->Append(ID_M_TEMPLATE_EDIT, _("&Edit\tCtrl-E"));
	m_menu_template->Append(ID_M_TEMPLATE_DELETE, _("&Delete\tCtrl-D"));
	m_menu_template->AppendSeparator();
	m_menu_template->Append(ID_M_TEMPLATE_LOAD, _("&Import\tCtrl-O"));
	m_menu_template->Append(ID_M_TEMPLATE_SAVE, _("&Export\tCtrl-S"));
	m_menu_template->AppendSeparator();
	m_menu_template->Append(ID_M_TEMPLATE_IPK, _("S&ystem"), template_ipk_menu);
	m_menu_template->Append(ID_M_TEMPLATE_IPK, _("&User templates"), m_menu_user_templates);
	
	wxMenu *menu_option = new wxMenu();
	menu_option->Append(ID_M_OPTION_SERVER, _("&Server"));
	menu_option->Append(ID_M_OPTION_PERIOD, _("&Period"));
	menu_option->AppendSeparator();
	menu_option->Append(ID_M_OPTION_INCFONT, _("&Increase font\t+"));
	menu_option->Append(ID_M_OPTION_DECFONT, _("&Decrease font\t-"));
	wxMenu *menu_help = new wxMenu();
	menu_help->Append(ID_M_HELP_RAP, _("&Help\tF1"));
	menu_help->Append(ID_M_HELP_ABOUT, _("&About"));
	
	wxMenuBar *menu_bar = new wxMenuBar();
	menu_bar->Append(m_raport_menu, _("&Raports"));
	menu_bar->Append(m_menu_template, _("&Templates"));
	menu_bar->Append(menu_option, _("&Options"));
	menu_bar->Append(menu_help, _("&Help"));
	
	this->SetMenuBar(menu_bar);

	m_menu_template->Enable(ID_M_TEMPLATE_SAVE, false);
	m_menu_template->Enable(ID_M_TEMPLATE_EDIT, false);
	m_menu_template->Enable(ID_M_TEMPLATE_DELETE, false);
	
	wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);

	params_listc = new wxListCtrl(this, ID_L_ITEMSELECT,
			wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
	wxFont font = params_listc->GetFont();
	int size = 9;
	wxConfig::Get()->Read(KEY_FONTSIZE, &size);
	if ((size < MIN_FONTSIZE) || (size > MAX_FONTSIZE)) {
		size = 9;
	}
	font.SetPointSize(size);
	params_listc->SetFont(font);

	
	top_sizer->Add(params_listc,
			1, /* expand vertically */
			wxALIGN_TOP | 
			wxEXPAND /* expand horizontally */
			);
	assert (NUM_OF_BMPS == 7);
	m_but_bmps[0] = wxBitmap(bmp_start_xpm);
	m_but_bmps[1] = wxBitmap(bmp_green1_xpm);
	m_but_bmps[2] = wxBitmap(bmp_green2_xpm);
	m_but_bmps[3] = wxBitmap(bmp_yellow1_xpm);
	m_but_bmps[4] = wxBitmap(bmp_yellow2_xpm);
	m_but_bmps[5] = wxBitmap(bmp_red1_xpm);
	m_but_bmps[6] = wxBitmap(bmp_red2_xpm);
	m_cur_bmp = 0;
	
	top_sizer->Add(new wxBitmapButton(this, ID_B_STARTSTOP, m_but_bmps[0]),
			0,
			wxALIGN_BOTTOM |
			wxEXPAND
		      );
	wxStaticCast(FindWindowById(ID_B_STARTSTOP), wxBitmapButton)->SetBitmapDisabled(
			wxBitmap(bmp_grey_xpm));
	m_raport_menu->Enable(ID_M_RAPORT_START, false);
	wxStaticCast(FindWindowById(ID_B_STARTSTOP), wxBitmapButton)->Enable(false);
	
	m_running = false;
	m_bufsize = 10;
	m_per_type = szPER_ACT;
	m_per_per = 10;
	
	params_listc->InsertColumn(0, _("Param"), wxLIST_FORMAT_LEFT, -2);
	params_listc->InsertColumn(1, _("Value"), wxLIST_FORMAT_LEFT, -2);
	params_listc->InsertColumn(2, _("Description"), wxLIST_FORMAT_LEFT, -2);

	this->SetSizer(top_sizer);
	
	this->Show(true);

	m_help = new szHelpController();
	m_help->AddBook(wxGetApp().GetSzarpDir() + 
			_T("resources/documentation/new/raporter/html/raporter.hhp"));

	m_http = new szHTTPLockingClient();
	m_pfetcher = new szParamFetcher(server, this, m_http);
	m_pfetcher->SetPeriod(m_per_per);
	m_report_name = title;
	
	SetIsTestRaport(title);

	m_loaded = true;

	m_report_ipk = true;
	if (LoadIPK()) {
		if (LoadReportIPK(title)) {
			wxStaticCast(FindWindowById(ID_B_STARTSTOP), wxBitmapButton)->Enable(true);
			m_raport_menu->Enable(ID_M_RAPORT_START, true);
		} else {
			stat_sb->SetStatusText(_("Choose report template"), 1);
		}
	}

	m_timer = new wxTimer(this);
	m_timer->Start(1000);
#ifdef MINGW32
	m_skip_onsize = false;
#endif
}

szRaporter::~szRaporter() 
{
	delete m_timer;
	delete m_http;
	//m_raplist.DeleteContents(true);
	m_raplist.Clear();
	if (ipk)
		delete ipk;
}

void szRaporter::ReloadTemplateMenu() 
{
	int start = template_ipk_menu->GetMenuItemCount() + ID_M_TEMPLATE_IPK + 1;
	int count = m_menu_user_templates->GetMenuItemCount();
	for (int i = start; i < count + start; i++) {
		m_menu_user_templates->Delete(i);
	}

	if (m_ur.m_list.size() == 0) {
		m_menu_user_templates->Append(start, _("(None)"));
		m_menu_user_templates->Enable(start, false);
		return;
	}

	for (unsigned int i = 0; i < m_ur.m_list.size(); i++) {
		m_menu_user_templates->Append(start + i, m_ur.m_list[i]);
	}
}

bool szRaporter::LoadIPK() 
{

	wxString title;
	wxArrayString raports;
	if (!szServerDlg::GetReports(m_server,m_http, title, raports))
		return false;

	m_menu_template->SetLabel(ID_M_TEMPLATE_IPK, title);

	for (unsigned int i = 0; i < template_ipk_menu->GetMenuItemCount(); i++) {
		template_ipk_menu->Delete(ID_M_TEMPLATE_IPK + i + 1);
	}
	for (unsigned int i = 0; i < raports.size(); i++) {
		template_ipk_menu->Append(ID_M_TEMPLATE_IPK + i + 1, raports[i]);
	}

	m_ur.RefreshList(title);
	ReloadTemplateMenu();
	ipk_raps = raports;
	m_menu_template->Enable(ID_M_TEMPLATE_SAVE, false);

	return true;
}

void szRaporter::SetIsTestRaport(wxString report_name) {
	if (report_name.CmpNoCase(_("test report")) == 0)
		m_test_window = true;
	else
		m_test_window = false;
}

int szRaporter::AskOverwrite(wxString config)
{
	if (!config.IsEmpty()) {
		config = wxString(_(" for configuration ")) + _T("\"") + config + _T("\"");
	}
	return wxMessageBox(
				wxString(_("User report template with the same name already exists"))
					+ config 
					+ _(". Do you want to replace existing template?"), 
				_("Template overwrite"), 
				wxICON_QUESTION | wxYES | wxNO,
				this);
}

bool szRaporter::LoadReportIPK(const wxString &rname) 
{
	if (rname.IsEmpty()) {
		return false;
	}
	m_menu_template->Enable(ID_M_TEMPLATE_SAVE, false);
	m_menu_template->Enable(ID_M_TEMPLATE_EDIT, false);
	m_menu_template->Enable(ID_M_TEMPLATE_DELETE, false);
	m_raplist.Clear();
	m_pfetcher->SetReportName(rname);
	m_report_name = rname;
	SetTitle(_T("Raporter: ") + m_report_name);
	SetIsTestRaport(m_report_name);
	m_report_ipk = true;
	RefreshReport();
	SetFitSize();
	return m_pfetcher->IsValid();
}

void szRaporter::LoadReportFile(const wxString &fname) 
{
	szParList parlist;

	if (this->ipk == NULL) {
		this->ipk = szServerDlg::GetIPK(m_server, m_http);
		if(this->ipk == NULL)
			return;
	}

	parlist.RegisterIPK(this->ipk);
	if (parlist.LoadFile(fname, true) <= 0) {
		return;
	}
	
	m_raplist.Clear();
	m_report_ipk = false;
	
	wxString report_name = parlist.GetExtraRootProp(SZ_REPORTS_NS_URI, _T("title"));
	wxString config_found;
	if (fname.IsEmpty() and m_ur.FindTemplate(report_name, config_found)) {
		if (AskOverwrite(config_found) == wxNO) {
			return;
		}
	}

	m_report_name = report_name;
	m_raplist = parlist;
	
	if (m_pfetcher->SetSource(m_raplist)) {
		SetTitle(_T("Raporter: ") + m_report_name);
		SetIsTestRaport(m_report_name);
		m_ur.SaveTemplate(m_report_name, m_raplist);
		m_ur.RefreshList();
		RefreshReport();
		SetFitSize();
		wxStaticCast(FindWindowById(ID_B_STARTSTOP), wxBitmapButton)->Enable(true);
		m_raport_menu->Enable(ID_M_RAPORT_START, true);
		m_menu_template->Enable(ID_M_TEMPLATE_SAVE, true);
		m_menu_template->Enable(ID_M_TEMPLATE_EDIT, true);
		m_menu_template->Enable(ID_M_TEMPLATE_DELETE, true);
	} else {
		Stop();
		RefreshReport(true);
		wxStaticCast(FindWindowById(ID_B_STARTSTOP), wxBitmapButton)->Enable(false);
		m_raport_menu->Enable(ID_M_RAPORT_START, false);
		m_menu_template->Enable(ID_M_TEMPLATE_SAVE, false);
		m_menu_template->Enable(ID_M_TEMPLATE_EDIT, false);
		m_menu_template->Enable(ID_M_TEMPLATE_DELETE, false);
		wxMessageBox(_("Registering report on current server failed. Template does not contain valid report."), 
				_("Incorrect report template"), 
				wxICON_ERROR | wxOK, 
				this);
	}
}

void szRaporter::SaveReportFile(const wxString &fname) 
{
	m_raplist.AddNs(_T("rap"), SZ_REPORTS_NS_URI);
	m_raplist.SetExtraRootProp(SZ_REPORTS_NS_URI, _T("title"), m_report_name);
	m_raplist.SaveFile(fname);
}


void szRaporter::OnRapIPK(wxCommandEvent &ev) 
{
	if (ev.GetId() < (int)ipk_raps.size() + ID_M_TEMPLATE_IPK + 1) {
		if (LoadReportIPK(ipk_raps[ev.GetId()-ID_M_TEMPLATE_IPK-1])) {
			wxStaticCast(FindWindowById(ID_B_STARTSTOP), wxBitmapButton)->Enable(true);
			m_raport_menu->Enable(ID_M_RAPORT_START, true);
		}
	} else {
		wxString path = m_ur.GetTemplatePath(m_ur.m_list[
				ev.GetId() - ID_M_TEMPLATE_IPK - 1 - ipk_raps.size()
				]);
		LoadReportFile(path);
		m_ur.RefreshList();
		ReloadTemplateMenu();
	}
}



void szRaporter::OnExit(wxCommandEvent& WXUNUSED(ev))
{
	OnExitDo();
}

void szRaporter::OnExitDo() 
{
	m_pfetcher->Delete();
	Destroy();
}

void szRaporter::OnClose(wxCloseEvent &ev)
{
	OnExitDo();
}

void szRaporter::OnTimer(wxTimerEvent &ev)
{
	if (!m_running) {
		return;
	}
	wxBitmapButton* but = wxStaticCast(FindWindowById(ID_B_STARTSTOP), wxBitmapButton);
	if (!m_pfetcher->IsValid()) {
		m_raport_menu->Enable(ID_M_RAPORT_START, false);
		but->Enable(false);
		return;
	} else {
		m_raport_menu->Enable(ID_M_RAPORT_START, true);
		but->Enable(true);
	}
	
	time_t t = time(NULL);
	int index = (t - m_last_data) / m_pfetcher->GetPeriod() * 2 + 1;
	if (index >= (NUM_OF_BMPS - 2)) {
		index = NUM_OF_BMPS - 2;
	}
	m_cur_bmp = index + t % 2;
	but->SetBitmapLabel(m_but_bmps[m_cur_bmp]);
}

void szRaporter::OnTemplateSave(wxCommandEvent &ev) 
{
	SaveReportFile(wxEmptyString);
}

void szRaporter::OnTemplateLoad(wxCommandEvent &ev) 
{
	LoadReportFile(wxEmptyString);
	ReloadTemplateMenu();
}

void szRaporter::OnTemplateNew(wxCommandEvent &ev) 
{
	if(this->ipk == NULL) {
		this->ipk = szServerDlg::GetIPK(m_server, m_http);
		m_raplist.RegisterIPK(ipk);
	}

	if(this->ipk == NULL)
		return;

	szRaporterEdit ed(this->ipk, this, wxID_ANY, _("Raporter->Editor"));
	
	ed.g_data.m_raplist.Clear();
	ed.g_data.m_report_name.Clear();
	
	if (ed.ShowModal() != wxID_OK) {
		return;
	}
	wxString config_found;
	while (m_ur.FindTemplate(ed.g_data.m_report_name, config_found)) {
		if (AskOverwrite(config_found) == wxNO) {
			if (ed.ShowModal() != wxID_OK) {
				return;
			}
		} else {
			break;
		}
	}

	m_report_ipk = false;
	m_raplist.Clear();
	m_raplist = ed.g_data.m_raplist;
	m_report_name = ed.g_data.m_report_name;

	SetTitle(_T("Raporter: ") + m_report_name);
	SetIsTestRaport(m_report_name);
	
	m_ur.SaveTemplate(m_report_name, m_raplist);

	m_pfetcher->SetSource(m_raplist);

	wxStaticCast(FindWindowById(ID_B_STARTSTOP), wxBitmapButton)->Enable(true);
	RefreshReport();
	SetFitSize();
	m_menu_template->Enable(ID_M_TEMPLATE_SAVE, true);
	m_menu_template->Enable(ID_M_TEMPLATE_EDIT, true);
	m_menu_template->Enable(ID_M_TEMPLATE_DELETE, true);
	ReloadTemplateMenu();
}

void szRaporter::OnTemplateEdit(wxCommandEvent &ev) 
{
	if(this->ipk == NULL) {
		this->ipk = szServerDlg::GetIPK(m_server, m_http);
		if(this->ipk == NULL)
			return;
		m_raplist.RegisterIPK(ipk);
	}

	szRaporterEdit ed(this->ipk, this, wxID_ANY, _("Raporter->Editor"));
	
	ed.g_data.m_report_name = m_report_name;
	wxString oldname = m_report_name;
	ed.g_data.m_raplist = m_raplist;
	if ( ed.ShowModal() != wxID_OK ) {
		return;
	}
	wxString config_found;
	while (ed.g_data.m_report_name.Cmp(oldname) and 
			m_ur.FindTemplate(ed.g_data.m_report_name, config_found)) {
		if (AskOverwrite(config_found) == wxNO) {
			if (ed.ShowModal() != wxID_OK) {
				return;
			}
		} else {
			break;
		}
	}

	m_report_ipk = false;
	m_raplist.Clear();
	m_raplist = ed.g_data.m_raplist;
	m_report_name = ed.g_data.m_report_name;
	SetTitle(_T("Raporter: ") + m_report_name);
	SetIsTestRaport(m_report_name);
	if (m_report_name.Cmp(oldname) != 0) {
		m_ur.RemoveTemplate(oldname);
	}
	m_ur.SaveTemplate(m_report_name, m_raplist);
	m_pfetcher->SetSource(m_raplist);
	RefreshReport();
	SetFitSize();
	ReloadTemplateMenu();
}

void szRaporter::OnTemplateDelete(wxCommandEvent &ev) 
{
	if (wxMessageBox(_("Are you sure you want to remove current report?"), 
				_("Delete user report"),
				wxICON_QUESTION | wxYES_NO,
				this) == wxNO) {
		return;
	}
	m_raplist.Clear();
	m_ur.RemoveTemplate(m_report_name);
	m_report_name.Empty();
	SetTitle(_T("Raporter "));
	Stop();
	RefreshReport(true);
	ReloadTemplateMenu();
	m_raport_menu->Enable(ID_M_RAPORT_START, false);
	wxStaticCast(FindWindowById(ID_B_STARTSTOP), wxBitmapButton)->Enable(false);
}



void szRaporter::OnOptFileDump(wxCommandEvent &ev)
{
	FileDumpDialog *d = new FileDumpDialog(this, &m_report_name);
	d->ShowModal();
}

void szRaporter::OnOptServer(wxCommandEvent &ev)
{
	wxString server = szServerDlg::GetServer(m_server, _T("Raporter"), true );
	if (server.IsEmpty() || !server.Cmp(_T("Cancel")) || !server.Cmp(m_server)) {
		return;
	}
	if (m_running) {
		OnStartStop(ev);
	}
	m_pfetcher->ResetBase(m_server);
		
	m_server = server;
	m_report_name = wxEmptyString;
	m_loaded = false;
	ipk = NULL;
	m_report_ipk = true;

	LoadIPK();
	m_pfetcher = new szParamFetcher(m_server, this, m_http);
	m_pfetcher->SetPeriod(m_per_per);
	m_raplist.Clear();
	SetTitle(_T("Raporter"));
	m_test_window = false;
	RefreshReport(true);
	SetFitSize();
	wxStaticCast(FindWindowById(ID_B_STARTSTOP), wxBitmapButton)->Enable(false);
}


void szRaporter::OnOptPeriod(wxCommandEvent &ev) 
{  
	szRaporterPeriod pd(this, wxID_ANY, _("Raporter->Period"));
	pd.g_data.m_type = m_per_type;
	pd.g_data.m_per_s = m_per_per%60;
	pd.g_data.m_per_m = (m_per_per/60)%60;
	pd.g_data.m_per_h = m_per_per/3600;
	if ( pd.ShowModal() == wxID_OK ) {
		m_per_type = pd.g_data.m_type;
		m_per_per = pd.g_data.m_per_s
			+ pd.g_data.m_per_m * 60
			+ pd.g_data.m_per_h * 3600;
		wxLogMessage(_T("opt_period: ok(%d, %d)"), m_per_type, m_per_per);
		//restarting timer with new time
		m_pfetcher->SetPeriod(m_per_per);
	} else {
		wxLogMessage(_T("opt_period: cancel"));
	}
}

#if 0
void szRaporter::OnOptBuf(wxCommandEvent &ev) {
  szRaporterBuf bd(this, wxID_ANY, _("Raporter->Buffer"));
  bd.g_data.m_bufsize = m_bufsize;
  if ( bd.ShowModal() == wxID_OK ) {
    m_bufsize = bd.g_data.m_bufsize;
    wxLogMessage(_T("opt_buf: ok (%d)"), m_bufsize);
  }else {
    wxLogMessage(_T("opt_buf: cancel (%d)"), m_bufsize);
  }
}
#endif

void szRaporter::OnFontSize(wxCommandEvent &ev)
{
	int dif = 1;
	if (ev.GetId() == ID_M_OPTION_DECFONT) {
		dif = -1;
	}
	ChangeFontSize(dif);
	FitSize();
}

void szRaporter::ChangeFontSize(int dif)
{
	assert (params_listc);
	wxFont f = params_listc->GetFont();
	int s = f.GetPointSize() + dif;
	if (s >= MIN_FONTSIZE && s <= MAX_FONTSIZE) {
		f.SetPointSize(s);
		params_listc->SetFont(f);
		wxConfig::Get()->Write(KEY_FONTSIZE, s);
	}
}

void szRaporter::OnHelpRap(wxCommandEvent &ev) 
{
	 m_help->DisplayContents();
}

void szRaporter::OnHelpAbout(wxCommandEvent &ev) 
{
	wxGetApp().ShowAbout();
}

void szRaporter::OnRapdata(wxCommandEvent &ev) 
{
	m_last_data = time(NULL);
	RefreshReport(true);
	if (m_fitsize) {
		FitSize();
	}
}

void szRaporter::Stop()
{
	m_running = false;
	m_pfetcher->Pause();
	m_cur_bmp = 0;
	wxStaticCast(FindWindowById(ID_B_STARTSTOP), wxBitmapButton)->SetBitmapLabel(
			m_but_bmps[0]);
	m_raport_menu->FindItem(ID_M_RAPORT_START)->SetText(_("Start\tSpace"));
}

void szRaporter::OnStartStop(wxCommandEvent &ev) 
{
	if ( m_running ) {
		Stop();
	} else {
		m_pfetcher->ResetTicker();
		m_last_data = time(NULL);
		if(!m_report_ipk)
			m_pfetcher->SetSource(m_raplist);
		if (m_pfetcher->IsAlive()) {
			m_pfetcher->Resume();
		} else {
			m_pfetcher->Run();
		}
		RefreshReport();
		m_running = true;
		m_cur_bmp = time(NULL) % 2 + 1;
		wxStaticCast(FindWindowById(ID_B_STARTSTOP), wxBitmapButton)->SetBitmapLabel(
				m_but_bmps[1]);
		m_raport_menu->FindItem(ID_M_RAPORT_START)->SetText(_("Stop\tSpace"));
	}
}

void szRaporter::RefreshReport(bool force) 
{
	stat_sb->SetStatusText( m_report_ipk?_("System"):_("Custom"), 0 );
	stat_sb->SetStatusText(m_report_name, 1);
	
	if (!m_running && !force) {
		return ;
	}
	params_listc->Freeze();
	params_listc->DeleteAllItems();
	int col0_width = params_listc->GetColumnWidth(0);
	int col1_width = params_listc->GetColumnWidth(1);	
	int col2_width = params_listc->GetColumnWidth(2);	
	if (col0_width == -2) col0_width = -1;
	if (col1_width == -2) col1_width = -1;
	if (col2_width == -2) col2_width = -1;
	
	m_pfetcher->Lock();
	if (m_pfetcher->IsValid()) {
		if (m_report_ipk || m_raplist.Count() == m_pfetcher->GetParams().Count()) {
			for (size_t i = 0; i < m_pfetcher->GetParams().Count(); i++) {
				wxString scut = m_pfetcher->GetParams().GetExtraProp(i, SZ_REPORTS_NS_URI, _T("short_name"));

				wxString desc;
				if (m_report_ipk)
					desc = m_pfetcher->GetParams().GetExtraProp(i, SZ_REPORTS_NS_URI, _T("description"));
				else 
					desc = m_raplist.GetExtraProp(i, SZ_REPORTS_NS_URI, _T("description"));

				if (desc.IsEmpty()) {
					desc = m_raplist.GetName(i);
					desc = desc.AfterLast(':');
				}

				wxString unit = m_pfetcher->GetParams().GetExtraProp(i, SZ_REPORTS_NS_URI, _T("unit"));
				wxString val = m_pfetcher->GetParams().GetExtraProp(i, SZ_REPORTS_NS_URI, _T("value"));
	
				if (m_test_window == false) {	
					//TODO sorting by order
					params_listc->InsertItem(i, scut);
					params_listc->SetItem(i, 1, val.CmpNoCase(_T("unknown"))? val : wxString(_("no data")));
					params_listc->SetItem(i, 2, desc + _T(" [") +
					      (unit.IsEmpty() ? _T("-") : unit ) + _T("]"));
				} else {
					params_listc->InsertItem(i, _T("-"));
					if (val.CmpNoCase(_T("unknown")) != 0)
						params_listc->SetItem(i,1, _T("ok"));
					else
						params_listc->SetItem(i,1, _("no data"));
					params_listc->SetItem(i, 2, desc);
				}
				if (scut == m_selected_parameter)
					params_listc->SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);		
			}
			stat_sb->SetStatusText(wxString::Format(_T("%d s"),this->m_per_per), 2);
			stat_sb->SetStatusText(wxDateTime::Now().FormatTime(), 3);
			params_listc->SetColumnWidth(0, col0_width);
			params_listc->SetColumnWidth(1, col1_width);
			params_listc->SetColumnWidth(2, col2_width);
			int total_width = 0, dummy = -1;
			params_listc->GetSize(&total_width, &dummy);
			total_width = total_width - params_listc->GetColumnWidth(0) - params_listc->GetColumnWidth(1);
			if (total_width - params_listc->GetColumnWidth(2) < 0)
				total_width = -1;
			params_listc->SetColumnWidth(2, total_width);
	
			if(m_pfetcher->GetParams().Count() > 0)
				FileDump::DumpValues(&m_pfetcher->GetParams(),m_report_name);
		}
	}
	m_pfetcher->Unlock();
	params_listc->Thaw();
	
}

void szRaporter::FitSize()
{
	assert (params_listc != NULL);
	int c = params_listc->GetItemCount();
	if (c <= 0) {
		return;
	}
	params_listc->Freeze();
	m_fitsize = false;
	wxRect ir;
	params_listc->GetItemRect(0, ir);
	wxCoord w, h;
	wxClientDC dc(params_listc);
	params_listc->SetColumnWidth(0, -1);
	dc.SetFont(params_listc->GetFont());
	dc.GetTextExtent(_("Param"), &w, &h);
	if (params_listc->GetColumnWidth(0) < (w + 20)) {
		params_listc->SetColumnWidth(0, w+ 20);
	}
	params_listc->SetColumnWidth(1, -1);
	dc.GetTextExtent(_("Value"), &w, &h);
	if (params_listc->GetColumnWidth(1) < (w + 20)) {
		params_listc->SetColumnWidth(1, w + 20);
	}
	params_listc->Thaw();
}

void szRaporter::SetFitSize()
{
	m_fitsize = true;
}

void szRaporter::OnListItemSelect(wxListEvent &event)
{
	wxListItem item = event.GetItem();
	m_selected_parameter = item.GetText();
}

IMPLEMENT_CLASS(szRaporter, wxFrame)
BEGIN_EVENT_TABLE(szRaporter, wxFrame)
	EVT_MENU_RANGE(ID_M_TEMPLATE_IPK, ID_M_TEMPLATE_IPK+64, 
			szRaporter::OnRapIPK)
	EVT_MENU(ID_M_RAPORT_START, szRaporter::OnStartStop)
	EVT_MENU(ID_M_RAPORT_EXIT, szRaporter::OnExit)
	EVT_MENU(ID_M_TEMPLATE_SAVE, szRaporter::OnTemplateSave)
	EVT_MENU(ID_M_TEMPLATE_LOAD, szRaporter::OnTemplateLoad)
	EVT_MENU(ID_M_TEMPLATE_NEW, szRaporter::OnTemplateNew)
	EVT_MENU(ID_M_TEMPLATE_EDIT, szRaporter::OnTemplateEdit)
	EVT_MENU(ID_M_TEMPLATE_DELETE, szRaporter::OnTemplateDelete)
	EVT_MENU(ID_M_OPTION_FILE_DUMP, szRaporter::OnOptFileDump)
	EVT_MENU(ID_M_OPTION_SERVER, szRaporter::OnOptServer)
	EVT_MENU(ID_M_OPTION_PERIOD, szRaporter::OnOptPeriod)
	EVT_MENU(ID_M_OPTION_INCFONT, szRaporter::OnFontSize)
	EVT_MENU(ID_M_OPTION_DECFONT, szRaporter::OnFontSize)
	EVT_MENU(ID_M_HELP_RAP, szRaporter::OnHelpRap)
	EVT_MENU(ID_M_HELP_ABOUT, szRaporter::OnHelpAbout)
	EVT_BUTTON(ID_B_STARTSTOP, szRaporter::OnStartStop)
//	EVT_SIZE(szRaporter::OnSize)
	EVT_SZ_FETCH(szRaporter::OnRapdata)
	EVT_CLOSE(szRaporter::OnClose)
	EVT_TIMER(-1, szRaporter::OnTimer)
	EVT_LIST_ITEM_SELECTED(ID_L_ITEMSELECT, szRaporter::OnListItemSelect)
END_EVENT_TABLE()

