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
 * confedit - SZARP configuration editor
 * pawel@praterm.com.pl
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "deditdlg.h"

#define array_size(a)	(int)(sizeof a / sizeof *a)

DrawEditDialog::DrawEditDialog(wxWindow* parent, int& col_ind, wxString& min, wxString& max)
	: wxDialog(parent, -1, _("Edit draw")),
	m_col_index(col_ind)
{
	wxBoxSizer* top_sizer = new wxBoxSizer(wxVERTICAL);

	wxBoxSizer* mm_sizer = new wxBoxSizer(wxHORIZONTAL);
	wxTextCtrl* min_ctrl = new wxTextCtrl(this, -1, _T(""), wxDefaultPosition,
			wxDefaultSize, 0, wxTextValidator(wxFILTER_NUMERIC, &min));
	mm_sizer->Add(new wxStaticText(this, -1, _("min.")));
	mm_sizer->Add(min_ctrl, 1, wxEXPAND | wxALL, 2);
	wxTextCtrl* max_ctrl = new wxTextCtrl(this, -1, _T(""), wxDefaultPosition,
			wxDefaultSize, 0, wxTextValidator(wxFILTER_NUMERIC, &max));
	mm_sizer->Add(new wxStaticText(this, -1, _("max.")));
	mm_sizer->Add(max_ctrl, 1, wxEXPAND | wxALL, 2);
	top_sizer->Add(mm_sizer, 0, wxALL, 10);

	wxStaticBoxSizer* c_sizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Color"));

	m_c[0] = new wxToggleButton(this, -1, _("auto"));
	c_sizer->Add(m_c[0], 1, wxEXPAND | wxALL, 10);
	wxGridSizer* g_sizer = new wxGridSizer(4, 3, 10);
	for (int i = 1; i < array_size(m_c); i++) {
		m_c[i] = new wxToggleButton(this, -1, _T(""), wxDefaultPosition,
				wxSize(42, 20));
		m_c[i]->SetBackgroundColour(DrawDefaultColors::MakeColor(i - 1));
		g_sizer->Add(m_c[i]);
	}
	for (int i = 0; i < array_size(m_c); i++) {
		Connect(m_c[i]->GetId(), wxEVT_COMMAND_TOGGLEBUTTON_CLICKED,
				wxCommandEventHandler(DrawEditDialog::OnColorChanged));
	}
	c_sizer->Add(g_sizer, 0, wxLEFT | wxRIGHT, 10);

	top_sizer->Add(c_sizer, 1, wxEXPAND | wxALL, 10);

	wxSizer* b_sizer = CreateButtonSizer(wxOK | wxCANCEL);
	top_sizer->Add(b_sizer, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);

	SetSizer(top_sizer);
	SetAutoLayout(TRUE);
	top_sizer->SetSizeHints(this);

	SetColor();
	min_ctrl->SetFocus();
}

void DrawEditDialog::SetColor()
{
	for (int i = 0; i < array_size(m_c); i++) {
		m_c[i]->SetValue(m_col_index == (i - 1));
	}
}

void DrawEditDialog::OnColorChanged(wxCommandEvent& ev)
{
	int id = ((wxWindow *)ev.GetEventObject())->GetId();
	
	int prev = m_col_index;
	for (int i = 0; i < array_size(m_c); i++) {
		if (id == m_c[i]->GetId()) {
			m_col_index = i - 1;
			break;
		}
	}
	if (prev != m_col_index) {
		m_c[prev + 1]->SetValue(false);
	}
}

#undef array_size

