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

#include "report_edit.h"
#include "../common/parselect.h"
#include "../common/cconv.h"

int param_filter(TParam *p) {
	TParam::FormulaType ft = p->GetFormulaType();

	int ret = 1;

	if (ft == TParam::NONE 
			|| ft == TParam::RPN
			|| ft == TParam::LUA_IPC
			|| (ft == TParam::DEFINABLE && p->GetType() == ParamType::COMBINED))
		ret = 0;

	return ret;
}

szReporterEdit::szReporterEdit(TSzarpConfig * _ipk, ReportInfo& report, wxWindow* parent, wxWindowID id) :
	wxDialog(parent, id, _("Report edit"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER), report(report), report_name(report.name)
{
	m_title = NULL;
	this->ipk = _ipk;
	
	wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
	wxStaticBoxSizer *desc_sizer = new wxStaticBoxSizer(
			new wxStaticBox(this, wxID_ANY, _("Title")),
			wxVERTICAL);
	
	desc_sizer->Add(m_title = new wxTextCtrl(this, ID_T_TITLE, _("User report"),
				wxDefaultPosition, wxDefaultSize, 0,
				wxTextValidator(wxFILTER_ALPHANUMERIC, &report_name)),
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
			_("Reporter->Editor->Add"), 
			false, 		// "add" style
			false, 		// multiple choice
			false,		// show short names of parameters
			true,		// show descriptions of parameters
			true, 		// "single" style
			param_filter,	// parameter filtering function
			true		// description from param name, not draw name
			);		
	
}

void szReporterEdit::OnParAdd(wxCommandEvent &ev) 
{
	if ( ps->ShowModal() == wxID_OK ) {

		ParamInfo pi = {
			ps->g_data.m_param->GetName(),
			SC::W2S(ps->g_data.m_param->GetShortName()),
			ps->g_data.m_param->GetUnit(),
			(unsigned int) ps->g_data.m_param->GetPrec(),
			SC::W2S(ps->g_data.m_desc)
		};

		report.params.push_back(pi);

		EnableOkButton();
		RefreshList();
	} 
}

void szReporterEdit::OnParDel(wxCommandEvent &ev) 
{
	long i = -1;
	while ( (i = rcont_listc->GetNextItem(i, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) 
			!= -1 ) {
		report.params.erase(report.params.begin() + i);
	}

	EnableOkButton();
	RefreshList();
}

void szReporterEdit::RefreshList() 
{
	rcont_listc->Freeze();
	rcont_listc->DeleteAllItems();

	for (size_t i = 0; i < report.params.size(); i++) {
		const wxString& scut = wxString(report.params[i].short_name);
		rcont_listc->InsertItem(i, scut);

		const wxString& desc = wxString(report.params[i].description);
		rcont_listc->SetItem(i, 1, desc);
	}

	if (report.params.size()) {
		rcont_listc->SetColumnWidth(0, -2);
		rcont_listc->SetColumnWidth(1, -1);
	}

	rcont_listc->Thaw();
}

bool szReporterEdit::TransferDataToWindow() 
{
	RefreshList();
	return wxDialog::TransferDataToWindow();
}

void szReporterEdit::OnListColDrag(wxListEvent &ev) 
{
	ev.Veto();
}

void szReporterEdit::OnTitleChanged(wxCommandEvent &ev)
{
	if (m_title) {
		EnableOkButton();
		report.name = SC::W2S(m_title->GetLineText(0));
	}
}
	
void szReporterEdit::EnableOkButton()
{
	if ((m_title->IsEmpty() or (report.params.size() == 0)) and m_okButton->IsEnabled()) {
		m_okButton->Enable(FALSE);
	} else if (!m_title->IsEmpty() and (report.params.size() > 0) and !m_okButton->IsEnabled()) {
		m_okButton->Enable(TRUE);
	}
}
		
IMPLEMENT_CLASS(szReporterEdit, wxDialog)
BEGIN_EVENT_TABLE(szReporterEdit, wxDialog)
	EVT_BUTTON(ID_B_ADD, szReporterEdit::OnParAdd)
      	EVT_BUTTON(ID_B_DEL, szReporterEdit::OnParDel)
      	EVT_LIST_COL_BEGIN_DRAG(ID_LC_RCONT, szReporterEdit::OnListColDrag)
      	EVT_TEXT(ID_T_TITLE, szReporterEdit::OnTitleChanged)
END_EVENT_TABLE()

