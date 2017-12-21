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
/* $Id$
 *
 * SZARP
 *
 * ecto@praterm.com.pl
 * pawel@praterm.com.pl
 */

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/listctrl.h>

#include "raporteredit.h"
#include "parselect.h"
#include "cconv.h"

int param_filter(TParam *p) {
	FormulaType ft = p->GetFormulaType();

	int ret = 1;

	if (ft == FormulaType::NONE 
			|| ft == FormulaType::RPN
			|| ft == FormulaType::LUA_IPC
			|| (ft == FormulaType::DEFINABLE && p->GetType() == ParamType::COMBINED))
		ret = 0;

	return ret;
}

szRaporterEdit::szRaporterEdit(TSzarpConfig *_ipk,
		wxWindow* parent, wxWindowID id, const wxString& title) :
	wxDialog(parent, id, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) 
{
	m_title = NULL;
	this->ipk = _ipk;
	
	wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
	wxStaticBoxSizer *desc_sizer = new wxStaticBoxSizer(
			new wxStaticBox(this, wxID_ANY, _("Title")),
			wxVERTICAL);
	
	desc_sizer->Add(m_title = new wxTextCtrl(this, ID_T_TITLE, _("User report"),
				wxDefaultPosition, wxDefaultSize, 0,
				wxTextValidator(wxFILTER_ALPHANUMERIC, &g_data.m_report_name)),
			0, wxEXPAND | wxALL, 8);
	
	wxStaticBoxSizer *rcont_sizer = new wxStaticBoxSizer(
			new wxStaticBox(this, wxID_ANY, _("Parameters list")), 
			wxVERTICAL);
	
	rcont_listc = new wxListCtrl(this, ID_LC_RCONT,
			wxDefaultPosition, wxDefaultSize,
			wxLC_REPORT|wxLC_SINGLE_SEL/*|wxLC_EDIT_LABELS*/);
	
	rcont_listc->InsertColumn(0, _("Shortcut"), wxLIST_FORMAT_LEFT, -2);
	rcont_listc->InsertColumn(1, _("Description"), wxLIST_FORMAT_LEFT, -2);
	
	rcont_sizer->Add(rcont_listc, 1, wxEXPAND, 0);
	
	top_sizer->Add(desc_sizer, 0, wxEXPAND | wxALL, 8);
	top_sizer->Add(rcont_sizer, 1, wxEXPAND | wxALL, 8);
	
	wxBoxSizer *but_sizer = new wxBoxSizer(wxHORIZONTAL);
	
	but_sizer->Add(new wxButton(this, ID_B_ADD, _("Add")),
			0, wxLEFT, 8);
	but_sizer->Add(new wxButton(this, ID_B_DEL, _("Delete")),
			0, wxLEFT | wxRIGHT, 8); 
	but_sizer->Add(CreateButtonSizer(wxOK|wxCANCEL/*|wxHELP*/), 0,
			wxLEFT | wxRIGHT | wxALIGN_CENTER, 8);
	
	top_sizer->Add(but_sizer, 0, wxTOP | wxBOTTOM | wxALIGN_CENTER, 16);
	
	this->SetSizer(top_sizer);  
	
	m_okButton = NULL;
	wxWindowList children = GetChildren();
	for (wxWindowList::iterator iter = children.begin(); iter != children.end(); ++iter) {
		if ((*iter)->GetId() == wxID_OK) {
			m_okButton = *iter;
			break;
		}
	}
	assert(m_okButton != NULL);
	m_okButton->Enable(FALSE);

	ps = new szParSelect(
			this->ipk, 	// config
			this, 		// parent widget
			wxID_ANY, 	
			_("Raporter->Editor->Add"), 
			false, 		// "add" style
			false, 		// multiple choice
			false,		// show short names of parameters
			true,		// show descriptions of parameters
			true, 		// "single" style
			param_filter,	// parameter filtering function
			true		// description from param name, not draw name
			);		
	
}

void szRaporterEdit::OnParAdd(wxCommandEvent &ev) 
{
	if ( ps->ShowModal() == wxID_OK ) {
		g_data.m_raplist.CreateEmpty(); /* do nothing if list is not empty */
		g_data.m_raplist.AddNs(_T("rap"), SZ_REPORTS_NS_URI);
		int i = g_data.m_raplist.Append(ps->g_data.m_param);
		g_data.m_raplist.SetExtraProp(i, SZ_REPORTS_NS_URI, _T("description"), ps->g_data.m_desc);
		
		EnableOkButton();
		RefreshList();
	} 
}

void szRaporterEdit::OnParDel(wxCommandEvent &ev) 
{
	long i = -1;
	while ( (i = rcont_listc->GetNextItem(i, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) 
			!= -1 ) {
		g_data.m_raplist.Remove(i);
	}
	EnableOkButton();
	RefreshList();
}

void szRaporterEdit::RefreshList() 
{
	rcont_listc->Freeze();
	rcont_listc->DeleteAllItems();

	for (size_t i = 0; i < g_data.m_raplist.Count(); i++) {
		assert(g_data.m_raplist.GetParam(i));
		wxString scut = g_data.m_raplist.GetParam(i)->GetShortName();
		rcont_listc->InsertItem(i, scut);

		wxString desc = g_data.m_raplist.GetExtraProp(i, SZ_REPORTS_NS_URI, _T("description"));
		rcont_listc->SetItem(i, 1, desc);
	}

	if (g_data.m_raplist.Count()) {
		rcont_listc->SetColumnWidth(0, -2);
		rcont_listc->SetColumnWidth(1, -1);
	}
	
	rcont_listc->Thaw();
}

bool szRaporterEdit::TransferDataToWindow() 
{
	RefreshList();
	return wxDialog::TransferDataToWindow();
}

void szRaporterEdit::OnListColDrag(wxListEvent &ev) 
{
	ev.Veto();
}

void szRaporterEdit::OnTitleChanged(wxCommandEvent &ev)
{
	//under MSW this event is generated also from wxTextCtrl constructor...
	if (m_title)
		EnableOkButton();
}
	
void szRaporterEdit::EnableOkButton()
{
	if ((m_title->IsEmpty() or (g_data.m_raplist.Count() == 0)) and m_okButton->IsEnabled()) {
		m_okButton->Enable(FALSE);
	} else if (!m_title->IsEmpty() and (g_data.m_raplist.Count() > 0) and !m_okButton->IsEnabled()) {
		m_okButton->Enable(TRUE);
	}
}
		
IMPLEMENT_CLASS(szRaporterEdit, wxDialog)
BEGIN_EVENT_TABLE(szRaporterEdit, wxDialog)
	EVT_BUTTON(ID_B_ADD, szRaporterEdit::OnParAdd)
      	EVT_BUTTON(ID_B_DEL, szRaporterEdit::OnParDel)
      	EVT_LIST_COL_BEGIN_DRAG(ID_LC_RCONT, szRaporterEdit::OnListColDrag)
      	EVT_TEXT(ID_T_TITLE, szRaporterEdit::OnTitleChanged)
END_EVENT_TABLE()

