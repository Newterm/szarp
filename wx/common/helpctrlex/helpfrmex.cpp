/////////////////////////////////////////////////////////////////////////////
// Name:        helpfrmex.cpp
// Purpose:     wxHtmlHelpFrameEx
// Notes:       Based on htmlhelp.cpp, implementing a monolithic
//              HTML Help controller class,  by Vaclav Slavik
// Author:      Harm van der Heijden, Vaclav Slavik and Julian Smart
// RCS-ID:      $Id: helpfrmex.cpp,v 1.3 2004/05/08 14:23:48 js Exp $
// Copyright:   (c) Harm van der Heijden, Vaclav Slavik and Julian Smart
// Licence:     wxWidgets licence
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "helpfrmex.h"
#endif

// For compilers that support precompilation, includes "wx.h"
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#if wxUSE_WXHTML_HELP

#ifndef WXPRECOMP
    #include "wx/intl.h"
    #include "wx/log.h"

    #include "wx/object.h"
    #include "wx/sizer.h"

    #include "wx/bmpbuttn.h"
    #include "wx/statbox.h"
    #include "wx/radiobox.h"
#endif // WXPRECOMP

#ifdef __WXMAC__
    #include "wx/menu.h"
    #include "wx/msgdlg.h"
#endif

#include "wx/html/htmlwin.h"
#include "wx/artprov.h"

#include "helpfrmex.h"
#include "helpctrlex.h"

// what is considered "small index"?
#define INDEX_IS_SMALL 100

/* Motif defines this as a macro */
#ifdef Below
#undef Below
#endif

IMPLEMENT_DYNAMIC_CLASS(wxHtmlHelpFrameEx, wxFrame)

wxHtmlHelpFrameEx::wxHtmlHelpFrameEx(wxWindow* parent, wxWindowID id, const wxString& title,
                                 int style, wxHtmlHelpData* data,
                                 wxConfigBase *config, const wxString& rootpath)
{
    Init(data);
    Create(parent, id, title, style, config, rootpath);
}

void wxHtmlHelpFrameEx::Init(wxHtmlHelpData* data)
{
    // Simply pass the pointer on to the help window
    m_Data = data;
    m_HtmlHelpWin = NULL;
    m_helpController = NULL;
}

// Create: builds the GUI components.
bool wxHtmlHelpFrameEx::Create(wxWindow* parent, wxWindowID id,
                             const wxString& WXUNUSED(title), int style,
                             wxConfigBase *config, const wxString& rootpath)
{
    m_HtmlHelpWin = new wxHtmlHelpWindowEx(m_Data);
    if (config)
        m_HtmlHelpWin->UseConfig(config, rootpath);

    wxFrame::Create(parent, id, _("Help"), 
                    wxPoint(m_HtmlHelpWin->GetCfgData().x, m_HtmlHelpWin->GetCfgData().y),
                    wxSize(m_HtmlHelpWin->GetCfgData().w, m_HtmlHelpWin->GetCfgData().h), 
                    wxDEFAULT_FRAME_STYLE, wxT("wxHtmlHelp"));
    m_HtmlHelpWin->Create(this, -1, wxDefaultPosition, wxDefaultSize,
        wxTAB_TRAVERSAL|wxNO_BORDER, style);

    GetPosition(& (m_HtmlHelpWin->GetCfgData().x), & (m_HtmlHelpWin->GetCfgData()).y);

    SetIcon(wxArtProvider::GetIcon(wxART_HELP, wxART_HELP_BROWSER));

    // On the Mac, each modeless frame must have a menubar.
    // TODO: add more menu items, and perhaps add a style to show
    // the menubar: compulsory on the Mac, optional elsewhere.
#if 0 // def __WXMAC__
    wxMenuBar* menuBar = new wxMenuBar;

    wxMenu* fileMenu = new wxMenu;
    fileMenu->Append(wxID_HTML_OPENFILE, _("&Open..."));
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_CLOSE, _("&Close"));

    wxMenu* helpMenu = new wxMenu;
    helpMenu->Append(wxID_ABOUT, _("&About..."));

    menuBar->Append(fileMenu,_("File"));
    menuBar->Append(helpMenu,_("Help"));
    SetMenuBar(menuBar);
#endif

//    CreateStatusBar();

    // Might do this
//    m_HtmlHelpWin->GetHtmlWindow()->SetRelatedFrame(this, m_TitleFormat);
//    m_HtmlHelpWin->GetHtmlWindow()->SetRelatedStatusBar(0);

    return TRUE;
}

wxHtmlHelpFrameEx::~wxHtmlHelpFrameEx()
{
}

void wxHtmlHelpFrameEx::SetTitleFormat(const wxString& format)
{
    if (GetHelpWindow() && GetHelpWindow()->GetHtmlWindow())
        GetHelpWindow()->GetHtmlWindow()->SetRelatedFrame(this, format);
    m_TitleFormat = format;
}

/*
EVENT HANDLING :
*/


void wxHtmlHelpFrameEx::OnActivate(wxActivateEvent& event)
{
    // This saves one mouse click when using the
    // wxHTML for context sensitive help systems
#ifndef __WXGTK__
    // NB: wxActivateEvent is a bit broken in wxGTK
    //     and is sometimes sent when it should not be
    if (event.GetActive() && m_HtmlHelpWin)
        m_HtmlHelpWin->GetHtmlWindow()->SetFocus();
#endif

    event.Skip();
}

void wxHtmlHelpFrameEx::OnCloseWindow(wxCloseEvent& evt)
{
    if (!IsIconized())
    {
        GetSize(& (m_HtmlHelpWin->GetCfgData().w), &(m_HtmlHelpWin->GetCfgData().h));
        GetPosition(& (m_HtmlHelpWin->GetCfgData().x), & (m_HtmlHelpWin->GetCfgData().y));
    }

#ifdef __WXGTK__
    if (IsGrabbed())
    {
        RemoveGrab();
    }
#endif
    

#if 0
    if (m_Config)
        WriteCustomization(m_Config, m_ConfigRoot);
#endif

    if (m_helpController)
    {
        m_helpController->OnCloseFrame(evt);
    }

    evt.Skip();
}

// Make the help controller's frame 'modal' if
// needed
void wxHtmlHelpFrameEx::AddGrabIfNeeded()
{
    // So far, wxGTK only
#ifdef __WXGTK__
    bool needGrab = FALSE;
    
    // Check if there are any modal windows present,
    // in which case we need to add a grab.
    for ( wxWindowList::Node * node = wxTopLevelWindows.GetFirst();
          node;
          node = node->GetNext() )
    {
        wxWindow *win = node->GetData();
        wxDialog *dialog = wxDynamicCast(win, wxDialog);

        if (dialog && dialog->IsModal())
            needGrab = TRUE;
    }

    if (needGrab)
        AddGrab();
#endif // __WXGTK__
}

#ifdef __WXMAC__
void wxHtmlHelpFrameEx::OnClose(wxCommandEvent& event)
{
    Close(TRUE);
}

void wxHtmlHelpFrameEx::OnAbout(wxCommandEvent& event)
{
    wxMessageBox(_("wxWidgets HTML Help Viewer (c) 1998-2003, Vaclav Slavik et al"), _("HelpView"),
        wxICON_INFORMATION|wxOK, this);
}
#endif

BEGIN_EVENT_TABLE(wxHtmlHelpFrameEx, wxFrame)
    EVT_ACTIVATE(wxHtmlHelpFrameEx::OnActivate)
    EVT_CLOSE(wxHtmlHelpFrameEx::OnCloseWindow)
#ifdef __WXMAC__
    EVT_MENU(wxID_CLOSE, wxHtmlHelpFrameEx::OnClose)
    EVT_MENU(wxID_ABOUT, wxHtmlHelpFrameEx::OnAbout)
#endif

END_EVENT_TABLE()

#endif // wxUSE_WXHTML_HELP

