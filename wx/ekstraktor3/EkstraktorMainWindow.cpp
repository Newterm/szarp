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
 * $Id$
 */

#include "config.h"

#include <vector>

#include "EkstraktorMainWindow.h"
#include "EkstraktorWidget.h"
#include "extraction.h"
#include "szapp.h"

#include "htmlabout.h"
#include "datechooser.h"
#include "cconv.h"

#include "ekstraktor3.h"
#include "sz4/base.h"

#include <boost/algorithm/string/case_conv.hpp>
DECLARE_APP(EkstrApp)

/** uncomment following line to turn on option for fonts configuration; it is
 * deprecated - fonts are set by SCC for all SZARP programs */
//#define CONFIG_FONT

#ifdef CONFIG_FONT
#include "fonts.h"
#define exSetDefFont(w) szSetDefFont(w)
#else
#define exSetDefFont(w)
#endif

#include <wx/image.h>
#include <wx/imagpng.h>


#include <wx/datetime.h>
#include <wx/statline.h>
#include <wx/calctrl.h>
#include <wx/filedlg.h>

#include <wx/app.h>

#include "../../resources/wx/icons/extr64.xpm"

#include <stdio.h>

int readable_filter(TParam *p)
{
	return !p->IsReadable();
}

EkstraktorMainWindow::EkstraktorMainWindow(EkstraktorWidget *widget, 
		const wxPoint pos, const wxSize size):
	wxFrame(NULL, -1, EKSTRAKTOR_TITLEBAR, pos, size, 
			// the wxRESIZE_BORDER option turned off
			wxDEFAULT_FRAME_STYLE - wxRESIZE_BORDER  )
{
	selectedValueType = SzbExtractor::TYPE_AVERAGE;

	mainWidget = widget;
	mainWidget->SetProbeType(PT_MIN10);
	progressDialog = NULL;

	exSetDefFont(this);
	wxImage::AddHandler(new wxPNGHandler);
	
	CreateMenu();
	CreateStatusBar();
	exSetDefFont(GetMenuBar());

	/* 
	 * Extractor main window:
	 * It's gonna look as follows:
	 
	+-----------------------------------------------sizer1---------------------------+
	|+-sizer1_1-------------------------------------------++-sizer1_2---------------+|
	||+-sizer1_1_1_1---------------++-sizer1_1_1_2-------+|| *separatorChooser      ||
	|||*firstDatabaseText            ||                  |||                        ||
	|||*startDateText *wxString      || *setStartDateBtn ||+ ---------------------- +|
	|||*lastDatabaseText             ||                  ||| *typeOfAvarege         ||
	|||*stopDateText                 || *setStopDateBtn  |||                        ||
	||+------------------------------+|                  |||                        ||
	|| *parameterListText            ||                  |||                        ||
	|| *parameterListWidget          ||                  |||                        ||
	||
	|+-------------------------------++------------------+ +------------------------+|
	| *chooseParamsBtn     *write_bt                                                 |
	+--------------------------------------------------------------------------------+

	*/

	selectedPeriod = PT_MIN10 - PT_SEC10; // PT_SEC10 is the first type used by ekstraktor3, we need to index accordingly
	selectedSeparator = COMMA;

	panel = new wxPanel(this);

	// Building begin/end date section
	firstDatabaseText = new wxStaticText(panel, -1, 
			wxString(FIRST_DATE_STR) + wxString(_T("YYYY-MM-DD HH:MM:SS")), wxDefaultPosition, wxDefaultSize);
	exSetDefFont(firstDatabaseText);
	lastDatabaseText = new wxStaticText(panel, -1, 
			wxString(LAST_DATE_STR) + wxString(_T("YYYY-MM-DD HH:MM:SS")) , wxDefaultPosition, wxDefaultSize);
	exSetDefFont(lastDatabaseText);
	startDateText = new wxStaticText(panel, -1, 
			wxString(START_DATE_STR) + wxString(_T("YYYY-MM-DD HH:MM:SS")), wxDefaultPosition, wxDefaultSize);
	stopDateText = new wxStaticText(panel, -1, 
			wxString(STOP_DATE_STR) + wxString(_T("YYYY-MM-DD HH:MM:SS")), wxDefaultPosition, wxDefaultSize);

	setStartDateBtn = new wxButton(panel, ID_ChangeStartDate, _("Change start date"), 
			//wxPoint(15, 2), wxDefaultSize, 0, wxDefaultValidator, _T(""));
			wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T(""));
	setStartDateBtn->SetToolTip(_("Set beginning of result data period"));

	setStopDateBtn = new wxButton(panel, ID_ChangeStopDate, _("Change stop date"), 
			//wxPoint(30, 2), wxDefaultSize, 0, wxDefaultValidator, _T(""));
			wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T(""));
	setStopDateBtn->SetToolTip(_("Set end of result data period"));

	wxStaticBoxSizer *separator_box_sizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Decimal Separator:"));
	wxStaticBox *static_box = separator_box_sizer->GetStaticBox();
	static_box->SetToolTip(_("Toggles between dot/comma and comma/semicolon decimal/field separator"));

	separator_box_sizer->Add(new wxRadioButton(panel, ID_CommaSeparator, _(", (colon)"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP), 0, wxEXPAND | wxRIGHT, 40);
	separator_box_sizer->Add(new wxRadioButton(panel, ID_DotSeparator, _(". (dot)")), 0, wxEXPAND | wxRIGHT, 40);

	minimize = new wxCheckBox(
			panel,
			ID_Minimize,
			_("Minimize output"));
	minimize->SetToolTip(_("Turns off printing of empty result lines"));
	
	minimize->SetValue(true);
			
	wxStaticBoxSizer *period_box_sizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Period:"));
	static_box = period_box_sizer->GetStaticBox();
	static_box->SetToolTip(_("Sets type of result data"));

	if (widget->IsProberConfigured()) {
		period_box_sizer->Add(new wxRadioButton(panel, ID_Sec10Period, _("10 SECONDS"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP),
			0, wxEXPAND | wxRIGHT, 20);
		period_box_sizer->Add(new wxRadioButton(panel, ID_Min10Period, _("10 MINUTES"), wxDefaultPosition, wxDefaultSize),
			0, wxEXPAND | wxRIGHT, 20);
	} else
		period_box_sizer->Add(new wxRadioButton(panel, ID_Min10Period, _("10 MINUTES"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP),
			0, wxEXPAND | wxRIGHT, 20);
	period_box_sizer->Add(new wxRadioButton(panel, ID_HourPeriod, _("HOUR")), 0, wxEXPAND | wxRIGHT, 20);
	period_box_sizer->Add(new wxRadioButton(panel, ID_8HourPeriod, _("8 HOURS")), 0, wxEXPAND | wxRIGHT, 20);
	dayPeriodBtn = new wxRadioButton(panel, ID_DayPeriod, _("DAY"));
	period_box_sizer->Add(dayPeriodBtn, 0, wxEXPAND | wxRIGHT, 20);
	period_box_sizer->Add(new wxRadioButton(panel, ID_WeekPeriod, _("WEEK")), 0, wxEXPAND | wxRIGHT, 20);
	period_box_sizer->Add(new wxRadioButton(panel, ID_MonthPeriod, _("MONTH")), 0, wxEXPAND | wxRIGHT, 20);

	value_type_box_sizer = new wxStaticBoxSizer(wxVERTICAL, panel, _("Values type:"));
	averageBtn = new wxRadioButton(panel, ID_Average, _("Average"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	value_type_box_sizer->Add(averageBtn, 0, wxEXPAND | wxRIGHT, 20);
	counterBtn = new wxRadioButton(panel, ID_End, _("Counter"));
	value_type_box_sizer->Add(counterBtn, 0, wxEXPAND | wxRIGHT, 20);
	HourValue = new wxCheckBox(panel, ID_HourValue, _("Hour"));
	HourValue->SetValue(false);
	HourValue->Disable();

        sizer_top = new wxBoxSizer(wxVERTICAL);
        sizer1 = new wxBoxSizer(wxHORIZONTAL);
        sizer2 = new wxBoxSizer(wxHORIZONTAL);
        sizer1_1 = new wxBoxSizer(wxVERTICAL);
	sizer1_2 = new wxBoxSizer(wxVERTICAL);
	sizer1_1_1 = new wxBoxSizer(wxHORIZONTAL);
	sizer1_1_1_1 = new wxBoxSizer(wxVERTICAL);
	sizer1_1_1_2 = new wxBoxSizer(wxVERTICAL);

	sizer1_1_1_1->Add(firstDatabaseText, 0, wxTOP | wxLEFT | wxEXPAND, 5);
	sizer1_1_1_1->Add(startDateText, 0, wxLEFT | wxEXPAND, 5);

    sizer1_1_1_1->Add(lastDatabaseText, 0, wxLEFT | wxEXPAND, 5);
	sizer1_1_1_1->Add(stopDateText, 0, wxLEFT | wxEXPAND, 5);

	sizer1_1_1_2->Add(setStartDateBtn, 0, wxALL | wxEXPAND, 6);
	sizer1_1_1_2->Add(setStopDateBtn, 0, wxALL | wxEXPAND, 6);

	sizer1_1_1->Add(sizer1_1_1_1, 0, 0, 0);
	sizer1_1_1->Add(sizer1_1_1_2, 0, 0, 0);
	

	sizer1_1->Add(sizer1_1_1, 0, wxBOTTOM, 10);
	 
	sizer1_2->Add(separator_box_sizer, 0, wxALL, 5);
	sizer1_2->Add(minimize, 0, wxALL, 5);
	sizer1_2->Add(period_box_sizer, 0, wxALL, 5);
	sizer1_2->Add(value_type_box_sizer, 0, wxALL, 5);
	sizer1_2->Add(HourValue, 0, wxALL, 5);

	sizer1_1->Add(new wxStaticText(panel, -1, 
			_("Parameters to be extracted:"), wxDefaultPosition, wxDefaultSize), 0, wxALL, 5);
	
	parametersList = new wxListBox(panel, -1, wxDefaultPosition, wxSize(400,200), 0, NULL, wxLB_NEEDED_SB|wxLB_SINGLE);
	sizer1_1->Add(parametersList, 1, wxALL | wxEXPAND, 5);

	wxButton* add_bt = new wxButton(panel, ID_AddParametersBt, _("Add parameter"));
	add_bt->SetToolTip(_("Shows dialog to select parameters"));
	sizer2->Add(add_bt, 1, wxALL | wxEXPAND, 10);
	del_par_bt = new wxButton(panel, ID_DeleteParametersBt, _("Delete parameter"));
	del_par_bt->SetToolTip(_("Removes currently selected parameter from list"));
	sizer2->Add(del_par_bt, 1, wxALL | wxEXPAND, 10);
	write_bt = new wxButton(panel, ID_WriteResultsBt, _("Write results to file"));
	write_bt->SetToolTip(_("Opens dialog for selecting result file"));
	sizer2->Add(write_bt, 1, wxALL | wxEXPAND, 10);
	
	
	sizer1->Add(sizer1_1, 1, wxALL | wxEXPAND, 0);
	sizer1->Add(sizer1_2, 0, wxALL, 0);
	
	sizer_top->Add(sizer1, 1, wxALL | wxEXPAND, 0);
	sizer_top->Add(sizer2, 0, wxALL | wxEXPAND, 0);

	panel->SetSizer(sizer_top);
	sizer_top->SetSizeHints(this);

	m_help = new szHelpController;
#ifndef MINGW32
	m_help->AddBook(dynamic_cast<szAppConfig*>(wxTheApp)->GetSzarpDir() + L"/resources/documentation/new/ekstraktor3/html/ekstraktor3.hhp");
#else
	m_help->AddBook(dynamic_cast<szAppConfig*>(wxTheApp)->GetSzarpDir() + L"\\resources\\documentation\\new\\ekstraktor3\\html\\ekstraktor3.hhp");
#endif
	szHelpControllerHelpProvider* m_provider = new szHelpControllerHelpProvider;
	wxHelpProvider::Set(m_provider);
	m_provider->SetHelpController(m_help);
	
	setStartDateBtn->SetHelpText(_T("ekstraktor3-base-date"));
	setStopDateBtn->SetHelpText(_T("ekstraktor3-base-date"));
	add_bt->SetHelpText(_T("ekstraktor3-base-select"));
	del_par_bt->SetHelpText(_T("ekstraktor3-base-select"));
	write_bt->SetHelpText(_T("ekstraktor3-base-write"));
	minimize->SetHelpText(_T("ekstraktor3-ext-mini"));
	
	
	ps = new szParSelect(mainWidget->GetIpk(), this, ID_ParSelectDlg, _("Choose parameter"),
			TRUE, TRUE, TRUE, TRUE,FALSE, readable_filter);
	ps->SetHelp(_T("ekstraktor3-base-select"));
	parlist = new szParList();
	parlist->RegisterIPK(mainWidget->GetIpk());
	
	wxIcon icon(wxICON(extr64));
	if (icon.Ok())
		SetIcon(icon);
	TestEmpty();
	dynamic_cast<wxRadioButton*>(FindWindowById(ID_Min10Period))->SetValue(true);
	Show( TRUE );
}

void EkstraktorMainWindow::CreateMenu()
{
	fileMenu = new wxMenu();
	fileMenu->Append(ID_WriteResults, _("Write results to file...\tCtrl-W"));
	fileMenu->AppendSeparator();
	
	fileMenu->Append(ID_ReadParamListFromFile, _("Read parameters from file...\tCtrl-O"));
	fileMenu->Append(ID_WriteParamListToFile, _("Write parameters to file\tCtrl-S"));

	fileMenu->AppendSeparator();
	fileMenu->Append(ID_PrintParamList, _("Print parameters list...\tCtrl-P"));
#ifdef MING32
	fileMenu->Append(ID_PrintPreview, _("Printing preview"));
#endif
	fileMenu->AppendSeparator();
	fileMenu->Append(ID_Quit, _("Quit\tCtrl-Q"));

#ifdef CONFIG_FONT
	wxMenu* optsMenu = new wxMenu();
	optsMenu->Append(ID_Fonts, _("&Fonts"));
#endif

	wxMenu* helpMenu = new wxMenu();

	helpMenu->Append(ID_Help, _("Table of contents\tF1"));
	helpMenu->Append(ID_ContextHelp, _("Context help\tShift-F1"));
	helpMenu->Append(ID_About, _("About..."));

	wxMenuBar *mainWindowMenuBar = new wxMenuBar;
	mainWindowMenuBar->Append( fileMenu, _("&File"));
#ifdef CONFIG_FONT
	mainWindowMenuBar->Append( optsMenu, _("&Options"));
#endif
	mainWindowMenuBar->Append( helpMenu, _("Hel&p"));
	exSetDefFont(mainWindowMenuBar);

	SetMenuBar(mainWindowMenuBar);
}

void EkstraktorMainWindow::TestEmpty()
{
	bool enable = TRUE;
	if ((parlist == NULL) || parlist->IsEmpty())
		enable = FALSE;
	fileMenu->Enable(ID_PrintParamList, enable);
#ifdef MING32
	fileMenu->Enable(ID_PrintPreview, enable);
#endif
	fileMenu->Enable(ID_WriteParamListToFile, enable);
	fileMenu->Enable(ID_WriteResults, enable);
	del_par_bt->Enable(enable);
	write_bt->Enable(enable);
}

void EkstraktorMainWindow::SetStartDate(time_t start_time)
{
	startDateText->SetLabel( wxString(START_DATE_STR) + FormatDate(start_time) );
}

void EkstraktorMainWindow::SetStopDate(time_t stop_time)
{
	stopDateText->SetLabel( wxString(STOP_DATE_STR) + FormatDate(stop_time) );
}

void EkstraktorMainWindow::SetFirstDatabaseDate(time_t oldest)
{
	firstDatabaseText->SetLabel( wxString(FIRST_DATE_STR) + FormatDate(oldest) );
}

void EkstraktorMainWindow::SetLastDatabaseDate(time_t newest)
{
	lastDatabaseText->SetLabel( wxString(LAST_DATE_STR) + FormatDate(newest));
}

void EkstraktorMainWindow::SetStartDateUndefined()
{
	startDateText->SetLabel( wxString(START_DATE_STR) + wxString(_("undefined")) );
}

void EkstraktorMainWindow::SetStopDateUndefined()
{
	stopDateText->SetLabel( wxString(STOP_DATE_STR) + wxString(_("undefined")) );
}

void EkstraktorMainWindow::SetFirstDatabaseDateUndefined()
{
	firstDatabaseText->SetLabel( wxString(FIRST_DATE_STR) + wxString(_("undefined")) );
}

void EkstraktorMainWindow::SetLastDatabaseDateUndefined()
{
	lastDatabaseText->SetLabel( wxString(LAST_DATE_STR) + wxString(_("undefined")) );
}

EkstraktorMainWindow::~EkstraktorMainWindow() 
{
	delete ps;
}

wxString EkstraktorMainWindow::FormatDate(time_t date)
{
	return wxString( wxDateTime(date).Format(_T("%Y-%m-%d %H:%M:%S")) );
}

void EkstraktorMainWindow::onChooseParams(wxCommandEvent &event) 
{
	if (ps->IsShown()) {
		ps->Raise();
	} else {
		ps->Show(TRUE);
	}
}

void EkstraktorMainWindow::onAddParams(szParSelectEvent &event)
{
	const ArrayOfTParam *params = event.GetParams();
	int n = params->Count();
	for (int i = 0; i < n; i++) {
		parametersList->Append(wxString(params->Item(i)->GetName()));
		parlist->Append(params->Item(i));
	}
	if (n == 1) {
		SetStatusText(wxString(_("Added parameter: ")) + 
				wxString(params->Item(0)->GetName()));
	} else {
		SetStatusText(wxString::Format(_("Added parameters: %d"), n));
	}
	TestEmpty();
}

void EkstraktorMainWindow::onDeleteParams(wxCommandEvent &event) 
{
	int i;
	if ((i = parametersList->GetSelection()) > -1) {
		parlist->Remove(i);
		parametersList->Delete(i);
		parametersList->SetSelection(wxMin( (unsigned int) i, parametersList->GetCount()));
		TestEmpty();
	}
}

void EkstraktorMainWindow::onReadParamListFromFile(wxCommandEvent &event)
{
	int i, j;
	szParList *old = new szParList(*parlist);
	i = parlist->LoadFile();
	if (i <= 0) {
		delete old;
		return;
	}
	if ((i > 0) && (parlist->Unbinded() > 0)) {
		if (wxMessageBox(
				_("Loaded list contains parameters from other configuration.\nSelect Ok to remove these parameters from list\nor Cancel to abort loading."),
				_("Incorrect parameters"),
				wxOK | wxCANCEL) == wxOK) {
			parlist->RemoveUnbinded();
			SetStatusText(wxString::Format(_("Loaded %d parameters"), 
						parlist->Count()));
			delete old;
		} else {
			delete parlist;
			parlist = old;
			SetStatusText(_("Loading of parameters list canceled"));
		}
	} 
	for (i = 0, j = 0; i < (int)parlist->Count(); i++) {
		if (!parlist->GetParam(i)->IsReadable()) {
			parlist->Remove(i);
			j++;
			i--;
		}
	}
	if (j > 0) {
		wxMessageBox(_("Loaded list contains parameters not existing in base.\nThese parameters will be removed from list."),
				_("Incorrect parameters"),
				wxOK);
	}
	parlist->FillListBox(parametersList);
	TestEmpty();
}

void EkstraktorMainWindow::onWriteParamListToFile(wxCommandEvent &event)
{
	if (parlist->SaveFile() == TRUE) {
		if (parlist->IsModified() == FALSE)
			SetStatusText(_("Parameters list saved"));
	} 
}

void EkstraktorMainWindow::onWriteResults(wxCommandEvent &event)
{
	int format = 0;
	// CHECK IF THERE ARE ANY PARAMETRES TO EXTRACTION CHOSEN
	if(parametersList->GetCount() == 0) {
		wxMessageBox(_("You have chosen no parameters to extraction"),
				_("Cannot make operation"),
				wxOK | wxICON_ERROR, this);
		return;
	}
	// NOW CHOOSE FILE NAME AND FILE FORMAT (DIALOG)
	wxFileDialog *file = new wxFileDialog(this, 
			_("Choose file to extract data to"),
			wxString(directory),
			_T(""),

			_("Comma Separated Values (*.csv)|*.csv|OpenOffice format (*.ods)|*.ods|XML format (*.xml)|*.xml|All files (*.*)|*.*"), wxSAVE | wxOVERWRITE_PROMPT);
	file->SetFilterIndex(3);

	if (file->ShowModal() == wxID_OK) {
		filename.clear();
		directory.clear();
		wxFileName filepath(file->GetPath());
		filename = filepath.GetFullName();
		directory = filepath.GetPath();
		if (filename.empty() || directory.empty()) {
			wxMessageBox(_("Cannot make operation"),
					_("Couldn't open file "),
					wxOK | wxICON_ERROR, this);
			delete file;
			return;
		}
		format = file->GetFilterIndex();
		delete file;
	} else {
		delete file;
		return;
	}
	// PREPARE DEFAULT ARGUMENT LIST
	struct extr_arguments arguments = getExtrDefaultArguments();
	// MAKE A LIST OF PARAMETERS FOR extract()
	arguments.params.clear();
	for(int i = 0; (unsigned int) i < parametersList->GetCount(); i++)
		arguments.params.push_back(SzbExtractor::Param(
			std::wstring(parametersList->GetString(i).wchar_str()),
			std::wstring(),
			selectedValueType ));

	// SET THE REST OF ARGUMENTS ACCORDINGLY
        arguments.start_time = mainWidget->GetStartDate();
        arguments.end_time = mainWidget->GetStopDate();
        arguments.probe = (SZARP_PROBE_TYPE)(selectedPeriod + PT_SEC10); // PT_SEC10 is the first SZARP_PROBE_TYPE that is contained in the widget, so when casting his index must be added.
	arguments.output = directory + L"/" + filename;
	if (selectedSeparator == PERIOD) {
		arguments.dec_delim = '.';
		arguments.delimiter = L", ";
	} else {
		arguments.dec_delim = ',';
	}

	int l = filename.size();
	if ((format == 3) && (l > 4 )) {
		std::wstring extension = filename.substr(l - 4);
		boost::to_lower(extension);
		if (extension == L".xml") {
			arguments.xml = 1; // XML
		} else if (extension == L".ods") {
			arguments.openoffice = 1; // ODS
		} else {
			arguments.csv = 1; // CSV, default
		}
	} else {
		switch (format) {
			case 2: 
				arguments.xml = 1;
				break;
			case 1: 
				arguments.openoffice = 1;
				break;
			default:
				arguments.csv = 1;
				break;
		}
	}
	if ((l <= 4) or filename.substr(l - 4, 1).compare(L".")) {
		switch (format) {
			case 2: 
				arguments.output += L".xml";
				break;
			case 1: 
				arguments.output += L".ods";
				break;
			default:
				arguments.output += L".csv";
				break;
		}
	}

        progressDialog = new wxProgressDialog(_("Please wait"),
				_("Extraction in progress..."), 104,
				this,
				wxPD_AUTO_HIDE 
#ifndef MINGW32
// this is broken under Windows
				| wxPD_ESTIMATED_TIME | wxPD_REMAINING_TIME 
#endif
				| wxPD_CAN_ABORT
				);
	
	cancel_lval = 0;
	int extr_ret =  extract(
			arguments,
			mainWidget->GetExtractor(),
			print_progress,
			this,
			&cancel_lval,
			mainWidget->GetIpk()
			);
	arguments.output.clear();
	delete progressDialog;

	// ANALYSE THE OUTPUT VALUE
	if (extr_ret == 0) {
		 SetStatusText(wxString(_("Parameters successfully extracted to ")) + 
				 filename );
		 wxMessageBox(wxString(_("Extraction completed")),
					 wxString(_("Completed")),
					 wxOK, this);
	} else {
		wxString msg;
		SetStatusText(wxString(_("Couldn't extract data")) );
		switch ((SzbExtractor::ErrorCode)extr_ret) {
			 case SzbExtractor::ERR_NOIMPL :
				 msg = _("Error: function not implemented");
				 break;
			 case SzbExtractor::ERR_CANCEL :
				 msg = _("Error: extraction canceled by user");
				 break;
			 case SzbExtractor::ERR_XMLWRITE :
				 msg = _("Error: XMLWriter error, program or libxml2 bug");
				 break;
			 case SzbExtractor::ERR_SYSERR :
				 msg = _("Error: system error");
				 break;
			 case SzbExtractor::ERR_ZIPCREATE :
				 msg = _("Error: cannot create file.\nCheck permissions.");
				 break;
			 case SzbExtractor::ERR_ZIPADD :
				 msg = _("Error: cannot add content to file.\nProgram bug?.");
				 break;
			 case SzbExtractor::ERR_LOADXSL :
				 msg = _("Error: error loading XSLT stylesheet");
				 break;
			 case SzbExtractor::ERR_TRANSFORM :
				 msg = _("Error: error applying XSL stylesheet");
				 break;
			 default:
				 msg = _("Error: other error (file access, parameters problems)");
				 break;
		} // switch
		if ((SzbExtractor::ErrorCode)extr_ret != 
				SzbExtractor::ERR_CANCEL) {
			wxMessageBox(msg, wxString(_("Error")),
					wxOK | wxICON_ERROR, this);
		}
	} 
}

void EkstraktorMainWindow::onWriteAvailableParamsToDisk(wxCommandEvent &event)
{
}

void EkstraktorMainWindow::onPrintParams(wxCommandEvent &event)
{
	parlist->Print(this);
}

void EkstraktorMainWindow::onPrintPreview(wxCommandEvent &event)
{
	parlist->PrintPreview(this);
}

void EkstraktorMainWindow::onQuit(wxCommandEvent &event)
{
	wxConfig::Get()->Flush();
	Close();
}

void EkstraktorMainWindow::onClose(wxCloseEvent &event)
{
	if (event.CanVeto() && parlist->IsModified() && !parlist->IsEmpty()) {
			if (wxMessageBox(_("Current parameters list is not saved.\nExit anyway?"),
						_("Parameters list modified"), 
						wxICON_QUESTION | wxYES_NO) 
				== wxNO) {
			event.Veto();
			return;
		}
		
	}
	delete parlist;
	wxExit();
}

#ifdef CONFIG_FONT
void EkstraktorMainWindow::onFonts(wxCommandEvent &event)
{
	szFontProvider::Get()->ShowFontsDialog(this);
}
#endif

void EkstraktorMainWindow::onShowContextHelp(wxCommandEvent& event)
{
    wxContextHelp contextHelp(this);
}


void EkstraktorMainWindow::onHelp(wxCommandEvent &event)
{
	m_help -> DisplayContents();
}

void EkstraktorMainWindow::onAbout(wxCommandEvent &event)
{
	wxGetApp().ShowAbout();
}

void EkstraktorMainWindow::onMinimize(wxCommandEvent &event)
{
	mainWidget->SetMinimized(minimize->GetValue());
}

void EkstraktorMainWindow::onStartDate(wxCommandEvent &event)
{
	DateChooserWidget *dat = 
		new DateChooserWidget(
				this,
				_("Select start date"),
				1,
				mainWidget->GetFirstDate(), 
				mainWidget->GetLastDate(),
				10
		);
		wxDateTime date = mainWidget->GetStartDate();
	
	if(dat->GetDate(date)) {
		delete dat;
	} else {
		delete dat;
		return;
	}
	mainWidget->SetStartDate(date.GetTicks());
	SetStartDate(date.GetTicks());
}

void EkstraktorMainWindow::onStopDate(wxCommandEvent &event)
{
	DateChooserWidget *dat = 
		new DateChooserWidget(
				this,
				_("Select stop date"),
				1,
				mainWidget->GetFirstDate(), 
				mainWidget->GetLastDate(),
				10
		);
		wxDateTime date = mainWidget->GetStopDate();
	
	if(dat->GetDate(date)) {
		delete dat;
	} else {
		delete dat;
		return;
	}
		mainWidget->SetStopDate(date.GetTicks());
	SetStopDate(date.GetTicks());
}

bool EkstraktorMainWindow::UpdateProgressBar(int percentage)
{
	wxString msg;
	int ret;
	switch (percentage) {
	case 101 :
		msg =  _("Applying XSL transformation");
		break;
	case 102 :
		msg =  _("Compressing output file");
		break;
	case 103 :
		msg =  _("Dumping data to output");
		break;
	case 104:
		msg =  _("Completed");
		break;
	default :
		msg = _T("");
		break;
	}
	ret =  progressDialog->Update(percentage, msg);
	//progressDialog->Fit();
	return ret;
}

void EkstraktorMainWindow::ResumeProgressBar()
{
	assert (progressDialog != NULL);
	progressDialog->Resume();
}

void EkstraktorMainWindow::onPeriodChange(wxCommandEvent &event) {
	selectedPeriod = event.GetId() - ID_Sec10Period;

	if(event.GetId() == ID_DayPeriod){
		HourValue->Enable();
	} else {
		HourValue->Disable();
		HourValue->SetValue(false);
		averageBtn->Enable();
	}

	if((event.GetId() == ID_DayPeriod) && (averageBtn->GetValue())){
		HourValue->Disable();
	}
}

void EkstraktorMainWindow::onHourChecked(wxCommandEvent &event) {
	if(HourValue->IsChecked()){
		selectedValueType = SzbExtractor::TYPE_START;
		averageBtn->Disable();
		counterBtn->SetValue(true);
	} else {
		selectedValueType = SzbExtractor::TYPE_END;
		averageBtn->Enable();
	}
}

void EkstraktorMainWindow::onSeparatorChange(wxCommandEvent &event) {
	if (event.GetId() == ID_DotSeparator)
		selectedSeparator = PERIOD;
	else
		selectedSeparator = COMMA;
}

void EkstraktorMainWindow::onAverageTypeSet(wxCommandEvent &event) {
	selectedValueType = SzbExtractor::TYPE_AVERAGE;
	if(averageBtn->GetValue()){
		HourValue->Disable();
	}
}

void EkstraktorMainWindow::onEndTypeSet(wxCommandEvent &event) {
	selectedValueType = SzbExtractor::TYPE_END;
	if((!averageBtn->GetValue())&& dayPeriodBtn->GetValue()){
		HourValue->Enable();
	}
}

BEGIN_EVENT_TABLE(EkstraktorMainWindow, wxFrame)
	EVT_MENU(ID_ReadParamListFromFile, EkstraktorMainWindow::onReadParamListFromFile)
	EVT_MENU(ID_WriteParamListToFile, EkstraktorMainWindow::onWriteParamListToFile)
	EVT_MENU(ID_WriteResults, EkstraktorMainWindow::onWriteResults)
	EVT_MENU(ID_PrintParamList, EkstraktorMainWindow::onPrintParams)
	EVT_MENU(ID_PrintPreview, EkstraktorMainWindow::onPrintPreview)
	EVT_MENU(ID_Quit, EkstraktorMainWindow::onQuit)
	EVT_CLOSE(EkstraktorMainWindow::onClose)
#ifdef CONFIG_FONT
	EVT_MENU(ID_Fonts, EkstraktorMainWindow::onFonts)
#endif
	EVT_MENU(ID_Help, EkstraktorMainWindow::onHelp)
	EVT_MENU(ID_ContextHelp, EkstraktorMainWindow::onShowContextHelp)
	
	EVT_MENU(ID_About, EkstraktorMainWindow::onAbout)

	EVT_RADIOBUTTON(ID_DotSeparator, EkstraktorMainWindow::onSeparatorChange)
	EVT_RADIOBUTTON(ID_CommaSeparator, EkstraktorMainWindow::onSeparatorChange)

	EVT_RADIOBUTTON(ID_CommaSeparator, EkstraktorMainWindow::onPeriodChange)
	EVT_RADIOBUTTON(ID_Sec10Period, EkstraktorMainWindow::onPeriodChange)
	EVT_RADIOBUTTON(ID_Min10Period, EkstraktorMainWindow::onPeriodChange)
	EVT_RADIOBUTTON(ID_HourPeriod, EkstraktorMainWindow::onPeriodChange)
	EVT_RADIOBUTTON(ID_8HourPeriod, EkstraktorMainWindow::onPeriodChange)
	EVT_RADIOBUTTON(ID_DayPeriod, EkstraktorMainWindow::onPeriodChange)
	EVT_RADIOBUTTON(ID_WeekPeriod, EkstraktorMainWindow::onPeriodChange)
	EVT_RADIOBUTTON(ID_MonthPeriod, EkstraktorMainWindow::onPeriodChange)

	EVT_RADIOBUTTON(ID_Average, EkstraktorMainWindow::onAverageTypeSet)
	EVT_RADIOBUTTON(ID_End, EkstraktorMainWindow::onEndTypeSet)

	EVT_CHECKBOX(ID_Minimize, EkstraktorMainWindow::onMinimize)
	EVT_CHECKBOX(ID_HourValue, EkstraktorMainWindow::onHourChecked)
	EVT_BUTTON(ID_ChangeStartDate, EkstraktorMainWindow::onStartDate)
	EVT_BUTTON(ID_ChangeStopDate, EkstraktorMainWindow::onStopDate)
	EVT_BUTTON(ID_AddParametersBt, EkstraktorMainWindow::onChooseParams)
	EVT_BUTTON(ID_DeleteParametersBt, EkstraktorMainWindow::onDeleteParams)
	EVT_BUTTON(ID_WriteResultsBt, EkstraktorMainWindow::onWriteResults)
	EVT_SZ_PARADD(ID_ParSelectDlg, EkstraktorMainWindow::onAddParams)
//EVT_COMMAND(ID_ParSelectDlg, wxEVT_SZ_PARADD, EkstraktorMainWindow::onAddParams)
END_EVENT_TABLE()

void *print_progress(int progress, void *data)
{
	bool ret;
	assert (data != NULL);
	ret = ((EkstraktorMainWindow *)data)->UpdateProgressBar(progress);
	if (ret == FALSE) {
		if (wxMessageBox(_("Cancel extraction?"), _("Abort"),
				wxYES_NO | wxICON_QUESTION, 
				(wxWindow*)data) == wxYES) {
			((EkstraktorMainWindow *)data)->cancel_lval = 1;
		} else {
			((EkstraktorMainWindow *)data)->ResumeProgressBar();
		}
	}
	return NULL;
}
