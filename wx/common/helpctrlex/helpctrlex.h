/////////////////////////////////////////////////////////////////////////////
// Name:        helpctrlex.h
// Purpose:     wxHtmlHelpControllerEx
// Notes:       Implements an extended version of the wxHTML help controller
// Author:      Harm van der Heijden, Vaclav Slavik, Julian Smart
// RCS-ID:      $Id: helpctrlex.h,v 1.2 2004/03/21 00:01:37 js Exp $
// Copyright:   (c) Harm van der Heijden, Vaclav Slavik, Julian Smart
// Licence:     wxWidgets licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_HELPCTRLEX_H_
#define _WX_HELPCTRLEX_H_

#include "wx/defs.h"

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "helpctrlex.h"
#endif

#if wxUSE_WXHTML_HELP

// This style indicates that the window is
// embedded in the application and must not be
// destroyed by the help controller.
#define wxHF_EMBEDDED                0x00008000

// Create a dialog for the help window.
#define wxHF_DIALOG                  0x00010000

// Create a frame for the help window.
#define wxHF_FRAME                   0x00020000

// Make the dialog modal when displaying help.
#define wxHF_MODAL                   0x00040000

#include "helpwinex.h"
#include "helpfrmex.h"
#include "helpdlgex.h"
#include "wx/helpbase.h"

#define wxID_HTML_WINDOW   (wxID_HIGHEST + 2)

class WXDLLEXPORT wxHtmlHelpControllerEx : public wxHelpControllerBase
{
    DECLARE_DYNAMIC_CLASS(wxHtmlHelpControllerEx)

public:
    wxHtmlHelpControllerEx(int style = wxHF_DEFAULT_STYLE);
    virtual ~wxHtmlHelpControllerEx();

    void SetTitleFormat(const wxString& format);
    void SetTempDir(const wxString& path) { m_helpData.SetTempDir(path); }
    bool AddBook(const wxString& book_url, bool show_wait_msg = FALSE);
    bool AddBook(const wxFileName& book_file, bool show_wait_msg = FALSE);

    bool Display(const wxString& x);
    bool Display(int id);
    bool DisplayContents();
    bool DisplayIndex();
    bool KeywordSearch(const wxString& keyword
#if wxCHECK_VERSION(2,5,0)
        , wxHelpSearchMode mode = wxHELP_SEARCH_ALL
#endif
        );

    wxHtmlHelpWindowEx* GetHelpWindow() { return m_helpWindow; }
    void SetHelpWindow(wxHtmlHelpWindowEx* helpWindow) { m_helpWindow = helpWindow; }

    // wxHtmlHelpFrame* GetFrame() { return m_helpFrame; }
    void UseConfig(wxConfigBase *config, const wxString& rootpath = wxEmptyString);

    // Assigns config object to the Ctrl. This config is then
    // used in subsequent calls to Read/WriteCustomization of both help
    // Ctrl and it's wxHtmlWindow
    virtual void ReadCustomization(wxConfigBase *cfg, const wxString& path = wxEmptyString);
    virtual void WriteCustomization(wxConfigBase *cfg, const wxString& path = wxEmptyString);

    //// Backward compatibility with wxHelpController API

    virtual bool Initialize(const wxString& file, int WXUNUSED(server) ) { return Initialize(file); }
    virtual bool Initialize(const wxString& file);
    virtual void SetViewer(const wxString& WXUNUSED(viewer), long WXUNUSED(flags) = 0) {}
    virtual bool LoadFile(const wxString& file = wxT(""));
    virtual bool DisplaySection(int sectionNo);
    virtual bool DisplaySection(const wxString& section) { return Display(section); }
    virtual bool DisplayBlock(long blockNo) { return DisplaySection(blockNo); }
    virtual bool DisplayTextPopup(const wxString& text, const wxPoint& pos);

    virtual void SetParentWindow(wxWindow* parent) { m_ParentWindow = parent; }

    virtual void SetFrameParameters(const wxString& title,
                               const wxSize& size,
                               const wxPoint& pos = wxDefaultPosition,
                               bool newFrameEachTime = FALSE);

    /// Obtains the latest settings used by the help frame and the help
    /// frame.
    virtual wxFrame *GetFrameParameters(wxSize *size = NULL,
                               wxPoint *pos = NULL,
                               bool *newFrameEachTime = NULL);

    // Get direct access to help data:
    wxHtmlHelpData *GetHelpData() { return &m_helpData; }

    virtual bool Quit() ;
    virtual void OnQuit() {};

    void OnCloseFrame(wxCloseEvent& evt);

    // Make the help controller's frame 'modal' if
    // needed
    void MakeModalIfNeeded();

    // Find the top-most parent window
    wxWindow* FindTopLevelWindow();

protected:
    virtual wxWindow* CreateHelpWindow();
    virtual wxHtmlHelpFrameEx* CreateHelpFrame(wxHtmlHelpData *data);
    virtual wxHtmlHelpDialogEx* CreateHelpDialog(wxHtmlHelpData *data);
    virtual void DestroyHelpWindow();

    wxHtmlHelpData      m_helpData;
    wxHtmlHelpWindowEx*   m_helpWindow;
    wxConfigBase *      m_Config;
    wxString            m_ConfigRoot;
    wxString            m_titleFormat;
    int                 m_FrameStyle;
    wxWindow*           m_ParentWindow; // parent for frame/dialog

    // DECLARE_EVENT_TABLE()

    DECLARE_NO_COPY_CLASS(wxHtmlHelpControllerEx)
};

/*
 * wxModalHelp
 * A convenience class particularly for use on wxMac,
 * where you can only show modal dialogs from a modal
 * dialog.
 *
 * Use like this:
 *
 * wxModalHelp help(parent, filename, topic);
 *
 * If topic is empty, the help contents is displayed.
 */

class wxModalHelp
{
public:
    wxModalHelp(wxWindow* parent, const wxString& helpFile, const wxString& topic = wxEmptyString,
        int style = wxHF_FLAT_TOOLBAR | wxHF_CONTENTS | wxHF_INDEX | wxHF_SEARCH | wxHF_DIALOG | wxHF_MODAL);
};

#endif // wxUSE_WXHTML_HELP

#endif // _WX_HELPCTRLEX_H_

