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
 * reporter3 program
 * SZARP
 *
 * ecto@praterm.com.pl
 * pawel@praterm.com.pl
 */

#include <cmath>

#include <wx/progdlg.h>
#include <wx/config.h>

#include "htmlabout.h"

#include "repapp.h"
#include "reporter_view.h"
#include "reporter_controller.h"
#include "report_edit.h"
#include "szframe.h"
#include "userreports.h"

#define KEY_FONTSIZE	_T("Reporter/FontSize")
const int MIN_FONTSIZE	= 6;
const int DEFAULT_FONTSIZE	= 9;
const int MAX_FONTSIZE = 16;

DEFINE_EVENT_TYPE(REPORT_DATA_RESP)
DEFINE_EVENT_TYPE(START_EVENT)
DEFINE_EVENT_TYPE(STOP_EVENT)

typedef void (wxEvtHandler::*ReportDataResponseEventFunction)(ReportDataResponse&);

#define EVT_REPORT_DATA_RESP(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY( REPORT_DATA_RESP, id, -2, \
    (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction) \
    wxStaticCastEvent( ReportDataResponseEventFunction, & fn ), (wxObject *) NULL ),


#include "bmp_start.xpm"
#include "bmp_grey.xpm"
#include "bmp_green1.xpm"
#include "bmp_green2.xpm"
#include "bmp_yellow1.xpm"
#include "bmp_yellow2.xpm"
#include "bmp_red1.xpm"
#include "bmp_red2.xpm"

ReporterWindow::ReporterWindow(): wxFrame(NULL, wxID_ANY, _("Reporter")) {
	SetEvtHandlerEnabled(true);
#ifdef MINGW32
	m_skip_onsize = true;
#endif

	stat_sb = new wxStatusBar(this, wxID_ANY);
	const int widths[] = { -1, 75 };
	stat_sb->SetFieldsCount(2, widths);

	if (szFrame::default_icon.IsOk()) {
		SetIcon(szFrame::default_icon);
	}
	
	SetStatusBar(stat_sb);
	
	m_report_menu = new wxMenu();  
	system_reports = new wxMenu();
	user_reports = new wxMenu();
	m_menu_template = new wxMenu();
	wxMenu *menu_option = new wxMenu();
	wxMenu *menu_help = new wxMenu();
	wxMenuBar *menu_bar = new wxMenuBar();
	wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
	params_listc = new wxListCtrl(this, ID_L_ITEMSELECT,
			wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);

	m_report_menu->Append(ID_M_REPORT_STARTSTOP, _("&Start\tSpace"));
	m_report_menu->AppendSeparator();

	m_report_menu->Append(ID_M_REPORT_EXIT, _("E&xit\tAlt-X"));
	
	m_menu_template->Append(ID_M_TEMPLATE_NEW, _("&New\tCtrl-N"));
	m_menu_template->Append(ID_M_TEMPLATE_EDIT, _("&Edit\tCtrl-E"));
	m_menu_template->Append(ID_M_TEMPLATE_DELETE, _("&Delete\tCtrl-D"));

	m_menu_template->AppendSeparator();

	m_menu_template->Append(ID_M_TEMPLATE_LOAD, _("&Import\tCtrl-O"));
	m_menu_template->Append(ID_M_TEMPLATE_SAVE, _("&Export\tCtrl-S"));

	m_menu_template->AppendSeparator();

	m_menu_template->Append(ID_SYSTEM_REPORTS, _("S&ystem"), system_reports);
	m_menu_template->Append(ID_SYSTEM_REPORTS, _("&User templates"), user_reports);
	
	menu_option->Append(ID_M_OPTION_INCFONT, _("&Increase font\t+"));
	menu_option->Append(ID_M_OPTION_DECFONT, _("&Decrease font\t-"));

	menu_help->Append(ID_M_HELP_REP, _("&Help\tF1"));
	menu_help->Append(ID_M_HELP_ABOUT, _("&About"));
	
	menu_bar->Append(m_report_menu, _("&Reports"));
	menu_bar->Append(m_menu_template, _("&Reports"));
	menu_bar->Append(menu_option, _("&Options"));
	menu_bar->Append(menu_help, _("&Help"));
	
	this->SetMenuBar(menu_bar);

	m_menu_template->Enable(ID_M_TEMPLATE_SAVE, false);
	m_menu_template->Enable(ID_M_TEMPLATE_EDIT, false);
	m_menu_template->Enable(ID_M_TEMPLATE_DELETE, false);

	SetReportListFont();
	
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

	m_report_menu->Enable(ID_M_REPORT_START, false);
	wxStaticCast(FindWindowById(ID_B_STARTSTOP), wxBitmapButton)->Enable(false);
	
	params_listc->InsertColumn(0, _("Param"), wxLIST_FORMAT_LEFT, -2);
	params_listc->InsertColumn(1, _("Value"), wxLIST_FORMAT_LEFT, -2);
	params_listc->InsertColumn(2, _("Description"), wxLIST_FORMAT_LEFT, -2);

	this->SetSizer(top_sizer);
	
	this->Show(true);

	m_help = new szHelpController();
	m_help->AddBook(wxGetApp().GetSzarpDir() + 
			_T("resources/documentation/new/reporter/html/reporter.hhp"));

#ifdef MINGW32
	m_skip_onsize = false;
#endif

	m_timer = new wxTimer(this);
	m_timer->Start(1000);
}

void ReporterWindow::AssignController(std::shared_ptr<ReportControllerBase> controller) {
	reporter = controller;
}

void ReporterWindow::SetReportListFont() {
	wxFont font = params_listc->GetFont();

	int size = 0;
	wxConfig::Get()->Read(KEY_FONTSIZE, &size);
	if ((size < MIN_FONTSIZE) || (size > MAX_FONTSIZE)) {
		size = DEFAULT_FONTSIZE;
	}
	font.SetPointSize(size);
	params_listc->SetFont(font);
}

void ReporterWindow::LoadUserReports(std::vector<ReportInfo>& reports_to_load)
{
	int start = system_reports->GetMenuItemCount() + ID_SYSTEM_REPORTS + 1;
	int count = user_reports->GetMenuItemCount();

	for (int i = start; i < count + start; i++) {
		user_reports->Delete(i);
	}

	if (reports_to_load.size() == 0) {
		// TODO (cannot get this to work under wx 2.8)
	} else {
		for (unsigned int i = 0; i < reports_to_load.size(); ++i) {
			user_reports->Append(start + i, wxString(reports_to_load[i].name));
		}
	}
}

void ReporterWindow::LoadSystemReports(std::vector<ReportInfo>& reports_to_load)
{
	int start = system_reports->GetMenuItemCount() + ID_SYSTEM_REPORTS + 1;
	int count = system_reports->GetMenuItemCount();

	for (int i = start; i < count + start; i++) {
		system_reports->Delete(i);
	}

	if (reports_to_load.size() == 0) {
		system_reports->Append(start, _("(None)"));
		system_reports->Enable(start, false);
	} else {
		for (unsigned int i = 0; i < reports_to_load.size(); ++i) {
			system_reports->Append(start + i, wxString(reports_to_load[i].name));
		}
	}
}

bool ReporterWindow::AskOverwrite()
{
	return wxMessageBox(
		wxString(_("User report template with the same name already exists"))
			+ _(". Do you want to replace existing template?"), 
		_("Report overwrite"), 
		wxICON_QUESTION | wxYES | wxNO,
		this) != wxNO;
}

void ReporterWindow::OnSystemReport(wxCommandEvent &ev) 
{
	Stop(ev);
	const int report_id = ev.GetId()-ID_SYSTEM_REPORTS-1;
	reporter->ShowReport(report_id);
}

void ReporterWindow::OnExit(wxCommandEvent& WXUNUSED(ev))
{
	OnExitDo();
}

void ReporterWindow::OnExitDo() 
{
	Destroy();
}

void ReporterWindow::OnClose(wxCloseEvent &ev)
{
	OnExitDo();
}

void ReporterWindow::onSubscriptionError(const std::wstring& error)
{
	Stop(L"");
}

void ReporterWindow::onSubscriptionStarted()
{
	Start();
}

const wxString ReporterWindow::GetFilePath() const {
	wxFileDialog dlg(NULL, _("Choose a file"), _T(""), _T(""),
		_("SZARP parameters list (*.xpl)|*.xpl|All files (*.*)|*.*"),
		wxSAVE | wxCHANGE_DIR); 

	if (dlg.ShowModal() != wxID_OK)
		return wxEmptyString;

	return dlg.GetPath();
}

void ReporterWindow::OnReportSave(wxCommandEvent &ev) 
{
	auto path = GetFilePath();
	if (path == wxEmptyString) return;

	if (path.Right(4).Find('.') < 0) {
		path += _T(".xpl");
	}

	wxFileName file(path);
	std::string file_string = std::string(path.mb_str());
	reporter->SaveCurrentReport(file_string);
}

void ReporterWindow::OnReportLoad(wxCommandEvent &ev) 
{
	auto path = GetFilePath();
	if (path == wxEmptyString) return;

	reporter->LoadReportFromFile(std::string(path.mb_str()));
}


void ReporterWindow::OnReportNew(wxCommandEvent &ev) 
{
	reporter->NewReport();
	
	wxStaticCast(FindWindowById(ID_B_STARTSTOP), wxBitmapButton)->Enable(true);
	m_report_menu->Enable(ID_M_REPORT_START, true);
}

void ReporterWindow::OnReportEdit(wxCommandEvent &ev) 
{
	reporter->EditReport();
}

void ReporterWindow::OnReportDelete(wxCommandEvent &ev) 
{
	if (wxMessageBox(_("Are you sure you want to remove current report?"), 
				_("Delete user report"),
				wxICON_QUESTION | wxYES_NO,
				this) == wxNO) {
		return;
	}
	
	ClearParams();
	reporter->RemoveCurrentReport();

	SetTitle(_T("Reporter "));
	SetStatusText(L"");

	m_menu_template->Enable(ID_M_TEMPLATE_SAVE, false);
	m_menu_template->Enable(ID_M_TEMPLATE_EDIT, false);
	m_menu_template->Enable(ID_M_TEMPLATE_DELETE, false);

	m_report_menu->Enable(ID_M_REPORT_STARTSTOP, false);
	wxStaticCast(FindWindowById(ID_B_STARTSTOP), wxBitmapButton)->Enable(false);

	wxStaticCast(FindWindowById(ID_B_STARTSTOP), wxBitmapButton)->SetBitmapLabel(
			m_but_bmps[0]);
	m_report_menu->FindItem(ID_M_REPORT_STARTSTOP)->SetText(_("Start\tSpace"));
}

void ReporterWindow::OnFontSize(wxCommandEvent &ev)
{
	int dif = 1;
	if (ev.GetId() == ID_M_OPTION_DECFONT) {
		dif = -1;
	}
	ChangeFontSize(dif);
	FitSize();
}

void ReporterWindow::ChangeFontSize(int dif)
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

void ReporterWindow::OnHelpRep(wxCommandEvent &ev) 
{
	 m_help->DisplayContents();
}

void ReporterWindow::OnHelpAbout(wxCommandEvent &ev) 
{
	wxGetApp().ShowAbout();
}

void ReporterWindow::Start() {
	wxCommandEvent event(START_EVENT, GetId());
	wxPostEvent(this, event);
}

void ReporterWindow::Stop(const std::wstring& msg) {
	wxCommandEvent event(STOP_EVENT, GetId());
	event.SetString(wxString(msg));
	wxPostEvent(this, event);
}

void ReporterWindow::Stop(wxCommandEvent &ev)
{
	// Doesn't need mutex as WX guarantees this will be processed non-concurrently
	if (!is_running) return;

	is_running = false;

	wxStaticCast(FindWindowById(ID_B_STARTSTOP), wxBitmapButton)->SetBitmapLabel(
			m_but_bmps[0]);
	m_report_menu->FindItem(ID_M_REPORT_STARTSTOP)->SetText(_("Start\tSpace"));

	const wxString evt_string = ev.GetString();
	if (!evt_string.IsEmpty()) {
		wxMessageBox(evt_string, _("Server error."), wxOK | wxICON_ERROR, wxGetApp().GetTopWindow());
	}

	reporter->OnStop();
}

void ReporterWindow::Start(wxCommandEvent &ev) {
	is_running = true;
	wxStaticCast(FindWindowById(ID_B_STARTSTOP), wxBitmapButton)->SetBitmapLabel(
			m_but_bmps[1]);
	m_report_menu->FindItem(ID_M_REPORT_STARTSTOP)->SetText(_("Stop\tSpace"));
	reporter->OnStart();
}

void ReporterWindow::OnStartStop(wxCommandEvent &ev) 
{
	if (is_running) {
		Stop(ev);
	} else {
		Start(ev);
	}
}

void ReporterWindow::UpdateValue(const std::wstring& param, const double val)
{
	ReportDataResponse r(param, val);
	wxPostEvent(this, r);
}

void ReporterWindow::OnData(ReportDataResponse& resp) {
	double val = resp.value;
	unsigned int p_no = params[resp.param_name];
	const ParamInfo& pi = current_report.params[p_no];

	// wx doesnt format doubles well (use wxNumberFormatter with wx 2.9+)
	std::ostringstream d_ss;
	d_ss.precision(pi.prec);
	d_ss << std::fixed << val / pow(10, pi.prec);
	std::string formated_val = d_ss.str();

	params_listc->Freeze();
	params_listc->SetItem(p_no, 1, wxString(formated_val.c_str(), wxConvUTF8));
	params_listc->Thaw();
}

void ReporterWindow::SetStatusText(const std::wstring& rname)
{
	stat_sb->SetStatusText(wxString(rname), 0);
}

void ReporterWindow::ClearParams()
{
	params_listc->DeleteAllItems();
}

void ReporterWindow::AddParam(const ParamInfo& param, unsigned int i)
{
	wxString unit = wxString(param.unit);
	if (unit.IsEmpty())
		unit = wxT("-");

	params_listc->InsertItem(i, wxString(param.short_name));
	params_listc->SetItem(i, 1, _("---"));
	params_listc->SetItem(i, 2, wxString(param.description) + _T(" [") + unit + _T("]"));
}


void ReporterWindow::ShowReport(ReportInfo& report)
{
	params_listc->Freeze();

	int col0_width = params_listc->GetColumnWidth(0);
	int col1_width = params_listc->GetColumnWidth(1);	
	int col2_width = params_listc->GetColumnWidth(2);	
	if (col0_width == -2) col0_width = -1;
	if (col1_width == -2) col1_width = -1;
	if (col2_width == -2) col2_width = -1;

	DrawParamsList(report);

	params_listc->SetColumnWidth(0, col0_width);
	params_listc->SetColumnWidth(1, col1_width);
	params_listc->SetColumnWidth(2, col2_width);

	int total_width = 0, dummy = -1;
	params_listc->GetSize(&total_width, &dummy);
	total_width = total_width - params_listc->GetColumnWidth(0) - params_listc->GetColumnWidth(1);
	if (total_width - params_listc->GetColumnWidth(2) < 0)
		total_width = -1;

	params_listc->SetColumnWidth(2, total_width);

	if (!report.is_system) {
		m_menu_template->Enable(ID_M_TEMPLATE_SAVE, true);
		m_menu_template->Enable(ID_M_TEMPLATE_EDIT, true);
		m_menu_template->Enable(ID_M_TEMPLATE_DELETE, true);
	} else {
		m_menu_template->Enable(ID_M_TEMPLATE_SAVE, false);
		m_menu_template->Enable(ID_M_TEMPLATE_EDIT, false);
		m_menu_template->Enable(ID_M_TEMPLATE_DELETE, false);
	}

	m_report_menu->Enable(ID_M_REPORT_STARTSTOP, true);
	wxStaticCast(FindWindowById(ID_B_STARTSTOP), wxBitmapButton)->Enable(true);

	FitSize();

	params_listc->Thaw();
}

void ReporterWindow::DrawParamsList(const ReportInfo& report)
{
	params.clear();
	ClearParams();

	current_report = report;

	unsigned int i = 0;
	for (const auto& param: report.params) {
		params[param.name] = i;
		AddParam(param, i);
		++i;
	}

	SetStatusText(report.name);
}

void ReporterWindow::FitSize()
{
	int c = params_listc->GetItemCount();
	if (c <= 0) {
		return;
	}

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

	params_listc->SetColumnWidth(2, -1);
	dc.GetTextExtent(_("Description"), &w, &h);
	if (params_listc->GetColumnWidth(2) < (w + 20)) {
		params_listc->SetColumnWidth(2, w + 20);
	}
}

void ReporterWindow::ShowError(const char* error) const {
	wxString msg = _("Error occurred: ") + wxString::FromUTF8(error);
	wxMessageBox(msg, _("Operation failed."), wxOK | wxICON_ERROR, wxGetApp().GetTopWindow());
}

void ReporterWindow::OnTimer(wxTimerEvent &ev) {}


IMPLEMENT_CLASS(ReporterWindow, wxFrame)
BEGIN_EVENT_TABLE(ReporterWindow, wxFrame)
	EVT_MENU_RANGE(ID_SYSTEM_REPORTS, ID_SYSTEM_REPORTS+MAX_REPORTS_NUMBER, 
			ReporterWindow::OnSystemReport)
	EVT_MENU(ID_M_REPORT_STARTSTOP, ReporterWindow::OnStartStop)
	EVT_COMMAND(wxID_ANY, STOP_EVENT, ReporterWindow::Stop)
	EVT_COMMAND(wxID_ANY, START_EVENT, ReporterWindow::Start)
	EVT_MENU(ID_M_REPORT_EXIT, ReporterWindow::OnExit)
	EVT_MENU(ID_M_TEMPLATE_SAVE, ReporterWindow::OnReportSave)
	EVT_MENU(ID_M_TEMPLATE_LOAD, ReporterWindow::OnReportLoad)
	EVT_MENU(ID_M_TEMPLATE_NEW, ReporterWindow::OnReportNew)
	EVT_MENU(ID_M_TEMPLATE_EDIT, ReporterWindow::OnReportEdit)
	EVT_MENU(ID_M_TEMPLATE_DELETE, ReporterWindow::OnReportDelete)
	EVT_MENU(ID_M_OPTION_INCFONT, ReporterWindow::OnFontSize)
	EVT_MENU(ID_M_OPTION_DECFONT, ReporterWindow::OnFontSize)
	EVT_MENU(ID_M_HELP_REP, ReporterWindow::OnHelpRep)
	EVT_MENU(ID_M_HELP_ABOUT, ReporterWindow::OnHelpAbout)
	EVT_BUTTON(ID_B_STARTSTOP, ReporterWindow::OnStartStop)
	EVT_REPORT_DATA_RESP(wxID_ANY, ReporterWindow::OnData)
	EVT_TIMER(-1, ReporterWindow::OnTimer)
	EVT_CLOSE(ReporterWindow::OnClose)
END_EVENT_TABLE()

