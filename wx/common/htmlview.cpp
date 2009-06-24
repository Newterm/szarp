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
#include "htmlview.h"
#include "fonts.h"

// ID for controls
enum  {
	ID_Quit = 1,
	ID_Back,
	ID_Forward
};

BEGIN_EVENT_TABLE(HtmlViewerFrame, wxFrame)
	EVT_MENU(ID_Quit,  HtmlViewerFrame::OnQuit)
	EVT_MENU(ID_Back, HtmlViewerFrame::OnBack)
	EVT_MENU(ID_Forward, HtmlViewerFrame::OnForward)
	EVT_CLOSE(HtmlViewerFrame::OnClose)
END_EVENT_TABLE()

class MyHtmlWindow : public wxHtmlWindow {
	public:
		MyHtmlWindow(HtmlViewerFrame* direct_parent,
			wxWindow *parent, wxWindowID id = -1, 
			const wxPoint& pos = wxDefaultPosition, 
			const wxSize& size = wxDefaultSize, 
			long style = wxHW_SCROLLBAR_AUTO, 
			const wxString& name = _T("htmlWindow"));
		virtual void OnLinkClicked(const wxHtmlLinkInfo& link);
	private:
		HtmlViewerFrame* direct_parent;
};

MyHtmlWindow::MyHtmlWindow(HtmlViewerFrame* _direct_parent,
			wxWindow *parent, wxWindowID id, const wxPoint& pos, 
			const wxSize& size, long style, const wxString& name)
	: wxHtmlWindow(parent, id, pos, size, style, name),
	direct_parent(_direct_parent)
{
	szSetDefFont(this);
	int sizes[] = { 8, 10, 12, 14, 16, 18, 20 };
	SetFonts(_T(""), _T(""), sizes);
}

void MyHtmlWindow::OnLinkClicked(const wxHtmlLinkInfo& link)
{
	wxHtmlWindow::OnLinkClicked(link);
	direct_parent->UpdateHistoryButtons();
}

HtmlViewerFrame::HtmlViewerFrame(const wxString file, wxWindow* parent,
		const wxString& title, 
		const wxPoint& pos, const wxSize& size)
	: wxFrame(parent, -1, title, pos, size, wxDEFAULT_FRAME_STYLE, _T("html_viewer"))
{
	szSetDefFont(this);
	tb = CreateToolBar(wxTB_HORIZONTAL | wxTB_FLAT |
		wxTB_DOCKABLE);
	wxBitmap b1, b2, b3;
	b1.LoadFile(_T("/opt/szarp/resources/wx/icons/exit.xpm"));
	b2.LoadFile(_T("/opt/szarp/resources/wx/icons/back.xpm"));
	b3.LoadFile(_T("/opt/szarp/resources/wx/icons/forward.xpm"));
	
	tb->AddTool(ID_Quit, _("Quit"), b1, _("Close window"));
	tb->AddSeparator();
	tb->AddTool(ID_Back, _("Back"), b2, _("Go back"));
	tb->AddTool(ID_Forward, _("Forward"), b3, _("Go forward"));
	tb->EnableTool(ID_Back, FALSE);
	tb->EnableTool(ID_Forward, FALSE);
	CreateStatusBar(1);
	
	m_Html = new MyHtmlWindow(this, this);
	m_Html->SetRelatedFrame(this, _("SZARP help: %s"));
	m_Html->SetRelatedStatusBar(0);
	//      m_Html->ReadCustomization(wxConfig::Get());
	m_Html->LoadPage(file);
}
	
	
void HtmlViewerFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	if (GetParent() == NULL) {
		Close(TRUE);
	} else {
		Show(FALSE);
	}
}

void HtmlViewerFrame::OnClose(wxCloseEvent& event)
{
	if (event.CanVeto()) {
		event.Veto();
		Show(FALSE);
	} else {
		Destroy();
	}
}

void HtmlViewerFrame::OnBack(wxCommandEvent& WXUNUSED(event))
{
	m_Html->HistoryBack();
	UpdateHistoryButtons();
}

void HtmlViewerFrame::OnForward(wxCommandEvent& WXUNUSED(event))
{
	m_Html->HistoryForward();
	UpdateHistoryButtons();
}

void HtmlViewerFrame::UpdateHistoryButtons()
{
	tb->EnableTool(ID_Back, m_Html->HistoryCanBack());
	tb->EnableTool(ID_Forward, m_Html->HistoryCanForward());
}
