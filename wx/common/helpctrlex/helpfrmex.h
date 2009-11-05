/////////////////////////////////////////////////////////////////////////////
// Name:        helpfrmex.h
// Purpose:     wxHtmlHelpFrameEx
// Notes:       Based on htmlhelp.cpp, implementing a monolithic
//              HTML Help controller class,  by Vaclav Slavik
// Author:      Harm van der Heijden, Vaclav Slavik and Julian Smart
// RCS-ID:      $Id: helpfrmex.h,v 1.1 2004/03/20 16:27:21 js Exp $
// Copyright:   (c) Harm van der Heijden, Vaclav Slavik, Julian Smart
// Licence:     wxWidgets licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_HELPFRMEX_H_
#define _WX_HELPFRMEX_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "helpfrmex.h"
#endif

#include "wx/defs.h"

#if wxUSE_WXHTML_HELP

#include "wx/html/helpdata.h"
#include "wx/window.h"
#include "wx/frame.h"
#include "wx/config.h"
#include "wx/splitter.h"
#include "wx/notebook.h"
#include "wx/listbox.h"
#include "wx/choice.h"
#include "wx/combobox.h"
#include "wx/checkbox.h"
#include "wx/stattext.h"
#include "wx/html/htmlwin.h"
#include "wx/html/htmprint.h"

#include "helpwinex.h"

class wxHtmlHelpControllerEx;

class wxHtmlHelpFrameEx : public wxFrame
{
public:
    wxHtmlHelpFrameEx(wxHtmlHelpData* data = NULL) { Init(data); }
    wxHtmlHelpFrameEx(wxWindow* parent, wxWindowID wxWindowID,
                    const wxString& title = wxEmptyString,
                    int style = wxHF_DEFAULT_STYLE, wxHtmlHelpData* data = NULL,
                    wxConfigBase *config = NULL, const wxString& rootpath = wxEmptyString);
    bool Create(wxWindow* parent, wxWindowID id, const wxString& title = wxEmptyString,
                int style = wxHF_DEFAULT_STYLE,
                wxConfigBase *config = NULL, const wxString& rootpath = wxEmptyString);

    ~wxHtmlHelpFrameEx();

    /// Returns the data associated with this frame.
    wxHtmlHelpData* GetData() { return m_Data; }

    /// Returns the controller that created this frame.
    wxHtmlHelpControllerEx* GetController() const { return m_helpController; }

    /// Sets the controller associated with this frame.
    void SetController(wxHtmlHelpControllerEx* controller) { m_helpController = controller; }

    /// Returns the help window.
    wxHtmlHelpWindowEx* GetHelpWindow() const { return m_HtmlHelpWin; }

    // Sets format of title of the frame. Must contain exactly one "%s"
    // (for title of displayed HTML page)
    void SetTitleFormat(const wxString& format);

    // Make the help controller's frame 'modal' if
    // needed
    void AddGrabIfNeeded();

protected:
    void Init(wxHtmlHelpData* data = NULL);

    void OnCloseWindow(wxCloseEvent& event);
    void OnActivate(wxActivateEvent& event);

#ifdef __WXMAC__
    void OnClose(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
#endif

protected:
    // Temporary pointer to pass to window
    wxHtmlHelpData* m_Data;
    wxString m_TitleFormat;  // title of the help frame
    wxHtmlHelpWindowEx *m_HtmlHelpWin;
    wxHtmlHelpControllerEx* m_helpController;

    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxHtmlHelpFrameEx)
};

#endif // wxUSE_WXHTML_HELP

#endif

