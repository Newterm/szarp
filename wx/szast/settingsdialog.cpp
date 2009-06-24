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

#include "settingsdialog.h"

#include <wx/xrc/xmlres.h>

SettingsDialog::SettingsDialog(wxWindow *parent) : wxDialog() {
	wxXmlResource::Get()->LoadDialog(this, parent, _T("SettingsDialog"));

	m_speed_combo = XRCCTRL(*this, "combo_box_port_speed", wxComboBox);
	m_id_text_ctrl = XRCCTRL(*this, "text_ctrl_device_id", wxTextCtrl);
	m_path_text_ctrl = XRCCTRL(*this, "text_ctrl_device_path", wxTextCtrl);

}

void SettingsDialog::GetValues(wxString &speed, wxString &path, wxString &id) {
	speed = m_speed_combo->GetValue();
	path = m_path_text_ctrl->GetValue();
	id = m_id_text_ctrl->GetValue();
}

void SettingsDialog::SetValues(wxString speed, wxString path, wxString id) {
	m_speed_combo->SetValue(speed);
	m_path_text_ctrl->SetValue(path);
	m_id_text_ctrl->SetValue(id);
}
