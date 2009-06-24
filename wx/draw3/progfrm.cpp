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
#include "progfrm.h"
#include "ids.h"

ProgressFrame::ProgressFrame(wxWindow *parent) : 
		wxDialog(parent, 
			wxID_ANY, 
			_T(""), 
			wxDefaultPosition, 
			wxSize(200, 70), 
			wxFRAME_FLOAT_ON_PARENT | wxFRAME_NO_TASKBAR) {
	assert(parent);

	SetBackgroundColour(DRAW3_BG_COLOR);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

	wxStaticText* label = new wxStaticText(this, wxID_ANY,  _("Fetching values"));
	sizer->Add(label, 0, wxALIGN_CENTER | wxALL, 5);

	m_gauge = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition, wxSize(100, 30), wxGA_SMOOTH);
	sizer->Add(m_gauge, 0, wxALIGN_CENTER | wxALL, 5);
	SetSizer(sizer);

	Centre();
	GetParent()->Disable();
}

void ProgressFrame::SetProgress(int val) {
	m_gauge->SetValue(val);
}

ProgressFrame::~ProgressFrame() {
	GetParent()->Enable();
}
