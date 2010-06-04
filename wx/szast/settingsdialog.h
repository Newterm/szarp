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

#ifndef __SETTINGSDIALOG_H__
#define __SETTINGSDIALOG_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

class SettingsDialog : public wxDialog {	
	wxComboBox* m_speed_combo;
	wxTextCtrl* m_id_text_ctrl;
	wxTextCtrl* m_path_text_ctrl;
	wxTextCtrl* m_ip_address_text_ctrl;
	wxTextCtrl* m_port_text_ctrl;
	wxNotebook* m_settings_notebook;
public:
	void GetValues(wxString &speed, wxString &path, wxString &ip_address, wxString &port, MessagesGenerator::CONNECTION_TYPE& type, wxString &id);
	void SetValues(wxString speed, wxString path, wxString ip_address, wxString port, MessagesGenerator::CONNECTION_TYPE type, wxString id);
	SettingsDialog(wxWindow *parent);
};

#endif
