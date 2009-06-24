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
 * Simple WX HTML viewing frame content, based on test.cpp from
 * wxWindows samples.
 *
 * $Id$
 *
 * Pawe³ Pa³ucha <pawel@praterm.com.pl>

 */

#ifndef __HTMLVIEW_H__
#define __HTMLVIEW_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/html/htmlwin.h>

/** wHtmlWindow wrapper - to catch link clinking events */
class MyHtmlWindow;

/**
 * This frame is a standalone HTML viewer with fixed file to open.
 * Great for displaying help...
 */
class HtmlViewerFrame : public wxFrame
{
public:
	/**
	 * @param file path to HTML file to open
	 * @param title window title
	 * @param pos initial window position
	 * @param size initial window size
	 */
	HtmlViewerFrame(const wxString file, wxWindow* parent,
			const wxString& title, 
			const wxPoint& pos = wxDefaultPosition, 
			const wxSize& size = wxDefaultSize);
	/** Event handler - closes frame (really calls Show(FALSE)). */
	void OnQuit(wxCommandEvent& event);
	/** Event handler - called when user tries to close window
	 * with system menu. */
	void OnClose(wxCloseEvent& event);
	/** Event handler - goes back in history. */
	void OnBack(wxCommandEvent& event);
	/** Event handler - goes forward in history. */
	void OnForward(wxCommandEvent& event);
	/** Checks for history availibility and updates history buttons
	 * status. */
	void UpdateHistoryButtons();
private:
	MyHtmlWindow *m_Html;	/**< Html window. */
	wxToolBar *tb;
	DECLARE_EVENT_TABLE()
};

#endif


