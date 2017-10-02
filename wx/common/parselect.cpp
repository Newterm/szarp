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
 * raporter3 program
 * ecto@praterm.com.pl 
 *
 * $Id$
 */

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <regex>
#include <wx/treectrl.h>
#include <wx/valtext.h>
#include <wx/tokenzr.h>
#include <map>

#include "tokens.h"
#include "wx/config.h"
#include "parselect.h"
#include "fonts.h"

#define szID_PARSELECTADD			wxID_HIGHEST
#define szID_PARSELECTCLOSE			wxID_HIGHEST+1
#define szID_HELPSELECT				wxID_HIGHEST+2
#define szID_TREESEL				wxID_HIGHEST+3
#define ID_ExtractorSearch			wxID_HIGHEST+4
#define ID_ExtractorSearchReset			wxID_HIGHEST+5

DEFINE_EVENT_TYPE(wxEVT_SZ_PARADD)

szParSelectEvent::szParSelectEvent()
	: wxCommandEvent(wxEVT_SZ_PARADD, -1)
{
}

szParSelectEvent::szParSelectEvent(ArrayOfTParam params, int id,
				   wxEventType eventType)
	: wxCommandEvent(eventType, id)
{
	this->params = params;
}

szParSelectEvent::szParSelectEvent(const szParSelectEvent & event)
	: wxCommandEvent(event)
{
	params = event.params;
}

wxEvent *szParSelectEvent::Clone() const 
{
	return new szParSelectEvent(*this);
}

void szParSelectEvent::SetParams(ArrayOfTParam & params)
{
	this->params = params;
}

const ArrayOfTParam *szParSelectEvent::GetParams()
{
	return &params;
}

class szParTreeElem:public wxTreeItemData {
 private:
	TParam * m_param;
 public:
	szParTreeElem(TParam * _param)
		: m_param(_param) 
	{ }
	TParam *getParam() 
	{
		return m_param;
	}
};

szParSelect::szParSelect(TSzarpConfig * _ipk,
			 wxWindow * parent,
			 wxWindowID id,
			 const wxString & title,
			 bool addStyle,
			 bool multiple,
			 bool showShort,
			 bool showDescr,
			 bool _single,
			 szParFilter * filter, 
			 bool description_is_param_name)
	: wxDialog(parent, id, title)
{
	this->ipk = _ipk;
	szSetDefFont(this);

	single = _single;

	if (addStyle == FALSE)
		multiple = FALSE;
	if (multiple == TRUE) {
		showShort = FALSE;
		showDescr = FALSE;
	}

	m_is_multiple = multiple;
	m_filter = filter;
	this->description_is_param_name = description_is_param_name;

	wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
	par_sizer =
	    new wxStaticBoxSizer(new wxStaticBox(this, -1, _("Parameter")),
				 wxVERTICAL);

	par_trct = new wxTreeCtrl(this, ID_TRC_PARS,
				  wxDefaultPosition, wxSize(400, 300),
				  wxTR_HAS_BUTTONS | (multiple ? wxTR_MULTIPLE |
						      wxTR_EXTENDED : 0));
	szSetDefFont(par_trct);
	wxString CheckBoxName(_("Show like in draw program"));
	check_box =
	    new wxCheckBox(this, szID_TREESEL, CheckBoxName, wxDefaultPosition);
	wxString last_pos = wxConfig::Get()->Read(_T("TreeSel"), _T("true"));
	last_param = _T("NULL");

	if (last_pos.IsSameAs(_T("false"))) {
		check_box->SetValue(false);
		this->LoadParams();
	} else {
		check_box->SetValue(true);
		this->LoadParamsLikeInDraw();
	}
	input_text = new wxTextCtrl(this, ID_ExtractorSearch,  _T(""), wxDefaultPosition,
			wxSize(300, 10));
	reset_button = new wxButton(this, ID_ExtractorSearchReset, _("Reset"));
	search_sizer = new wxBoxSizer(wxHORIZONTAL);
	search_sizer->Add(input_text, 0, wxALL | wxGROW, 5);
	search_sizer->Add(reset_button, 0, wxALL | wxGROW, 5);

	par_sizer->Add(par_trct, 1, wxGROW | wxALL, 8);
	top_sizer->Add(search_sizer, 0, wxALIGN_CENTER, 0);
	top_sizer->Add(par_sizer, 1, wxGROW | wxALL, 8);

	if (showShort) {
		wxStaticBoxSizer *scut_sizer =
		    new wxStaticBoxSizer(new
					 wxStaticBox(this, -1, _("Shortcut")),
					 wxVERTICAL);

		scut_txtc = new wxTextCtrl(this, ID_TC_PAR_SCUT, wxEmptyString,
					   wxDefaultPosition, wxDefaultSize, 0,
					   wxTextValidator(wxFILTER_NONE,
							   &g_data.m_scut));
		scut_sizer->Add(scut_txtc, 0, wxGROW | wxALL, 8);
		top_sizer->Add(scut_sizer, 0, wxGROW | wxALL, 8);
	} else {
		scut_txtc = NULL;
	}

	if (showDescr) {
		wxStaticBoxSizer *desc_sizer =
		    new wxStaticBoxSizer(new
					 wxStaticBox(this, -1,
						     _("Description")),
					 wxVERTICAL);

		desc_txtc = new wxTextCtrl(this, ID_TC_PAR_DESC, wxEmptyString,
					   wxDefaultPosition, wxDefaultSize, 0,
					   wxTextValidator(wxFILTER_NONE,
							   &g_data.m_desc));
		desc_sizer->Add(desc_txtc, 0, wxGROW | wxALL, 8);
		top_sizer->Add(desc_sizer, 0, wxGROW | wxALL, 8);
	} else {
		desc_txtc = NULL;
	}

	if (addStyle && single == FALSE) {
		button_sizer = new wxBoxSizer(wxHORIZONTAL);
		wxButton *add_button =
		    new wxButton(this, szID_PARSELECTADD, _("Add"),
				 wxDefaultPosition);
		wxButton *close_button =
		    new wxButton(this, szID_PARSELECTCLOSE, _("Close"),
				 wxDefaultPosition);

		button_sizer->Add(add_button, 0, wxALL | wxGROW, 8);
		button_sizer->Add(close_button, 0, wxALL | wxGROW, 8);

		top_sizer->Add(check_box, 0, wxLEFT, 8);
		top_sizer->Add(button_sizer, 0, wxALIGN_CENTER, 0);
		add_button->SetDefault();
	} else {
		top_sizer->Add(check_box, 0, wxLEFT, 8);
		top_sizer->
		    Add(CreateButtonSizer(wxOK | wxCANCEL), 0,
			wxALL | wxALIGN_CENTER, 8);
	}

	main_help = NULL;
	this->SetSizer(top_sizer);
	top_sizer->SetSizeHints(this);
}

void szParSelect::SetHelp(const wxString & page)
{
	szHelpControllerHelpProvider *tmp_provider;

	tmp_provider = (szHelpControllerHelpProvider *) 
		szHelpControllerHelpProvider::Get();
	main_help = (szHelpController *) tmp_provider->GetHelpController();
	help_button = new wxButton(this, szID_HELPSELECT, _("Help"),
				   wxDefaultPosition);
	help_button->SetHelpText(page);

	button_sizer->Add(help_button, 1, wxALL | wxGROW, 8);
}

void szParSelect::LoadParamsLikeInDraw()
{
	assert(ipk != NULL);
	par_trct->DeleteAllItems();
	wxTreeItemId root_id = par_trct->AddRoot(wxString(ipk->GetTitle()));
	wxTreeItemId looked;
	bool found_last = false;

	std::map < std::wstring, wxTreeItemId > draws;
	std::map < std::wstring, wxTreeItemId >::iterator draws_iter;

	for (TParam * p = ipk->GetFirstParam(); p; p = p->GetNextGlobal()) {
		if (m_filter && m_filter(p))
			continue;
		for (TDraw * d = p->GetDraws(); d; d = d->GetNext()) {

			wxTreeItemId draw_id;
			draws_iter = draws.find(d->GetTranslatedWindow());

			if (draws_iter == draws.end()) {
				draw_id =
				    par_trct->AppendItem(root_id,
							 wxString(d->
								  GetTranslatedWindow
								  ()));
				draws[d->GetTranslatedWindow()] = draw_id;
			} else {
				draw_id = draws_iter->second;
			}

			wxString name = p->GetTranslatedDrawName();
			if (name.IsEmpty()) {
				name =
				    wxString(p->GetTranslatedName()).
				    AfterLast(':');
			}

			wxTreeItemId tmp = par_trct->AppendItem(draw_id, name,
								-1, -1,
								new
								szParTreeElem
								(p));

			if (last_param.IsSameAs(wxString(p->GetName()))) {
				looked = tmp;
				found_last = true;
			}
		}
	}

	if (!last_param.IsSameAs(_T("NULL")) && found_last) {
		ExpandToLastParam(looked);

	} else {
		par_trct->Expand(root_id);
		par_trct->UnselectAll();
	}

}

void szParSelect::LoadParams()
{

	assert(ipk != NULL);
	par_trct->DeleteAllItems();

	// ipk => treectrl
	TParam *param_it = ipk->GetFirstParam();

	wxTreeItemId root_id = par_trct->AddRoot(wxString(ipk->GetTitle()));
	wxTreeItemId looked;
	do {
		if (m_filter && m_filter(param_it)) {
			continue;
		}

		wxStringTokenizer tkz(wxString(param_it->GetTranslatedName()),
				      _T(":"));

		wxTreeItemId node_id = root_id;

#if wxCHECK_VERSION(2,5,0)
		wxTreeItemIdValue cookie;
#else
		long cookie;
#endif
		wxTreeItemId tmp;
		wxString token;

		while (tkz.CountTokens() > 1) {
			token = tkz.GetNextToken();
			// look for node
			bool found = false;
			for (wxTreeItemId i_id =
			     par_trct->GetFirstChild(node_id, cookie);
			     (i_id > 0);
			     i_id = par_trct->GetNextChild(node_id, cookie)) {
				if (par_trct->GetItemText(i_id).IsSameAs(token)) {
					node_id = i_id;
					found = true;
					break;
				}
			}
			// if not found - add new one
			if (!found){
				node_id = par_trct->AppendItem(node_id, token);
			}
		}
		token = tkz.GetNextToken();

		// add data on leaf
		tmp = par_trct->AppendItem(node_id, token, -1, -1,
					   new szParTreeElem(param_it));
		if (last_param.IsSameAs(wxString(param_it->GetName()))) {
			looked = tmp;
		}

	} while ((param_it = param_it->GetNextGlobal()) != NULL);

	SortTree(root_id);
	if (!last_param.IsSameAs(_T("NULL"))) {
		ExpandToLastParam(looked);
	} else {
		par_trct->Expand(root_id);
		par_trct->UnselectAll();
	}
}

void szParSelect::SortTree(wxTreeItemId id)
{
#if wxCHECK_VERSION(2,5,0)
	wxTreeItemIdValue cookie;
#else
	long cookie;
#endif
	wxTreeItemId i;

	if (!par_trct->ItemHasChildren(id))
		return;
	par_trct->SortChildren(id);
	for (i = par_trct->GetFirstChild(id, cookie); i > 0;
	     i = par_trct->GetNextChild(id, cookie)) {
		if (par_trct->ItemHasChildren(i)){
			SortTree(i);
		}
	}
}

void szParSelect::OnParSelect(wxTreeEvent & ev)
{
	szParTreeElem *tpar =
	    (szParTreeElem *) par_trct->GetItemData(ev.GetItem());
	if (tpar != NULL) {
		// FIXME: should be done by validator (TransferFromWindow):
		g_data.m_param = tpar->getParam();
		if (scut_txtc) {
			scut_txtc->
			    SetValue(wxString(g_data.m_param->GetShortName()));
		}
		if (desc_txtc) {
			if (description_is_param_name) {
				desc_txtc->
				    SetValue(wxString
					     (g_data.m_param->GetName()).
					     AfterLast(L':'));
			} else {
				desc_txtc->
				    SetValue(wxString
					     (g_data.m_param->GetDrawName()));
			}
		}
	}
}

void szParSelect::OnParChanging(wxTreeEvent & ev)
{
	if (par_trct->ItemHasChildren(ev.GetItem())) {
		ev.Veto();
	}
}

void szParSelect::OnHelpClicked(wxCommandEvent & ev)
{
	if (main_help != NULL) {
		main_help->DisplaySection(help_button->GetHelpText());
	}
}

void szParSelect::ExpandToLastParam(wxTreeItemId looked)
{
	par_trct->SelectItem(looked);
}

void szParSelect::OnCheckClicked(wxCommandEvent & ev)
{
	if (!par_trct->GetSelection().IsOk()
	    || !par_trct->GetItemData(par_trct->GetSelection())) {
		g_data.m_param = NULL;
		last_param = _T("NULL");
	} else {
		last_param = ((szParTreeElem *) par_trct->
			      GetItemData(par_trct->
					  GetSelection()))->getParam()->
		    GetName();
	}

	if (check_box->GetValue() == TRUE) {
		LoadParamsLikeInDraw();
	} else {
		LoadParams();
	}
	par_sizer->Add(par_trct, 1, wxGROW | wxALL, 8);

}

void szParSelect::OnAddClicked(wxCommandEvent & ev)
{
	szParSelectEvent newevent;
	ArrayOfTParam params;

	if (m_is_multiple && single == FALSE) {
		wxArrayTreeItemIds ids;
		int count = par_trct->GetSelections(ids);
		if (count <= 0) {
			return;
		}
		for (int i = 0; i < count; i++) {
			szParTreeElem *pte =
			    (szParTreeElem *) par_trct->GetItemData(ids.
								    Item(i));
			if (pte) {
				params.Add(pte->getParam());
			} else {
				par_trct->Expand(ids.Item(i));
			}
		}
	} else {
		wxTreeItemId id = par_trct->GetSelection();
		if (id < 0) {
			return;
		}
		szParTreeElem *pte =
		    (szParTreeElem *) par_trct->GetItemData(id);
		if (pte){
			params.Add(pte->getParam());
		}
	}

	if (params.GetCount() == 0){
		return;
	}

	par_trct->UnselectAll();
	newevent.SetId(GetId());
	newevent.SetEventObject(this);
	newevent.SetParams(params);
	wxPostEvent(GetParent(), newevent);
}

void szParSelect::OnCloseClicked(wxCommandEvent & ev)
{

	if (check_box->GetValue()) {
		wxConfig::Get()->Write(_T("TreeSel"), _T("true"));
	} else {
		wxConfig::Get()->Write(_T("TreeSel"), _T("false"));
	}
	Show(false);
}

void szParSelect::OnReset(wxCommandEvent & ev)
{
	input_text->Clear();
}

void szParSelect::OnSearch(wxCommandEvent & ev)
{
	if (check_box->GetValue() == TRUE) {
		SearchLikeInDraw();
	} else {
		Search();
	}
}

void szParSelect::Search()
{

	wxString match = input_text->GetValue();

	if(match != wxEmptyString){
		try {
			std::wregex re(match.wc_str(), std::regex_constants::icase);

			assert(ipk != NULL);
			par_trct->DeleteAllItems();
			// ipk => treectrl

			wxTreeItemId root_id = par_trct->AddRoot(wxString(ipk->GetTitle()));
			wxTreeItemId looked;
			for (TParam * param_it = ipk->GetFirstParam(); param_it; param_it = param_it->GetNextGlobal()) {
				const std::wstring& name = param_it->GetName();
				if (std::regex_search(name, re)) {
					if (m_filter && m_filter(param_it)) {
						continue;
					}

					wxStringTokenizer tkz(wxString(param_it->GetTranslatedName()),
						_T(":"));

					wxTreeItemId node_id = root_id;

					wxTreeItemIdValue cookie;
					wxTreeItemId tmp;
					wxString token;

					while (tkz.CountTokens() > 1) {
						token = tkz.GetNextToken();
						// look for node
						bool found = false;
						for (wxTreeItemId i_id =
							 par_trct->GetFirstChild(node_id, cookie);
							 (i_id > 0);
							 i_id = par_trct->GetNextChild(node_id, cookie)) {
							if (par_trct->GetItemText(i_id).IsSameAs(token)) {
								node_id = i_id;
								found = true;
								break;
							}
						}
						// if not found - add new one
						if (!found){
							node_id = par_trct->AppendItem(node_id, token);
						}
					}
					token = tkz.GetNextToken();

					// add data on leaf
					tmp = par_trct->AppendItem(node_id, token, -1, -1,
							new szParTreeElem(param_it));
					if (last_param.IsSameAs(wxString(param_it->GetName()))) {
						looked = tmp;
					}
				}

			}

			SortTree(root_id);
			if (!last_param.IsSameAs(_T("NULL"))) {
				ExpandToLastParam(looked);
			} else {
				par_trct->Expand(root_id);
				par_trct->UnselectAll();
			}
		} catch(const std::regex_error& e) {}
	}
}

void szParSelect::SearchLikeInDraw()
{
	wxString match = input_text->GetValue();
	if(match != wxEmptyString){
		try {
			const std::wregex re(match.wc_str(), std::regex_constants::icase);
			assert(ipk != NULL);
			par_trct->DeleteAllItems();
			wxTreeItemId root_id = par_trct->AddRoot(wxString(ipk->GetTitle()));
			wxTreeItemId looked;
			bool found_last = false;

			std::map < std::wstring, wxTreeItemId > draws;
			std::map < std::wstring, wxTreeItemId >::iterator draws_iter;

			for (TParam * p = ipk->GetFirstParam(); p; p = p->GetNextGlobal()) {
				for (TDraw * d = p->GetDraws(); d; d = d->GetNext()) {
					const auto& parameterTranslatedWindowName = d->GetTranslatedWindow();
					const auto& parameterName = p->GetName();
					const auto& parameterTranslatedName = p->GetTranslatedName();
					const auto& parameterTranslatedDrawName = p->GetTranslatedDrawName();
					if(std::regex_search(parameterTranslatedWindowName, re) || std::regex_search(parameterName, re) || std::regex_search(parameterTranslatedName, re) || std::regex_search(parameterTranslatedDrawName, re)) {
						if (m_filter && m_filter(p)){
							continue;
						}

						wxTreeItemId draw_id;
						draws_iter = draws.find(d->GetTranslatedWindow());

						if (draws_iter == draws.end()) {
							draw_id =
								par_trct->AppendItem(root_id,
										 wxString(d->
											GetTranslatedWindow
											 ()));
							draws[d->GetTranslatedWindow()] = draw_id;
						} else {
							draw_id = draws_iter->second;
						}

						wxString name = p->GetTranslatedDrawName();
						if (name.IsEmpty()) {
							name =
								wxString(p->GetTranslatedName()).
								AfterLast(':');
						}

						wxTreeItemId tmp = par_trct->AppendItem(draw_id, name,
											-1, -1,
											new
											szParTreeElem
											(p));

						if (last_param.IsSameAs(wxString(p->GetName()))) {
							looked = tmp;
							found_last = true;
						}
					}
				}
			}

			if (!last_param.IsSameAs(_T("NULL")) && found_last) {
				ExpandToLastParam(looked);

			} else {
				par_trct->Expand(root_id);
				par_trct->UnselectAll();
			}
		} catch(const std::regex_error& e) {}
	}
}
IMPLEMENT_DYNAMIC_CLASS(szParSelect, wxDialog)
BEGIN_EVENT_TABLE(szParSelect, wxDialog)
	EVT_TREE_SEL_CHANGED(ID_TRC_PARS, szParSelect::OnParSelect)
	EVT_TREE_SEL_CHANGING(ID_TRC_PARS, szParSelect::OnParChanging)
	EVT_BUTTON(szID_PARSELECTADD, szParSelect::OnAddClicked)
	EVT_BUTTON(szID_PARSELECTCLOSE, szParSelect::OnCloseClicked)
	EVT_BUTTON(szID_HELPSELECT, szParSelect::OnHelpClicked)
	EVT_CHECKBOX(szID_TREESEL, szParSelect::OnCheckClicked)
	EVT_BUTTON(ID_ExtractorSearchReset, szParSelect::OnReset)
	EVT_TEXT(ID_ExtractorSearch, szParSelect::OnSearch)
END_EVENT_TABLE()

