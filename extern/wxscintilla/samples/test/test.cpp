//////////////////////////////////////////////////////////////////////////////
// File:        test.cpp
// Purpose:     wxScintilla test application
// Maintainer:  Otto Wyss
// Created:     2003-09-01
// RCS-ID:      $Id$
// Copyright:   (c) 2004 wxCode
// Licence:     wxWindows licence
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// headers
//----------------------------------------------------------------------------

// For compilers that support precompilation, includes <wx/wx.h>.
#include <wx/wxprec.h>

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all 'standard' wxWindows headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

//! wxWindows headers
#include <wx/config.h>   // configuration support
#include <wx/filedlg.h>  // file dialog support
#include <wx/filename.h> // filename support
#include <wx/settings.h> // system settings
#include <wx/string.h>   // strings support
#include <wx/image.h>    // images support

//! application headers
#include "defsext.h"     // Additional definitions
#include "edit.h"        // Edit module
#include "prefs.h"       // Prefs


//----------------------------------------------------------------------------
// resources
//----------------------------------------------------------------------------

// the application icon (under Windows and OS/2 it is in resources)
#if defined(__WXGTK__) || defined(__WXMOTIF__) || defined(__WXMAC__) || defined(__WXMGL__) || defined(__WXX11__)
    #include "mondrian.xpm"
#endif

//============================================================================
// declarations
//============================================================================

const wxString APP_NAME = _T("wxScintilla");
const wxString APP_VENDOR = _T("wxCode");
const wxString APP_VERSION = _T("1.0.0");
const wxString APP_MAINT = _T("Otto Wyss");
const wxString APP_LICENCE = _T("wxWindows");
const wxString APP_COPYRIGTH = _T("(C) 2005 Otto Wyss");

#define APP_DESCR _("\
wxScintilla implements the Scintilla editing control (see \n\
http://wxcode.sf.net/components/wxscintilla/website/index.html) \n\
with the wxWidgets API. It's derived from wxStyledTextCtrl and has the \n\
same functionality and a rather similar API.\
")
const wxString APP_WEBSITE = _T("http://wxcode.sourceforge.net/");
const wxString APP_WEBPAGE = _T("components/wxscintilla/website/index.html");

const wxString APP_INFOS = _("\
This application is derived from the demo sample of wyoGuide.\
");
const wxString APP_WYOGUIDE = _T("http://wyoguide.sourceforge.net");

#define NONAME _("<untitled>")


//----------------------------------------------------------------------------
//! global application name
wxString *g_appname = NULL;

//! global print data, to remember settings during the session
wxPrintData *g_printData = (wxPrintData*) NULL;
wxPageSetupData *g_pageSetupData = (wxPageSetupData*) NULL;


//----------------------------------------------------------------------------
//! class declarations
class AppFrame;


//----------------------------------------------------------------------------
//! application APP_VENDOR-APP_NAME.
class App: public wxApp {
    friend class AppFrame;

public:
    //! the main function called durning application start
    virtual bool OnInit ();

    //! application exit function
    virtual int OnExit ();

private:
    //! frame window
    AppFrame* m_frame;

};

// created dynamically by wxWindows
DECLARE_APP (App);

//----------------------------------------------------------------------------
//! frame of the application
class AppFrame: public wxFrame {
    friend class App;
    friend class AppAbout;

public:
    //! constructor
    AppFrame (const wxString &title);

    //! destructor
    ~AppFrame ();

    //! event handlers
    //! common
    void OnClose (wxCloseEvent &event);
    void OnAbout (wxCommandEvent &event);
    void OnExit (wxCommandEvent &event);
    void OnTimerEvent (wxTimerEvent &event);
    //! file
    void OnFileNew (wxCommandEvent &event);
    void OnFileNewFrame (wxCommandEvent &event);
    void OnFileOpen (wxCommandEvent &event);
    void OnFileOpenFrame (wxCommandEvent &event);
    void OnFileSave (wxCommandEvent &event);
    void OnFileSaveAs (wxCommandEvent &event);
    void OnFileClose (wxCommandEvent &event);
    //! properties
    void OnProperties (wxCommandEvent &event);
    //! print
    void OnPrintSetup (wxCommandEvent &event);
    void OnPrintPreview (wxCommandEvent &event);
    void OnPrint (wxCommandEvent &event);
    //! edit events
    void OnEdit (wxCommandEvent &event);

private:
    // edit object
    Edit *m_edit;
    void FileOpen (wxString fname);

    //! creates the application menu bar
    wxMenuBar *m_menuBar;
    void CreateMenu ();

    // print preview position and size
    wxRect DeterminePrintSize ();

    DECLARE_EVENT_TABLE()
};

//----------------------------------------------------------------------------
//! about box of the application APP_VENDOR-APP_NAME
class AppAbout: public wxDialog {

public:
    //! constructor
    AppAbout (wxWindow *parent,
              int milliseconds = 0);

    //! destructor
    ~AppAbout ();

    // event handlers
    void OnTimerEvent (wxTimerEvent &event);

private:
    // timer
    wxTimer *m_timer;

    DECLARE_EVENT_TABLE()
};


//============================================================================
// implementation
//============================================================================

IMPLEMENT_APP (App)

//----------------------------------------------------------------------------
// App
//----------------------------------------------------------------------------

bool App::OnInit () {

    wxInitAllImageHandlers();

    // set application and vendor name
    SetAppName (APP_NAME);
    SetVendorName (APP_VENDOR);
    g_appname = new wxString ();
    g_appname->Append (APP_NAME);

    // about box shown for 3 seconds
    AppAbout (NULL, 3000);

    // initialize print data and setup
    g_printData = new wxPrintData;
    g_pageSetupData = new wxPageSetupDialogData;

    // create application frame
    m_frame = new AppFrame (*g_appname);

    // open application frame
    m_frame->Layout ();
    m_frame->Show (true);
    SetTopWindow (m_frame);

    return true;
}

int App::OnExit () {

    // delete global appname
    if (g_appname) delete g_appname;

    // delete global print data and setup
    if (g_printData) delete g_printData;
    if (g_pageSetupData) delete g_pageSetupData;

    return 0;
}

//----------------------------------------------------------------------------
// AppFrame
//----------------------------------------------------------------------------

BEGIN_EVENT_TABLE (AppFrame, wxFrame)
    // common
    EVT_CLOSE (                      AppFrame::OnClose)
    // file
    EVT_MENU (wxID_OPEN,             AppFrame::OnFileOpen)
    EVT_MENU (wxID_SAVE,             AppFrame::OnFileSave)
    EVT_MENU (wxID_SAVEAS,           AppFrame::OnFileSaveAs)
    EVT_MENU (wxID_CLOSE,            AppFrame::OnFileClose)
    // properties
    EVT_MENU (myID_PROPERTIES,       AppFrame::OnProperties)
    // print and exit
    EVT_MENU (wxID_PRINT_SETUP,      AppFrame::OnPrintSetup)
    EVT_MENU (wxID_PREVIEW,          AppFrame::OnPrintPreview)
    EVT_MENU (wxID_PRINT,            AppFrame::OnPrint)
    EVT_MENU (wxID_EXIT,             AppFrame::OnExit)
    // edit
    EVT_MENU (wxID_CLEAR,            AppFrame::OnEdit)
    EVT_MENU (wxID_CUT,              AppFrame::OnEdit)
    EVT_MENU (wxID_COPY,             AppFrame::OnEdit)
    EVT_MENU (wxID_PASTE,            AppFrame::OnEdit)
    EVT_MENU (myID_INDENTINC,        AppFrame::OnEdit)
    EVT_MENU (myID_INDENTRED,        AppFrame::OnEdit)
    EVT_MENU (wxID_SELECTALL,        AppFrame::OnEdit)
    EVT_MENU (myID_SELECTLINE,       AppFrame::OnEdit)
    EVT_MENU (wxID_REDO,             AppFrame::OnEdit)
    EVT_MENU (wxID_UNDO,             AppFrame::OnEdit)
    // find
    EVT_MENU (wxID_FIND,             AppFrame::OnEdit)
    EVT_MENU (myID_FINDNEXT,         AppFrame::OnEdit)
    EVT_MENU (myID_REPLACE,          AppFrame::OnEdit)
    EVT_MENU (myID_REPLACENEXT,      AppFrame::OnEdit)
    EVT_MENU (myID_BRACEMATCH,       AppFrame::OnEdit)
    EVT_MENU (myID_GOTO,             AppFrame::OnEdit)
    // view
    EVT_MENU_RANGE (myID_HILIGHTFIRST, myID_HILIGHTLAST,
                                     AppFrame::OnEdit)
    EVT_MENU (myID_DISPLAYEOL,       AppFrame::OnEdit)
    EVT_MENU (myID_INDENTGUIDE,      AppFrame::OnEdit)
    EVT_MENU (myID_LINENUMBER,       AppFrame::OnEdit)
    EVT_MENU (myID_LONGLINEON,       AppFrame::OnEdit)
    EVT_MENU (myID_WHITESPACE,       AppFrame::OnEdit)
    EVT_MENU (myID_FOLDTOGGLE,       AppFrame::OnEdit)
    EVT_MENU (myID_OVERTYPE,         AppFrame::OnEdit)
    EVT_MENU (myID_READONLY,         AppFrame::OnEdit)
    EVT_MENU (myID_WRAPMODEON,       AppFrame::OnEdit)
    // extra
    EVT_MENU (myID_CHANGELOWER,      AppFrame::OnEdit)
    EVT_MENU (myID_CHANGEUPPER,      AppFrame::OnEdit)
    EVT_MENU (myID_CONVERTCR,        AppFrame::OnEdit)
    EVT_MENU (myID_CONVERTCRLF,      AppFrame::OnEdit)
    EVT_MENU (myID_CONVERTLF,        AppFrame::OnEdit)
    EVT_MENU (myID_CHARSETANSI,      AppFrame::OnEdit)
    EVT_MENU (myID_CHARSETMAC,       AppFrame::OnEdit)
    // help
    EVT_MENU (wxID_ABOUT,            AppFrame::OnAbout)
END_EVENT_TABLE ()

AppFrame::AppFrame (const wxString &title)
        : wxFrame ((wxFrame *)NULL, -1, title, wxDefaultPosition, wxSize (600,400),
                    wxDEFAULT_FRAME_STYLE | wxNO_FULL_REPAINT_ON_RESIZE) {

    // intitialize important variables
    m_edit = NULL;

    // set icon and background
    SetTitle (*g_appname);
    SetIcon (wxICON (mondrian));

    // create menu
    m_menuBar = new wxMenuBar;
    CreateMenu ();

    // open first page
    m_edit = new Edit (this, -1);
    m_edit->SetFocus();

}

AppFrame::~AppFrame () {
}

// common event handlers
void AppFrame::OnClose (wxCloseEvent &event) {
    wxCommandEvent evt;
    OnFileClose (evt);
    if (m_edit && m_edit->Modified()) {
        if (event.CanVeto()) event.Veto (true);
        return;
    }
    Destroy();
}

void AppFrame::OnAbout (wxCommandEvent &WXUNUSED(event)) {
    AppAbout (this);
}

void AppFrame::OnExit (wxCommandEvent &WXUNUSED(event)) {
    Close (true);
}

// file event handlers
void AppFrame::OnFileOpen (wxCommandEvent &WXUNUSED(event)) {
    if (!m_edit) return;
    wxString fname;
    wxFileDialog dlg (this, _T("Open file"), _T(""), _T(""), _T("Any file (*)|*"),
                      wxOPEN | wxFILE_MUST_EXIST | wxCHANGE_DIR);
    if (dlg.ShowModal() != wxID_OK) return;
    fname = dlg.GetPath ();
    FileOpen (fname);
}

void AppFrame::OnFileSave (wxCommandEvent &WXUNUSED(event)) {
    if (!m_edit) return;
    if (!m_edit->Modified()) {
        wxMessageBox (_("There is nothing to save!"), _("Save file"),
                      wxOK | wxICON_EXCLAMATION);
        return;
    }
    m_edit->SaveFile ();
}

void AppFrame::OnFileSaveAs (wxCommandEvent &WXUNUSED(event)) {
    if (!m_edit) return;
    wxString filename = wxEmptyString;
    wxFileDialog dlg (this, _T("Save file"), _T(""), _T(""), _T("Any file (*)|*"), wxSAVE|wxOVERWRITE_PROMPT);
    if (dlg.ShowModal() != wxID_OK) return;
    filename = dlg.GetPath();
    m_edit->SaveFile (filename);
}

void AppFrame::OnFileClose (wxCommandEvent &WXUNUSED(event)) {
    if (!m_edit) return;
    if (m_edit->Modified()) {
        if (wxMessageBox (_("Text is not saved, save before closing?"), _("Close"),
                          wxYES_NO | wxICON_QUESTION) == wxYES) {
            m_edit->SaveFile();
            if (m_edit->Modified()) {
                wxMessageBox (_("Text could not be saved!"), _("Close abort"),
                              wxOK | wxICON_EXCLAMATION);
                return;
            }
        }
    }
    m_edit->SetFilename (wxEmptyString);
    m_edit->ClearAll();
    m_edit->SetSavePoint();
}

// properties event handlers
void AppFrame::OnProperties (wxCommandEvent &WXUNUSED(event)) {
    if (!m_edit) return;
    EditProperties (m_edit, 0);
}

// print event handlers
void AppFrame::OnPrintSetup (wxCommandEvent &WXUNUSED(event)) {
    (*g_pageSetupData) = * g_printData;
    wxPageSetupDialog pageSetupDialog(this, g_pageSetupData);
    pageSetupDialog.ShowModal();
    (*g_printData) = pageSetupDialog.GetPageSetupData().GetPrintData();
    (*g_pageSetupData) = pageSetupDialog.GetPageSetupData();
}

void AppFrame::OnPrintPreview (wxCommandEvent &WXUNUSED(event)) {
    wxPrintDialogData printDialogData( *g_printData);
    wxPrintPreview *preview =
        new wxPrintPreview (new EditPrint (m_edit),
                            new EditPrint (m_edit),
                            &printDialogData);
    if (!preview->Ok()) {
        delete preview;
        wxMessageBox (_("There was a problem with previewing.\n\
                         Perhaps your current printer is not correctly?"),
                      _("Previewing"), wxOK);
        return;
    }
    wxRect rect = DeterminePrintSize();
    wxPreviewFrame *frame = new wxPreviewFrame (preview, this, _("Print Preview"));
    frame->SetSize (rect);
    frame->Centre(wxBOTH);
    frame->Initialize();
    frame->Show(true);
}

void AppFrame::OnPrint (wxCommandEvent &WXUNUSED(event)) {
    wxPrintDialogData printDialogData( *g_printData);
    wxPrinter printer (&printDialogData);
    EditPrint printout (m_edit);
    if (!printer.Print (this, &printout, true)) {
        if (wxPrinter::GetLastError() == wxPRINTER_ERROR) {
        wxMessageBox (_("There was a problem with printing.\n\
                         Perhaps your current printer is not correctly?"),
                      _("Previewing"), wxOK);
            return;
        }
    }
    (*g_printData) = printer.GetPrintDialogData().GetPrintData();
}

// edit events
void AppFrame::OnEdit (wxCommandEvent &event) {
    if (m_edit) m_edit->ProcessEvent (event);
}

// private functions
void AppFrame::CreateMenu () {

    // File menu
    wxMenu *menuFile = new wxMenu;
    menuFile->Append (wxID_OPEN, _("&Open ..\tCtrl+O"));
    menuFile->Append (wxID_SAVE, _("&Save\tCtrl+S"));
    menuFile->Append (wxID_SAVEAS, _("Save &as ..\tCtrl+Shift+S"));
    menuFile->Append (wxID_CLOSE, _("&Close\tCtrl+W"));
    menuFile->AppendSeparator();
    menuFile->Append (myID_PROPERTIES, _("Proper&ties ..\tCtrl+I"));
    menuFile->AppendSeparator();
    menuFile->Append (wxID_PRINT_SETUP, _("Print Set&up .."));
    menuFile->Append (wxID_PREVIEW, _("Print Pre&view\tCtrl+Shift+P"));
    menuFile->Append (wxID_PRINT, _("&Print ..\tCtrl+P"));
    menuFile->AppendSeparator();
    menuFile->Append (wxID_EXIT, _("&Quit\tCtrl+Q"));

    // Edit menu
    wxMenu *menuEdit = new wxMenu;
    menuEdit->Append (wxID_UNDO, _("&Undo\tCtrl+Z"));
    menuEdit->Append (wxID_REDO, _("&Redo\tCtrl+Shift+Z"));
    menuEdit->AppendSeparator();
    menuEdit->Append (wxID_CUT, _("Cu&t\tCtrl+X"));
    menuEdit->Append (wxID_COPY, _("&Copy\tCtrl+C"));
    menuEdit->Append (wxID_PASTE, _("&Paste\tCtrl+V"));
    menuEdit->Append (wxID_CLEAR, _("&Delete\tDel"));
    menuEdit->AppendSeparator();
    menuEdit->Append (wxID_FIND, _("&Find\tCtrl+F"));
    menuEdit->Enable (wxID_FIND, false);
    menuEdit->Append (myID_FINDNEXT, _("Find &next\tF3"));
    menuEdit->Enable (myID_FINDNEXT, false);
    menuEdit->Append (myID_REPLACE, _("&Replace\tCtrl+H"));
    menuEdit->Enable (myID_REPLACE, false);
    menuEdit->Append (myID_REPLACENEXT, _("Replace &again\tShift+F4"));
    menuEdit->Enable (myID_REPLACENEXT, false);
    menuEdit->AppendSeparator();
    menuEdit->Append (myID_BRACEMATCH, _("&Match brace\tCtrl+M"));
    menuEdit->Append (myID_GOTO, _("&Goto\tCtrl+G"));
    menuEdit->Enable (myID_GOTO, false);
    menuEdit->AppendSeparator();
    menuEdit->Append (myID_INDENTINC, _("&Indent increase\tTab"));
    menuEdit->Append (myID_INDENTRED, _("I&ndent reduce\tBksp"));
    menuEdit->AppendSeparator();
    menuEdit->Append (wxID_SELECTALL, _("&Select all\tCtrl+A"));
    menuEdit->Append (myID_SELECTLINE, _("Select &line\tCtrl+L"));

    // hilight submenu
    wxMenu *menuHilight = new wxMenu;
    int Nr;
    for (Nr = 0; Nr < g_LanguagePrefsSize; Nr++) {
        menuHilight->Append (myID_HILIGHTFIRST + Nr,
                             g_LanguagePrefs [Nr].name);
    }

    // charset submenu
    wxMenu *menuCharset = new wxMenu;
    menuCharset->Append (myID_CHARSETANSI, _("&ANSI (Windows)"));
    menuCharset->Append (myID_CHARSETMAC, _("&MAC (Macintosh)"));

    // View menu
    wxMenu *menuView = new wxMenu;
    menuView->Append (myID_HILIGHTLANG, _("&Hilight language .."), menuHilight);
    menuView->AppendSeparator();
    menuView->AppendCheckItem (myID_OVERTYPE, _("&Overwrite mode\tIns"));
    menuView->Check (myID_OVERTYPE, g_CommonPrefs.overTypeInitial);
    menuView->AppendCheckItem (myID_WRAPMODEON, _("&Wrap mode\tCtrl+U"));
    menuView->Check (myID_WRAPMODEON, g_CommonPrefs.wrapModeInitial);
    menuView->AppendSeparator();
    menuView->AppendCheckItem (myID_DISPLAYEOL, _("Show line &endings"));
    menuView->Check (myID_DISPLAYEOL, g_CommonPrefs.displayEOLEnable);
    menuView->AppendCheckItem (myID_INDENTGUIDE, _("Show &indent guides"));
    menuView->Check (myID_INDENTGUIDE, g_CommonPrefs.indentGuideEnable);
    menuView->AppendCheckItem (myID_LINENUMBER, _("Show line &numbers"));
    menuView->Check (myID_LINENUMBER, g_CommonPrefs.lineNumberEnable);
    menuView->AppendCheckItem (myID_LONGLINEON, _("Show &long line marker"));
    menuView->Check (myID_LONGLINEON, g_CommonPrefs.longLineOnEnable);
    menuView->AppendCheckItem (myID_WHITESPACE, _("Show white&space"));
    menuView->Check (myID_WHITESPACE, g_CommonPrefs.whiteSpaceEnable);
    menuView->AppendSeparator();
    menuView->Append (myID_USECHARSET, _("Use &code page of .."), menuCharset);

    // change case submenu
    wxMenu *menuChangeCase = new wxMenu;
    menuChangeCase->Append (myID_CHANGEUPPER, _("&Upper case"));
    menuChangeCase->Append (myID_CHANGELOWER, _("&Lower case"));

    // convert EOL submenu
    wxMenu *menuConvertEOL = new wxMenu;
    menuConvertEOL->Append (myID_CONVERTCR, _("CR (&Linux)"));
    menuConvertEOL->Append (myID_CONVERTCRLF, _("CR+LF (&Windows)"));
    menuConvertEOL->Append (myID_CONVERTLF, _("LF (&Macintosh)"));

    // Extra menu
    wxMenu *menuExtra = new wxMenu;
    menuExtra->AppendCheckItem (myID_READONLY, _("&Readonly mode"));
    menuExtra->Check (myID_READONLY, g_CommonPrefs.readOnlyInitial);
    menuExtra->AppendSeparator();
    menuExtra->Append (myID_CHANGECASE, _("Change &case to .."), menuChangeCase);
    menuExtra->AppendSeparator();
    menuExtra->Append (myID_CONVERTEOL, _("Convert line &endings to .."), menuConvertEOL);

    // Help menu
    wxMenu *menuHelp = new wxMenu;
    menuHelp->Append (wxID_ABOUT, _("&About ..\tShift+F1"));

    // construct menu
    m_menuBar->Append (menuFile, _("&File"));
    m_menuBar->Append (menuEdit, _("&Edit"));
    m_menuBar->Append (menuView, _("&View"));
    m_menuBar->Append (menuExtra, _("E&xtra"));
    m_menuBar->Append (menuHelp, _("&Help"));
    SetMenuBar (m_menuBar);

}

void AppFrame::FileOpen (wxString fname) {
    wxFileName w(fname); w.Normalize(); fname = w.GetFullPath();
    m_edit->LoadFile (fname);
}

wxRect AppFrame::DeterminePrintSize () {

    wxSize scr = wxGetDisplaySize();

    // determine position and size (shifting 16 left and down)
    wxRect rect = GetRect();
    rect.x += 16;
    rect.y += 16;
    rect.width = wxMin (rect.width, (scr.x - rect.x));
    rect.height = wxMin (rect.height, (scr.x - rect.y));

    return rect;
}


//----------------------------------------------------------------------------
// AppAbout
//----------------------------------------------------------------------------

BEGIN_EVENT_TABLE (AppAbout, wxDialog)
    EVT_TIMER (myID_ABOUTTIMER, AppAbout::OnTimerEvent)
END_EVENT_TABLE ()

AppAbout::AppAbout (wxWindow *parent,
                    int milliseconds)
        : wxDialog (parent, -1, _("About..."),
                    wxDefaultPosition, wxDefaultSize,
                    wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {

    // set timer if any
    m_timer = NULL;
    if (milliseconds > 0) {
        m_timer = new wxTimer (this, myID_ABOUTTIMER);
        m_timer->Start (milliseconds, wxTIMER_ONE_SHOT);
    }

    // about info
    wxFlexGridSizer *aboutinfo = new wxFlexGridSizer (2, 0, 2);
    aboutinfo->Add (new wxStaticText(this, -1, _("Vendor: ")),0, wxALIGN_LEFT);
    aboutinfo->Add (new wxStaticText(this, -1, APP_VENDOR),0, wxALIGN_LEFT);
    aboutinfo->Add (new wxStaticText(this, -1, _("Version: ")),0, wxALIGN_LEFT);
    aboutinfo->Add (new wxStaticText(this, -1, APP_VERSION),0, wxALIGN_LEFT);
    aboutinfo->Add (new wxStaticText(this, -1, _("Written by: ")),0, wxALIGN_LEFT);
    aboutinfo->Add (new wxStaticText(this, -1, APP_MAINT),0, wxALIGN_LEFT);
    aboutinfo->Add (new wxStaticText(this, -1, _("Licence type: ")),0, wxALIGN_LEFT);
    aboutinfo->Add (new wxStaticText(this, -1, APP_LICENCE),0, wxALIGN_LEFT);
    aboutinfo->Add (new wxStaticText(this, -1, _("Copyright: ")),0, wxALIGN_LEFT);
    aboutinfo->Add (new wxStaticText(this, -1, APP_COPYRIGTH),0, wxALIGN_LEFT);

    // about icontitle//info
    wxBoxSizer *aboutpane = new wxBoxSizer (wxHORIZONTAL);
    wxBitmap bitmap = wxBitmap(wxICON (mondrian));
    aboutpane->Add (new wxStaticBitmap (this, -1, bitmap),
                    1, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 20);
    aboutpane->Add (aboutinfo, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
    aboutpane->Add (20, 0);

    // about complete
    wxBoxSizer *totalpane = new wxBoxSizer (wxVERTICAL);
    totalpane->Add (0, 10);
    wxStaticText *appname = new wxStaticText(this, -1, APP_NAME);
    appname->SetFont (wxFont (20, wxDEFAULT, wxNORMAL, wxBOLD));
    totalpane->Add (appname, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 40);
    totalpane->Add (0, 10);
    totalpane->Add (aboutpane, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    totalpane->Add (new wxStaticText(this, -1, wxGetTranslation(APP_DESCR)),
                    0, wxALIGN_LEFT | wxLEFT | wxRIGHT, 10);
    totalpane->Add (0, 6);
    totalpane->Add (new wxStaticText(this, -1, APP_WEBSITE),
                    0, wxALIGN_CENTER | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    totalpane->Add (new wxStaticText(this, -1, wxGetTranslation(APP_INFOS)),
                    0, wxALIGN_LEFT | wxLEFT | wxRIGHT, 10);
    totalpane->Add (0, 6);
    totalpane->Add (new wxStaticText (this, -1, APP_WYOGUIDE),
                    0, wxALIGN_CENTER | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    wxButton *okButton = new wxButton (this, wxID_OK, _("OK"));
    okButton->SetDefault();
    totalpane->Add (okButton, 0, wxALIGN_CENTER | wxALL, 10);

    SetSizerAndFit (totalpane);
    CentreOnParent();
    ShowModal();
}

AppAbout::~AppAbout () {
    if (m_timer)  {
        delete m_timer;
        m_timer = NULL;
    }
}

//----------------------------------------------------------------------------
// event handlers
void AppAbout::OnTimerEvent (wxTimerEvent &WXUNUSED(event)) {
    if (m_timer) delete m_timer;
    m_timer = NULL;
    EndModal (wxID_OK);
}

