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
 * scc - Szarp Control Center
 * SZARP

 * Paweł Pałucha pawel@praterm.com.pl
 *
 * $Id$
 */

#include "sccframe.h"
#include "sccapp.h"
#include "sccmenu.h"
#include "scchideselectionframe.h"
#include <wx/log.h>
#include <wx/image.h>
#include <wx/config.h>
#include "htmlabout.h"
#include "cconv.h"
#include "szarpfonts.h"
#include "version.h"

#ifndef MINGW32
#include "../../resources/wx/icons/szarp64.xpm"
#endif

/**
 * Program events' ids
 */
enum {
	ID_Quit = wxID_HIGHEST,
	ID_About,
	ID_Help,
	ID_Timer,
	ID_SupportTunnel,
	ID_SzarpDataDir,
	ID_Fonts,
	ID_HideBases
};

BEGIN_EVENT_TABLE(SCCTaskBarItem, szTaskBarItem)
    EVT_TASKBAR_LEFT_DOWN(SCCTaskBarItem::OnMouseDown)
    EVT_TASKBAR_RIGHT_DOWN(SCCTaskBarItem::OnMouseMiddleDown)
    EVT_MENU(ID_Quit, SCCTaskBarItem::OnQuit)
    EVT_MENU(ID_About, SCCTaskBarItem::OnAbout)
    EVT_MENU(ID_SupportTunnel, SCCTaskBarItem::OnSupportTunnel)
#ifndef MINGW32
    EVT_MENU(ID_Fonts, SCCTaskBarItem::OnFonts)
#endif
    EVT_MENU(ID_SzarpDataDir, SCCTaskBarItem::OnSzarpDataDir)
    EVT_MENU(ID_Help, SCCTaskBarItem::OnHelp)
    EVT_MENU_RANGE(SCC_MENU_FIRST_ID, SCC_MENU_MAX_ID,SCCTaskBarItem::OnMenu)
    EVT_MENU(ID_HideBases, SCCTaskBarItem::OnHideBases)
    EVT_CLOSE(SCCTaskBarItem::OnClose)
END_EVENT_TABLE()

SCCTaskBarItem::SCCTaskBarItem(SCCMenu* _menu, wxString prefix,	wxString suffix) :
	system_menu(NULL), menu(NULL), new_menu(_menu), wxmenu(NULL),
	m_sel_frame(NULL), tunnel_frame(NULL)

{
	wxLog *logger=new wxLogStderr();
	wxLog::SetActiveTarget(logger);

	system_menu = CreateSystemMenu();

	help = new szHelpController;

	wxIcon icon(wxICON(szarp64));
	if (icon.Ok())
		SetIcon(icon, _("Start SZARP applications"));
}

void SCCTaskBarItem::ReplaceMenu(SCCMenu *_menu) {
	if (new_menu) {
		SCCMenu::Destroy(new_menu);
	}
	new_menu = _menu;
}


SCCTaskBarItem::~SCCTaskBarItem()
{
    m_sel_frame->Destroy();
	delete system_menu;
}

wxMenu* SCCTaskBarItem::CreateSystemMenu()
{
	wxMenu* m;

	m = new wxMenu();
	assert(m != NULL);
	m->Append(ID_Help, _("&Help"));
	m->Append(ID_About, _("&About"));
#ifndef MINGW32
	m->Append(ID_SupportTunnel, _("&Support tunnel"));
	m->Append(ID_Fonts, _("SZARP &fonts"));
#endif
	m->Append(ID_SzarpDataDir, _("SZARP &data directory"));
	m->Append(ID_HideBases, _("&Hide SZARP databases"));
	m->AppendSeparator();
	m->Append(ID_Quit, _("E&xit"));
	return m;
}

void SCCTaskBarItem::OnMouseDown(wxTaskBarIconEvent& event)
{
	if (new_menu != NULL) {
		if (menu != NULL) {
			SCCMenu::Destroy(menu);
		}
		if (wxmenu != NULL)
			delete wxmenu;

		menu = new_menu;
		new_menu = NULL;

    		if (m_sel_frame == NULL)
	            m_sel_frame = new SCCSelectionFrame();
        
		wxmenu = menu->CreateWxMenu(m_sel_frame->GetHiddenDatabases());
	}
	PopupMenu(wxmenu);
}

void SCCTaskBarItem::OnMouseMiddleDown(wxTaskBarIconEvent& event)
{
	PopupMenu(system_menu);
}

void SCCTaskBarItem::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	if (wxMessageBox(_("Do you want to close the application?"),
				_("Question"), wxICON_QUESTION | wxYES_NO)
			!= wxYES)
		return;
	else
		wxExit();
}

void SCCTaskBarItem::OnAbout(wxCommandEvent& WXUNUSED(event))
{
	wxGetApp().ShowAbout();
}

void SCCTaskBarItem::OnSupportTunnel(wxCommandEvent& WXUNUSED(event))
{
	if (tunnel_frame == NULL || !tunnel_frame->IsShown())
		if (wxMessageBox(_("Do you really want to activate 'IT support tunnel'?"), _("IT support tunnel"), wxICON_QUESTION | wxYES_NO) != wxYES)
		return;

	if (!tunnel_frame)
		tunnel_frame = new SCCTunnelFrame(NULL);
	tunnel_frame->Show();
	tunnel_frame->Centre();

}

void SCCTaskBarItem::OnSzarpDataDir(wxCommandEvent& WXUNUSED(event))
{
	int ret = wxMessageBox(_("This option allows you to change directory where SZARP stores its historical data. Changing this setting may cause "
			" unavailability of already synchronized data. You must also choose directory that you have write access to."),
			_("Warning"),
			wxOK | wxCANCEL | wxICON_INFORMATION);

	if (ret != wxOK)
		return;

	wxString path = wxGetApp().GetSzarpDataDir();

	wxDirDialog *dd = new wxDirDialog(NULL, _("Choose directory"), path);

	if (dd->ShowModal() != wxID_OK) {
		dd->Destroy();
		return;
	}

	wxString npath = dd->GetPath();

	if (npath != path) {
		wxMessageBox(_("SZARP data dir has been changed. You must restart all SZARP applications for this action to take effect"),
			_("Information"),
			wxICON_EXCLAMATION);

		wxGetApp().SetSzarpDataDir(npath);
	}

	dd->Destroy();
}

void SCCTaskBarItem::OnHelp(wxCommandEvent& WXUNUSED(event))
{
	help->AddBook(wxGetApp().GetSzarpDir() + _T("resources/documentation/new/scc/html/scc.hhp"));
	help->DisplayContents();
}

void SCCTaskBarItem::OnMenu(wxCommandEvent& event)
{
	printf("MENU COMMAND ID=%d, command = '%s'\n", event.GetId(),
		(SC::S2A(menu->GetCommand(event.GetId()))).c_str());
	wxExecute(menu->GetCommand(event.GetId()));
}


void SCCTaskBarItem::OnClose(wxCloseEvent& event)
{
#ifndef MINGW32
	if (event.CanVeto()) {
		event.Veto();
		return;
	}
#endif
	if (tunnel_frame)
		tunnel_frame->Close(true);
	wxExit();
}

#ifndef MINGW32
void SCCTaskBarItem::OnFonts(wxCommandEvent& WXUNUSED(event))
{
	szCommonFontDlg* dlg = new szCommonFontDlg(NULL);
	dlg->ShowModal();
	delete dlg;
}
#endif

void SCCTaskBarItem::OnHideBases(wxCommandEvent& WXUNUSED(event))
{
    if(m_sel_frame == NULL){
        m_sel_frame = new SCCSelectionFrame();
    }
    m_sel_frame->Centre();
    if(m_sel_frame->ShowModal() == wxID_OK) {
	if (menu == NULL) return;
	if (wxmenu != NULL)
	    delete wxmenu;
        wxmenu = menu->CreateWxMenu(m_sel_frame->GetHiddenDatabases());
    }
}
