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

 * S³awomir Chy³ek schylek@praterm.com.pl
 *
 * $Id$
 */

#include "szautaskbaritem.h"
#include "szauapp.h"
#include "cconv.h"
#include "version.h"
#include <wx/icon.h>
#include <wx/process.h>

#include "../../resources/wx/icons/szau_16_wait.xpm"
#include "../../resources/wx/icons/szau_16_new.xpm"
#include "../../resources/wx/icons/szau_16_error.xpm"
#include "../../resources/wx/icons/szau_16_r0.xpm"
#include "../../resources/wx/icons/szau_16_r45.xpm"
#include "../../resources/wx/icons/szau_16_r90.xpm"
#include "../../resources/wx/icons/szau_16_r135.xpm"
#include "../../resources/wx/icons/szau_16_r180.xpm"
#include "../../resources/wx/icons/szau_16_r225.xpm"
#include "../../resources/wx/icons/szau_16_r270.xpm"
#include "../../resources/wx/icons/szau_16_r315.xpm"

SzauTaskBarItem::SzauTaskBarItem(Updater* updater, UpdaterStatus status) :
	m_current_frame_no(0) {
	m_prev_status = status;
	m_menu = NULL;
	m_wait_icon = wxIcon(szau_16_wait_xpm);
	m_new_icon = wxIcon(szau_16_new_xpm);
	m_error_icon = wxIcon(szau_16_error_xpm);

	m_frames.push_back(wxIcon(szau_16_r0_xpm));
	m_frames.push_back(wxIcon(szau_16_r45_xpm));
	m_frames.push_back(wxIcon(szau_16_r90_xpm));
	m_frames.push_back(wxIcon(szau_16_r135_xpm));
	m_frames.push_back(wxIcon(szau_16_r180_xpm));
	m_frames.push_back(wxIcon(szau_16_r225_xpm));
	m_frames.push_back(wxIcon(szau_16_r270_xpm));
	m_frames.push_back(wxIcon(szau_16_r315_xpm));

	SetIcon(m_wait_icon, _("Szarp Automatic Updater"));

	m_timer = new wxTimer(this, ID_TIMER);

	m_updater = updater;

	m_timer->Start(3000);

	m_prev_status = NEWEST_VERSION;
}

wxMenu* SzauTaskBarItem::CreatePopupMenu() {
	m_menu = new wxMenu();

	m_menu->Append(new wxMenuItem(m_menu, ID_INSTALL_MENU, _("Install"), wxEmptyString, wxITEM_NORMAL));
	m_menu->Append(new wxMenuItem(m_menu, ID_ABOUT_MENU, _("About"), wxEmptyString, wxITEM_NORMAL));

	m_menu->AppendSeparator();
	m_menu->Append(new wxMenuItem(m_menu, ID_CLOSE_APP_MENU, _("Close application"), wxEmptyString, wxITEM_NORMAL));

	m_menu->Enable(ID_INSTALL_MENU, m_prev_status == READY_TO_INSTALL);

	return m_menu;
}

void SzauTaskBarItem::OnCloseMenuEvent(wxCommandEvent &event) {
	RemoveIcon();
	wxExit();
}

void SzauTaskBarItem::OnAbout(wxCommandEvent &event) {
	wxGetApp().ShowAbout();
}

void SzauTaskBarItem::OnInstall(wxCommandEvent &event) {
	m_updater->Install();
}

void SzauTaskBarItem::OnLeftMouseDown(wxTaskBarIconEvent &event) {
	wxTaskBarIconEvent e(wxEVT_TASKBAR_RIGHT_DOWN, this);
	wxPostEvent(this, e);
}

void SzauTaskBarItem::OnTimer(wxTimerEvent& WXUNUSED(event)) {

	size_t value;
	UpdaterStatus status = m_updater->getStatus(value);

	if (status == NEWEST_VERSION) {
		SetIcon(m_wait_icon, _("Szarp Automatic Updater\nCurrent Version: ") + wxString(SC::A2S(SZARP_VERSION)));
	} else if (status == DOWNLOADING) {
		wxString tmp;
		tmp.Printf(_("Szarp Automatic Updater\nDownloading: %d%%"), value);
		SetIcon(m_frames[m_current_frame_no], tmp);
		m_current_frame_no = (int)((double) m_frames.size() * ((double) value / 100.0));
	} else if (status == READY_TO_INSTALL) {
		SetIcon(m_new_icon, _("Szarp Automatic Updater\nNew version available"));
	} else if (status == UPDATE_ERROR) {
		SetIcon(m_error_icon, _("Szarp Automatic Updater\nError"));
	}

	if (status == READY_TO_INSTALL && m_prev_status != READY_TO_INSTALL) {
		if (m_menu)
			m_menu->Enable(ID_INSTALL_MENU, true);
#if defined(__WXMSW__) && defined(__HAS_BALLOON__)
		ShowBalloon(_("Szarp Automatic Updater"), _("New version available"));
#endif
	}

	if (status != READY_TO_INSTALL && m_prev_status == READY_TO_INSTALL) {
		if (m_menu)
			m_menu->Enable(ID_INSTALL_MENU, false);
	}

	m_prev_status = status;

}

#if defined(__WXMSW__) && defined(__HAS_BALLOON__)
BEGIN_EVENT_TABLE(SzauTaskBarItem, BallonTaskBar)
#else
BEGIN_EVENT_TABLE(SzauTaskBarItem, wxTaskBarIcon)
#endif
EVT_MENU(ID_CLOSE_APP_MENU, SzauTaskBarItem::OnCloseMenuEvent)
EVT_MENU(ID_ABOUT_MENU, SzauTaskBarItem::OnAbout)
EVT_MENU(ID_INSTALL_MENU, SzauTaskBarItem::OnInstall)
EVT_TASKBAR_LEFT_DOWN(SzauTaskBarItem::OnLeftMouseDown)
EVT_TIMER(ID_TIMER, SzauTaskBarItem::OnTimer)
END_EVENT_TABLE()

