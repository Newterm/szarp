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
 * SZAU - Szarp Automatic Updater
 * SZARP

 * Adam Smyk asmyk@praterm.pl
 *
 * $Id: kontrtaskbaritem.cpp 63 2009-08-06 21:58:06Z asmyk $
 */

#include "kontrtaskbaritem.h"
#include "kontr.h"
#include "kontroler.h"
#include "version.h"
#include <wx/icon.h>
#include <wx/process.h>

KontrolerTaskBarItem::KontrolerTaskBarItem(bool remote_mode, bool operator_mode, wxString server)
{
	m_menu = NULL;
	m_remote_mode = m_remote_mode;
	m_operator_mode = m_operator_mode;
	m_server = m_server;
	m_kontroler = new szKontroler(NULL, m_remote_mode, m_operator_mode, m_server);
	m_kontroler->m_on_screen = false;
	m_kontroler_icon = wxIcon(kontroler3_16_xpm);	
	if(m_kontroler_icon.Ok())
		SetIcon(m_kontroler_icon, _("Szarp Kontroler"));
}


wxMenu* KontrolerTaskBarItem::CreatePopupMenu() {
	wxMenu* menu = new wxMenu();	
	menu->Append(new wxMenuItem(m_menu, ID_OPEN_MENU, _("Open"), wxEmptyString, wxITEM_NORMAL));
	menu->Append(new wxMenuItem(m_menu, ID_ABOUT_MENU, _("About"), wxEmptyString, wxITEM_NORMAL));
	menu->AppendSeparator();
	menu->Append(new wxMenuItem(m_menu, ID_CLOSE_APP_MENU, _("Close application"), wxEmptyString, wxITEM_NORMAL));
	return menu;
}

void KontrolerTaskBarItem::OnCloseMenuEvent(wxCommandEvent &event) {
	if (wxMessageBox(_("You really wan't end?"), _("Kontroler->End"),
		wxOK | wxCANCEL, NULL) == wxOK) {    
	m_kontroler->Destroy();
	delete m_menu;
	delete m_kontroler;
	RemoveIcon();
	exit(0);
  }
}

void KontrolerTaskBarItem::OnAbout(wxCommandEvent &event) {
	wxGetApp().ShowAbout();
}

void KontrolerTaskBarItem::OnOpen(wxCommandEvent &event) {
	if (m_kontroler->m_on_screen) 
		return;
	if (m_kontroler->loaded) {
			m_kontroler->Show(true);
			m_kontroler->Raise();
			m_kontroler->m_on_screen = true;
	}
}

void KontrolerTaskBarItem::OnLeftMouseDown(wxTaskBarIconEvent &event) {
	//wxTaskBarIconEvent e(wxEVT_TASKBAR_RIGHT_DOWN, this);
	//wxPostEvent(this, e);
	m_menu = CreatePopupMenu();
	PopupMenu(m_menu);
	delete m_menu;
}
	
#if defined(__WXMSW__) && defined(__HAS_BALLOON__)
BEGIN_EVENT_TABLE(KontrolerTaskBarItem, BallonTaskBar)
#else
BEGIN_EVENT_TABLE(KontrolerTaskBarItem, wxTaskBarIcon)
#endif
EVT_MENU(ID_CLOSE_APP_MENU, KontrolerTaskBarItem::OnCloseMenuEvent)
EVT_MENU(ID_ABOUT_MENU, KontrolerTaskBarItem::OnAbout)
EVT_MENU(ID_OPEN_MENU, KontrolerTaskBarItem::OnOpen)
EVT_TASKBAR_LEFT_DOWN(KontrolerTaskBarItem::OnLeftMouseDown)
EVT_TASKBAR_RIGHT_DOWN(KontrolerTaskBarItem::OnLeftMouseDown)
END_EVENT_TABLE()

