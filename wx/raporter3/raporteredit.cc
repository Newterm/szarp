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

 * ecto@praterm.com.pl
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


szRaporterEdit::szRaporterEdit(TSzarpConfig *_ipk,
		wxWindow* parent, wxWindowID id, const wxString& title) :
	wxDialog(parent, id, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) 
{
	this->ipk = _ipk;
	
	wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
	wxStaticBoxSizer *desc_sizer = new wxStaticBoxSizer(
			new wxStaticBox(this, wxID_ANY, _("Description")), 
			wxVERTICAL);
	
	desc_sizer->Add(new wxTextCtrl(this, wxID_ANY, _("User report"),
				wxDefaultPosition, wxDefaultSize, 0,
				wxTextValidator(wxFILTER_NONE, &g_data.m_report_name)),
			0, wxEXPAND | wxALL, 8);
	
	wxStaticBoxSizer *rcont_sizer = new wxStaticBoxSizer(
			new wxStaticBox(this, wxID_ANY, _("Contents")), 
			wxVERTICAL);
	
	rcont_listc = new wxListCtrl(this, ID_LC_RCONT,
			wxDefaultPosition, wxDefaultSize,
			wxLC_REPORT|wxLC_SINGLE_SEL/*|wxLC_EDIT_LABELS*/);
	
	rcont_listc->InsertColumn(0, _("Shortcut"), wxLIST_FORMAT_LEFT, -2);
	rcont_listc->InsertColumn(1, _("Name"), wxLIST_FORMAT_LEFT, -2);
	rcont_listc->InsertColumn(2, _("Description"), wxLIST_FORMAT_LEFT, -2);
	
	rcont_sizer->Add(rcont_listc, 1, wxEXPAND, 0);
	
	top_sizer->Add(desc_sizer, 0, wxEXPAND | wxALL, 8);
	top_sizer->Add(rcont_sizer, 1, wxEXPAND | wxALL, 8);
	
	wxBoxSizer *but_sizer = new wxBoxSizer(wxHORIZONTAL);
	
	but_sizer->Add(new wxButton(this, ID_B_ADD, _("Add")),
			0, wxALL, 8);
	but_sizer->Add(new wxButton(this, ID_B_DEL, _("Delete")),
			0, wxALL, 8); 
	but_sizer->Add(CreateButtonSizer(wxOK|wxCANCEL/*|wxHELP*/), 0,
			wxALL | wxALIGN_CENTER, 8);
	
	top_sizer->Add(but_sizer, 0, wxALL | wxALIGN_CENTER, 8);
	
	this->SetSizer(top_sizer);  
	
	ps = new szParSelect(this->ipk, this, wxID_ANY, _("Raporter->Editor->Add"), false, false, false,true,true);
	
}

void szRaporterEdit::OnParAdd(wxCommandEvent &ev) 
{
	if ( ps->ShowModal() == wxID_OK ) {
		g_data.m_raplist.CreateEmpty(); /* do nothing if list is not empty */
		g_data.m_raplist.AddNs(_T("rap"), SZ_REPORTS_NS_URI);
		int i = g_data.m_raplist.Append(ps->g_data.m_param);
		g_data.m_raplist.SetExtraProp(i, SZ_REPORTS_NS_URI, _T("description"), ps->g_data.m_desc);
		
		RefreshList();
	} else {
		wxLogMessage(_T("par_add: cancel"));
	}
}

void szRaporterEdit::OnParDel(wxCommandEvent &ev) 
{
	long i = -1;
	while ( (i = rcont_listc->GetNextItem(i, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) 
			!= -1 ) {
		g_data.m_raplist.Remove(i);
	}
	
	RefreshList();
}

void szRaporterEdit::RefreshList() 
{
	rcont_listc->Freeze();
	rcont_listc->DeleteAllItems();
	

	for (size_t i = 0; i < g_data.m_raplist.Count(); i++) {
		wxString scut = g_data.m_raplist.GetParam(i)->GetShortName();
		rcont_listc->InsertItem(i, scut);

		wxString name = g_data.m_raplist.GetName(i).AfterLast(L':');
		rcont_listc->SetItem(i, 1, name);

		wxString desc = g_data.m_raplist.GetExtraProp(i, SZ_REPORTS_NS_URI, _T("description"));
		if (desc != name)
			rcont_listc->SetItem(i, 2, desc);
	}

	if (g_data.m_raplist.Count()) {
		rcont_listc->SetColumnWidth(0, -2);
		rcont_listc->SetColumnWidth(1, -1);
		rcont_listc->SetColumnWidth(2, -1);
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


IMPLEMENT_CLASS(szRaporterEdit, wxDialog)
BEGIN_EVENT_TABLE(szRaporterEdit, wxDialog)
  EVT_BUTTON(ID_B_ADD, szRaporterEdit::OnParAdd)
  EVT_BUTTON(ID_B_DEL, szRaporterEdit::OnParDel)
  EVT_LIST_COL_BEGIN_DRAG(ID_LC_RCONT, szRaporterEdit::OnListColDrag)
END_EVENT_TABLE()
