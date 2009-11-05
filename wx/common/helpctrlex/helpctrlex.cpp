/////////////////////////////////////////////////////////////////////////////
// Name:        helpctrlex.cpp
// Purpose:     wxHtmlHelpControllerEx
// Notes:       Based on htmlhelp.cpp, implementing a monolithic
//              HTML Help controller class,  by Vaclav Slavik
// Author:      Harm van der Heijden, Vaclav Slavik, and Julian Smart
// RCS-ID:      $Id: helpctrlex.cpp,v 1.4 2004/09/27 09:53:21 js Exp $
// Copyright:   (c) Harm van der Heijden, Vaclav Slavik and Julian Smart
// Licence:     wxWidgets licence
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "helpctrlex.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#if wxUSE_WXHTML_HELP

#ifndef WX_PRECOMP
    #include "wx/app.h"
    #include "wx/intl.h"
#endif // WX_PRECOMP

#include "wx/busyinfo.h"

#ifdef __WXGTK__
    // for the hack in AddGrabIfNeeded()
    #include "wx/dialog.h"
#endif // __WXGTK__

#if wxUSE_HELP
    #include "wx/tipwin.h"
#endif

#include "helpctrlex.h"
#include "helpfrmex.h"

IMPLEMENT_DYNAMIC_CLASS(wxHtmlHelpControllerEx, wxHelpControllerBase)

wxHtmlHelpControllerEx::wxHtmlHelpControllerEx(int style)
{
    m_helpWindow = NULL;
    m_Config = NULL;
    m_ConfigRoot = wxEmptyString;
    m_titleFormat = _("Help: %s");
    m_FrameStyle = style;
    m_ParentWindow = NULL;
}

wxHtmlHelpControllerEx::~wxHtmlHelpControllerEx()
{
    if (m_Config)
        WriteCustomization(m_Config, m_ConfigRoot);
    if (m_helpWindow)
    {
        DestroyHelpWindow();
    }
}

void wxHtmlHelpControllerEx::DestroyHelpWindow()
{
    if (m_FrameStyle & wxHF_EMBEDDED)
        return;

    // Find top-most parent window
    // If a modal dialog
    wxWindow* parent = FindTopLevelWindow();
    if (parent)
    {
        wxDialog* dialog = wxDynamicCast(parent, wxDialog);
        if (dialog && dialog->IsModal())
        {
            dialog->EndModal(wxID_OK);
        }
        parent->Destroy();
        m_helpWindow = NULL;
    }
}

void wxHtmlHelpControllerEx::OnCloseFrame(wxCloseEvent& evt)
{
    evt.Skip();

    OnQuit();

    m_helpWindow->SetController(NULL);
    m_helpWindow = NULL;
}

void wxHtmlHelpControllerEx::SetTitleFormat(const wxString& title)
{
    m_titleFormat = title;
    wxHtmlHelpFrameEx* frame = wxDynamicCast(FindTopLevelWindow(), wxHtmlHelpFrameEx);
    wxHtmlHelpDialogEx* dialog = wxDynamicCast(FindTopLevelWindow(), wxHtmlHelpDialogEx);
    if (frame)
    {
        frame->SetTitleFormat(title);
    }
    else if (dialog)
        dialog->SetTitleFormat(title);
}

// Find the top-most parent window
wxWindow* wxHtmlHelpControllerEx::FindTopLevelWindow()
{
    wxWindow* parent = m_helpWindow;
    while (parent && !parent->IsKindOf(CLASSINFO(wxTopLevelWindow)))
    {
        parent = parent->GetParent();
    }
    return parent;
}

bool wxHtmlHelpControllerEx::AddBook(const wxFileName& book_file, bool show_wait_msg)
{
    return AddBook(wxFileSystem::FileNameToURL(book_file), show_wait_msg);
}

bool wxHtmlHelpControllerEx::AddBook(const wxString& book, bool show_wait_msg)
{
    wxBusyCursor cur;
#if wxUSE_BUSYINFO
    wxBusyInfo* busy = NULL;
    wxString info;
    if (show_wait_msg)
    {
        info.Printf(_("Adding book %s"), book.c_str());
        busy = new wxBusyInfo(info);
    }
#endif
    bool retval = m_helpData.AddBook(book);
#if wxUSE_BUSYINFO
    if (show_wait_msg)
        delete busy;
#endif
    if (m_helpWindow) 
        m_helpWindow->RefreshLists();
    return retval;
}



wxHtmlHelpFrameEx *wxHtmlHelpControllerEx::CreateHelpFrame(wxHtmlHelpData *data)
{
    wxHtmlHelpFrameEx* frame = new wxHtmlHelpFrameEx(data);
    frame->SetController(this);
    frame->SetTitleFormat(m_titleFormat);    
    frame->Create(m_ParentWindow, -1, wxEmptyString, m_FrameStyle, m_Config, m_ConfigRoot);
    return frame;
}

wxHtmlHelpDialogEx *wxHtmlHelpControllerEx::CreateHelpDialog(wxHtmlHelpData *data)
{
    wxHtmlHelpDialogEx* dialog = new wxHtmlHelpDialogEx(data);
    dialog->SetController(this);
    dialog->SetTitleFormat(m_titleFormat);    
    dialog->Create(m_ParentWindow, -1, wxEmptyString, m_FrameStyle, m_Config, m_ConfigRoot);
    return dialog;
}

wxWindow* wxHtmlHelpControllerEx::CreateHelpWindow()
{
    if (m_helpWindow)
    {
        if (m_FrameStyle & wxHF_EMBEDDED)
            return m_helpWindow;

        wxWindow* topLevelWindow = FindTopLevelWindow();
        if (topLevelWindow)
            topLevelWindow->Raise();
        return m_helpWindow;
    }

    if (m_Config == NULL)
    {
        m_Config = wxConfigBase::Get(FALSE);
        if (m_Config != NULL)
            m_ConfigRoot = _T("wxWindows/wxHtmlHelpControllerEx");
    }

    if (m_FrameStyle & wxHF_FRAME)
    {
        wxHtmlHelpFrameEx* frame = CreateHelpFrame(&m_helpData);
        m_helpWindow = frame->GetHelpWindow();
        frame->Show(TRUE);
    }
    else if (m_FrameStyle & wxHF_DIALOG)
    {
        wxHtmlHelpDialogEx* dialog = CreateHelpDialog(&m_helpData);
        m_helpWindow = dialog->GetHelpWindow();
    }
    else
    {
        m_helpWindow = new wxHtmlHelpWindowEx(m_ParentWindow, -1, wxDefaultPosition, wxDefaultSize,
            wxTAB_TRAVERSAL|wxNO_BORDER, m_FrameStyle, &m_helpData);
    }

    return m_helpWindow;
}

void wxHtmlHelpControllerEx::ReadCustomization(wxConfigBase* cfg, const wxString& path)
{
    /* should not be called by the user; call UseConfig, and the controller
     * will do the rest */
    if (m_helpWindow && cfg)
        m_helpWindow->ReadCustomization(cfg, path);
}

void wxHtmlHelpControllerEx::WriteCustomization(wxConfigBase* cfg, const wxString& path)
{
    /* typically called by the controllers OnCloseFrame handler */
    if (m_helpWindow && cfg)
        m_helpWindow->WriteCustomization(cfg, path);
}

void wxHtmlHelpControllerEx::UseConfig(wxConfigBase *config, const wxString& rootpath)
{
    m_Config = config;
    m_ConfigRoot = rootpath;
    if (m_helpWindow) m_helpWindow->UseConfig(config, rootpath);
    ReadCustomization(config, rootpath);
}

//// Backward compatibility with wxHelpController API

bool wxHtmlHelpControllerEx::Initialize(const wxString& file)
{
    wxString dir, filename, ext;
    wxSplitPath(file, & dir, & filename, & ext);

    if (!dir.IsEmpty())
        dir = dir + wxFILE_SEP_PATH;

    // Try to find a suitable file
    wxString actualFilename = dir + filename + wxString(wxT(".zip"));
    if (!wxFileExists(actualFilename))
    {
        actualFilename = dir + filename + wxString(wxT(".htb"));
        if (!wxFileExists(actualFilename))
        {
            actualFilename = dir + filename + wxString(wxT(".hhp"));
            if (!wxFileExists(actualFilename))
            {
#if wxUSE_LIBMSPACK
                actualFilename = dir + filename + wxString(wxT(".chm"));
                if (!wxFileExists(actualFilename))
#endif
                    return false;
            }
        }
    }

    return AddBook(wxFileName(actualFilename));
}

bool wxHtmlHelpControllerEx::LoadFile(const wxString& WXUNUSED(file))
{
    // Don't reload the file or we'll have it appear again, presumably.
    return TRUE;
}

bool wxHtmlHelpControllerEx::DisplaySection(int sectionNo)
{
    return Display(sectionNo);
}

bool wxHtmlHelpControllerEx::DisplayTextPopup(const wxString& text, const wxPoint& WXUNUSED(pos))
{
#if wxUSE_TIPWINDOW
    static wxTipWindow* s_tipWindow = NULL;

    if (s_tipWindow)
    {
        // Prevent s_tipWindow being nulled in OnIdle,
        // thereby removing the chance for the window to be closed by ShowHelp
        s_tipWindow->SetTipWindowPtr(NULL);
        s_tipWindow->Close();
    }
    s_tipWindow = NULL;

    if ( !text.empty() )
    {
        s_tipWindow = new wxTipWindow(wxTheApp->GetTopWindow(), text, 100, & s_tipWindow);

        return TRUE;
    }
#endif // wxUSE_TIPWINDOW

    return FALSE;
}

void wxHtmlHelpControllerEx::SetFrameParameters(const wxString& title,
                                   const wxSize& size,
                                   const wxPoint& pos,
                                   bool WXUNUSED(newFrameEachTime))
{
    SetTitleFormat(title);
    wxHtmlHelpFrameEx* frame = wxDynamicCast(FindTopLevelWindow(), wxHtmlHelpFrameEx);
    wxHtmlHelpDialogEx* dialog = wxDynamicCast(FindTopLevelWindow(), wxHtmlHelpDialogEx);
    if (frame)
        frame->SetSize(pos.x, pos.y, size.x, size.y);
    else if (dialog)
        dialog->SetSize(pos.x, pos.y, size.x, size.y);
}

wxFrame* wxHtmlHelpControllerEx::GetFrameParameters(wxSize *size,
                                   wxPoint *pos,
                                   bool *newFrameEachTime)
{
    if (newFrameEachTime)
        (* newFrameEachTime) = FALSE;

    wxHtmlHelpFrameEx* frame = wxDynamicCast(FindTopLevelWindow(), wxHtmlHelpFrameEx);
    wxHtmlHelpDialogEx* dialog = wxDynamicCast(FindTopLevelWindow(), wxHtmlHelpDialogEx);
    if (frame)
    {
        if (size)
            (* size) = frame->GetSize();
        if (pos)
            (* pos) = frame->GetPosition();
        return frame;
    }
    else if (dialog)
    {
        if (size)
            (* size) = dialog->GetSize();
        if (pos)
            (* pos) = dialog->GetPosition();
        return NULL;
    }
    return NULL;
}

bool wxHtmlHelpControllerEx::Quit()
{
    DestroyHelpWindow();
    return TRUE;
}

// Make the help controller's frame 'modal' if
// needed
void wxHtmlHelpControllerEx::MakeModalIfNeeded()
{
    if ((m_FrameStyle & wxHF_EMBEDDED) == 0)
    {
        wxHtmlHelpFrameEx* frame = wxDynamicCast(FindTopLevelWindow(), wxHtmlHelpFrameEx);
        wxHtmlHelpDialogEx* dialog = wxDynamicCast(FindTopLevelWindow(), wxHtmlHelpDialogEx);
        if (frame)
            frame->AddGrabIfNeeded();
        else if (dialog && (m_FrameStyle & wxHF_MODAL))
        {
            dialog->ShowModal();
        }
    }
}


bool wxHtmlHelpControllerEx::Display(const wxString& x)
{
    CreateHelpWindow();
    bool success = m_helpWindow->Display(x);
    MakeModalIfNeeded();
    return success;    
}

bool wxHtmlHelpControllerEx::Display(int id)
{
    CreateHelpWindow();
    bool success = m_helpWindow->Display(id);
    MakeModalIfNeeded();
    return success;
}

bool wxHtmlHelpControllerEx::DisplayContents()
{
    CreateHelpWindow();
    bool success = m_helpWindow->DisplayContents();
    MakeModalIfNeeded();
    return success;
}

bool wxHtmlHelpControllerEx::DisplayIndex()
{
    CreateHelpWindow();
    bool success = m_helpWindow->DisplayIndex();
    MakeModalIfNeeded();
    return success;
}

bool wxHtmlHelpControllerEx::KeywordSearch(const wxString& keyword
#if wxCHECK_VERSION(2,5,0)
        , wxHelpSearchMode mode
#endif
     )
{
    CreateHelpWindow();
    bool success = m_helpWindow->KeywordSearch(keyword
#if wxCHECK_VERSION(2,5,0)
        , mode
#endif
        );
    MakeModalIfNeeded();
    return success;
}

/*
 * wxModalHelp
 * A convenience class, to use like this:
 *
 * wxModalHelp help(parent, helpFile, topic);
 */

wxModalHelp::wxModalHelp(wxWindow* parent, const wxString& helpFile, const wxString& topic, int style)
{
    // Force some mandatory styles
    style |= wxHF_DIALOG | wxHF_MODAL;

    wxHtmlHelpControllerEx controller(style);
    controller.Initialize(helpFile);
    controller.SetParentWindow(parent);

    if (topic.IsEmpty())
        controller.DisplayContents();
    else
        controller.DisplaySection(topic);
}

#endif // wxUSE_WXHTML_HELP

