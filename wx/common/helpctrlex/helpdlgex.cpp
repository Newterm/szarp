/////////////////////////////////////////////////////////////////////////////
// Name:        newhelpdlg.cpp
// Purpose:     wxHtmlHelpDialogEx
// Notes:       Based on htmlhelp.cpp, implementing a monolithic
//              HTML Help controller class,  by Vaclav Slavik
// Author:      Harm van der Heijden, Vaclav Slavik and Julian Smart
// RCS-ID:      $Id: helpdlgex.cpp,v 1.3 2004/08/09 12:19:15 js Exp $
// Copyright:   (c) Harm van der Heijden, Vaclav Slavik and Julian Smart
// Licence:     wxWidgets licence
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "helpdlgex.h"
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

#include "helpdlgex.h"
#include "helpctrlex.h"

// what is considered "small index"?
#define INDEX_IS_SMALL 100

/* Motif defines this as a macro */
#ifdef Below
#undef Below
#endif

IMPLEMENT_DYNAMIC_CLASS(wxHtmlHelpDialogEx, wxDialog)

wxHtmlHelpDialogEx::wxHtmlHelpDialogEx(wxWindow* parent, wxWindowID id, const wxString& title,
                                 int style, wxHtmlHelpData* data,
                                 wxConfigBase *config, const wxString& rootpath)
{
    Init(data);
    Create(parent, id, title, style, config, rootpath);
}

void wxHtmlHelpDialogEx::Init(wxHtmlHelpData* data)
{
    // Simply pass the pointer on to the help window
    m_Data = data;
    m_HtmlHelpWin = NULL;
    m_helpController = NULL;
}

// Create: builds the GUI components.
bool wxHtmlHelpDialogEx::Create(wxWindow* parent, wxWindowID id,
                             const wxString& WXUNUSED(title), int style,
                             wxConfigBase *config, const wxString& rootpath)
{
    m_HtmlHelpWin = new wxHtmlHelpWindowEx(m_Data);
    if (config)
        m_HtmlHelpWin->UseConfig(config, rootpath);

    wxDialog::Create(parent, id, _("Help"), 
                    wxPoint(m_HtmlHelpWin->GetCfgData().x, m_HtmlHelpWin->GetCfgData().y),
                    wxSize(m_HtmlHelpWin->GetCfgData().w, m_HtmlHelpWin->GetCfgData().h), 
                    wxDEFAULT_FRAME_STYLE|wxRESIZE_BORDER, wxT("wxHtmlHelp"));
    m_HtmlHelpWin->Create(this, -1, wxDefaultPosition, wxDefaultSize,
        wxTAB_TRAVERSAL|wxNO_BORDER, style);

    GetPosition(& (m_HtmlHelpWin->GetCfgData().x), & (m_HtmlHelpWin->GetCfgData()).y);

    SetIcon(wxArtProvider::GetIcon(wxART_HELP, wxART_HELP_BROWSER));

    // Might do this
//    m_HtmlHelpWin->GetHtmlWindow()->SetRelatedFrame(this, m_TitleFormat);
//    m_HtmlHelpWin->GetHtmlWindow()->SetRelatedStatusBar(0);

    wxWindow* item1 = this;
    wxBoxSizer* item2 = new wxBoxSizer(wxVERTICAL);
    item1->SetSizer(item2);
    item1->SetAutoLayout(TRUE);

    wxWindow* item3 = m_HtmlHelpWin;
    item2->Add(item3, 1, wxGROW|wxALL, 5);

    wxBoxSizer* item4 = new wxBoxSizer(wxHORIZONTAL);
    item2->Add(item4, 0, wxGROW, 5);

    item4->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* item6 = new wxButton(item1, wxID_OK, _("&Close"), wxDefaultPosition, wxDefaultSize, 0);
    item4->Add(item6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 10);
#ifdef __WXMAC__
    // Add some space for the resize handle
    item4->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL, 0);
#endif
    
    Layout();
    Centre();

#if wxCHECK_VERSION(2, 5, 2)
    // Reduce flicker by updating the splitter pane sizes before the
    // frame is shown
    wxSizeEvent sizeEvent(GetSize(), GetId());
    ProcessEvent(sizeEvent);

    m_HtmlHelpWin->GetSplitterWindow()->UpdateSize();
#endif

    return TRUE;
}

wxHtmlHelpDialogEx::~wxHtmlHelpDialogEx()
{
}

void wxHtmlHelpDialogEx::SetTitleFormat(const wxString& format)
{
    m_TitleFormat = format;
}

/*
EVENT HANDLING :
*/


void wxHtmlHelpDialogEx::OnCloseWindow(wxCloseEvent& evt)
{
    GetSize(& (m_HtmlHelpWin->GetCfgData().w), &(m_HtmlHelpWin->GetCfgData().h));
    GetPosition(& (m_HtmlHelpWin->GetCfgData().x), & (m_HtmlHelpWin->GetCfgData().y));

    if (m_helpController)
    {
        m_helpController->OnCloseFrame(evt);
    }

    evt.Skip();
}

BEGIN_EVENT_TABLE(wxHtmlHelpDialogEx, wxDialog)
    EVT_CLOSE(wxHtmlHelpDialogEx::OnCloseWindow)
END_EVENT_TABLE()

#endif // wxUSE_WXHTML_HELP

