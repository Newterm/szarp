#include "paramslist.h"
#include "defcfg.h"
#include "paredit.h"

#include <wx/xrc/xmlres.h>

class ParamsDialogKeyboardHandler: public wxEvtHandler
{
public:
	ParamsDialogKeyboardHandler(ParamsListDialog *pld) {
		m_pld = pld;
	}

	void OnChar(wxKeyEvent &event) {
		if (event.GetKeyCode() == WXK_UP)
			m_pld->SelectPreviousParam();
		else if (event.GetKeyCode() == WXK_DOWN)
			m_pld->SelectNextParam();
		else if (event.GetKeyCode() == WXK_RETURN)
			m_pld->SelectCurrentParam();
		else
			event.Skip();
	}
private:
	ParamsListDialog* m_pld;
	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(ParamsDialogKeyboardHandler, wxEvtHandler)
	EVT_CHAR(ParamsDialogKeyboardHandler::OnChar)
END_EVENT_TABLE()

ParamsListDialog::ParamsListDialog(wxWindow *parent, DefinedDrawsSets *dds, DatabaseManager *dbmgr, bool search_mode) {

	SetHelpText(_T("draw3-ext-parametersset"));

	wxXmlResource::Get()->LoadObject(this, parent, _T("PARAM_LIST"), _T("wxDialog"));

	m_search_text = XRCCTRL(*this, "PARAM_SEARCH", wxTextCtrl);

	m_popup_menu = ((wxMenuBar*) wxXmlResource::Get()->LoadObject(this,_T("CONTEXT_MENU"),_T("wxMenuBar")))->GetMenu(0);

	m_param_list = XRCCTRL(*this, "PARAM_LIST_CTRL", wxListCtrl);
	m_def_sets = dds;

	m_dbmgr = dbmgr;

	ParamsDialogKeyboardHandler* eh;
	eh = new ParamsDialogKeyboardHandler(this);
	m_search_text->PushEventHandler(eh);

	eh = new ParamsDialogKeyboardHandler(this);
	m_param_list->PushEventHandler(eh);
	
	m_selected_index = -1;

	wxSizer *main_sizer = GetSizer();

	if (search_mode) {

		wxButton *button = XRCCTRL(*this, "wxID_CLOSE", wxButton);
		main_sizer->Show(button, false, true);
	
		wxButton *ok_button = XRCCTRL(*this, "wxID_OK", wxButton);
		main_sizer->Show(ok_button->GetContainingSizer(), true, true);

	} else {

		wxButton *button = XRCCTRL(*this, "wxID_CLOSE", wxButton);
		main_sizer->Show(button, true, true);

		wxButton *ok_button = XRCCTRL(*this, "wxID_OK", wxButton);
		main_sizer->Show(ok_button->GetContainingSizer(), false, true);
	}

	Layout();
	main_sizer->Fit(this);

	m_search_mode = search_mode;

	int w,h;
	m_param_list->GetSize(&w, &h);
	m_param_list->InsertColumn(0, _("Parameters"), wxLIST_FORMAT_LEFT, w - 10);

	m_search_text->SetFocus();

	LoadParams();
}

void ParamsListDialog::SelectPreviousParam() {
	if (m_selected_index < 1)
		return;

	m_param_list->SetItemState(m_selected_index, 0, wxLIST_STATE_SELECTED);

	long down = m_selected_index - 1;

	m_param_list->SetItemState(down, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);

	m_param_list->EnsureVisible(down);

}

void ParamsListDialog::SelectNextParam() {
	if (m_selected_index < 0 
			|| (m_selected_index + 1) >= m_param_list->GetItemCount())
		return;

	m_param_list->SetItemState(m_selected_index, 0, wxLIST_STATE_SELECTED);

	long up = m_selected_index + 1;

	m_param_list->SetItemState(up, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);

	m_param_list->EnsureVisible(up);

}

void ParamsListDialog::SetCurrentPrefix(wxString prefix) {
	m_prefix = prefix;
}

void ParamsListDialog::LoadParams() {
	const std::vector<DefinedParam*>& dps = m_def_sets->GetDefinedParams();

	m_param_list->DeleteAllItems();

	wxString filter = m_search_text->GetValue().Lower();

	long j = 0;
	for (std::vector<DefinedParam*>::const_iterator i = dps.begin();
			i != dps.end();
			i++, j++) {
		DefinedParam* dp = *i;
		wxString text = dp->GetBasePrefix() + _T(":") + dp->GetParamName();

		if (!filter.IsEmpty() && text.Lower().Find(filter) == wxNOT_FOUND)
			continue;

		wxListItem i;
		i.SetText(text);
		i.SetData(dp);
		i.SetId(j);

		i.SetColumn(0);
		m_param_list->InsertItem(i);
	}

	if (m_param_list->GetItemCount() > 0) {
		m_param_list->SetItemState(0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
	}


}

void ParamsListDialog::OnSerachTextChanged(wxCommandEvent &e) {
	LoadParams();
}

void ParamsListDialog::OnListItemClick(wxListEvent& e) {
	m_selected_index = e.GetIndex();

	PopupMenu(m_popup_menu);
}

void ParamsListDialog::OnListItemSelected(wxListEvent& e) {
	m_selected_index = e.GetIndex();
}

void ParamsListDialog::OnAdd(wxCommandEvent &e) {
	ParamEdit *pe = new ParamEdit(this, m_def_sets->GetParentManager(), m_dbmgr);

	assert(m_prefix.IsEmpty() == false);

	pe->SetCurrentConfig(m_prefix);

	if (pe->StartNewParameter() == wxID_OK) {
		DefinedParam* dp = new DefinedParam(pe->GetBasePrefix(),
							wxString(_("User:Param:")) +  pe->GetParamName(),
							pe->GetUnit(),
							pe->GetFormula(),
							pe->GetPrec(),
							pe->GetFormulaType(),
							pe->GetStartTime());
		m_def_sets->AddDefinedParam(dp);

		dp->CreateParam();
		std::vector<DefinedParam*> dbv;
		dbv.push_back(dp);
		m_dbmgr->AddParams(dbv);
	}

	delete pe;

	LoadParams();
}

void ParamsListDialog::OnRemove(wxCommandEvent &e) {
	DefinedParam *p = GetSelectedParam();
	if (p == NULL) {
		wxMessageBox(_("Choose parameter first"), _("Error!"));
		return;
	}

	int ret = wxMessageBox(_("Are you sure you want to delete this param? All draws referring to this param will be removed as well."), _("Warning!"), wxOK | wxCANCEL);
	if (ret != wxOK)
		return;

	ConfigManager* cfg_mgr = m_def_sets->GetParentManager();
	DrawSetsHash& dsh = m_def_sets->GetRawDrawsSets();
	
	std::vector<wxString> rs;

	for (DrawSetsHash::iterator i = dsh.begin();
			i != dsh.end();
			i++) {

		DefinedDrawSet* ds = dynamic_cast<DefinedDrawSet*>(i->second);
		assert(ds);

		DrawInfoArray* dia = ds->GetDraws();

		std::vector<size_t> tr;

		SetsNrHash prefixes = ds->GetPrefixes();

		for (size_t j = 0; j < dia->size(); j++) {
			DefinedDrawInfo *dp = dynamic_cast<DefinedDrawInfo*>((*dia)[j]);
			assert(dp);

			if (dp->GetParam() == p)
				tr.push_back(j);

		}

		if (tr.size() == dia->size())
			rs.push_back(i->first);
		else for (size_t j = 0; j < tr.size(); j++) {
			ds->Remove(tr[j] - j);

			for (SetsNrHash::iterator k = prefixes.begin();
					k != prefixes.end();
					k++)
				cfg_mgr->NotifySetModified(k->first, i->first, ds);
			cfg_mgr->NotifySetModified(DefinedDrawsSets::DEF_PREFIX, i->first, ds);
		}

	}

	for (size_t i = 0; i < rs.size(); ++i)
		m_def_sets->RemoveSet(rs[i]);

	std::vector<DefinedParam*> dbv;
	dbv.push_back(p);
	m_dbmgr->RemoveParams(dbv);

	m_def_sets->DestroyParam(p);

	LoadParams();	

}

void ParamsListDialog::OnEdit(wxCommandEvent &e) {
	DefinedParam *p = GetSelectedParam();

	if (p == NULL) {
		wxMessageBox( _("Select parameter first!"), _("Error!"), wxID_OK);
		return;
	}

	if (m_def_sets->GetParentManager()->GetConfigByPrefix(p->GetBasePrefix()) == NULL) {
		wxMessageBox(wxString::Format(_("Configuration that this param refers to - '%s' is not present, parameter is not accesible."), 
				p->GetBasePrefix().c_str()), _("Error"), wxICON_ERROR);
		return;
	}

	ParamEdit *pe = new ParamEdit(this, m_def_sets->GetParentManager(), m_dbmgr);
	pe->SetCurrentConfig(m_prefix);
	int ret = pe->Edit(p);
	delete pe;

	if (ret != wxID_OK)
		return;
	LoadParams();	

}

DefinedParam* ParamsListDialog::GetSelectedParam() {
	if (m_selected_index < 0)
		return NULL;

	return (DefinedParam*) m_param_list->GetItemData(m_selected_index);
	
}

void ParamsListDialog::OnClose(wxCloseEvent &e) {
	if (e.CanVeto()) {
		e.Veto();
		EndModal(wxID_CANCEL);		
	} else
		Destroy();
}

void ParamsListDialog::OnOKButton(wxCommandEvent &e) {
	if (m_selected_index < 0) {
		wxMessageBox(_("Choose parameter first"), _("Error!"));
		return;
	}

	DefinedParam *param = (DefinedParam*) m_param_list->GetItemData(m_selected_index);
	if (m_def_sets->GetParentManager()->GetConfigByPrefix(param->GetBasePrefix()) == NULL) {
		wxMessageBox(wxString::Format(_("Configuration that this param refers to - '%s' is not present, parameter is not accesible."), 
				param->GetBasePrefix().c_str()), _("Error"), wxICON_ERROR);
		return;
	}

	EndModal(wxID_OK);
}

void ParamsListDialog::OnCancelButton(wxCommandEvent &e) {
	EndModal(wxID_CANCEL);
}

void ParamsListDialog::SelectCurrentParam() {
	if (m_search_mode)
		if (m_selected_index >= 0) {
				DefinedParam *param = (DefinedParam*) m_param_list->GetItemData(m_selected_index);
				if (m_def_sets->GetParentManager()->GetConfigByPrefix(param->GetBasePrefix()) == NULL) {
					wxMessageBox(wxString::Format(_("Configuration that this param refers to - '%s' is not present, parameter is not accesible."), 
						param->GetBasePrefix().c_str()), _("Error"), wxICON_ERROR);
					return;
				}
			EndModal(wxID_OK);
		} else
			EndModal(wxID_CANCEL);
	else {
		wxCommandEvent e;
		OnEdit(e);
	}
}

void ParamsListDialog::OnListItemActivated(wxListEvent &e) {
	m_selected_index = e.GetIndex();
	SelectCurrentParam();
}

void ParamsListDialog::OnHelpButton(wxCommandEvent &event) {
	wxHelpProvider::Get()->ShowHelp(this);
}

BEGIN_EVENT_TABLE(ParamsListDialog, wxDialog)
	EVT_TEXT(XRCID("PARAM_SEARCH"), ParamsListDialog::OnSerachTextChanged)
	EVT_MENU(XRCID("ADD_PARAMETER"), ParamsListDialog::OnAdd)
	EVT_MENU(XRCID("REMOVE_PARAMETER"), ParamsListDialog::OnRemove)
	EVT_MENU(XRCID("EDIT_PARAMETER"), ParamsListDialog::OnEdit)
	EVT_BUTTON(XRCID("NEW_BUTTON"), ParamsListDialog::OnAdd)
	EVT_BUTTON(XRCID("DELETE_BUTTON"), ParamsListDialog::OnRemove)
	EVT_BUTTON(XRCID("EDIT_BUTTON"), ParamsListDialog::OnEdit)
	EVT_BUTTON(XRCID("OK_BUTTON"), ParamsListDialog::OnOKButton)
	EVT_BUTTON(wxID_OK, ParamsListDialog::OnOKButton)
	EVT_BUTTON(wxID_HELP, ParamsListDialog::OnHelpButton)
	EVT_BUTTON(wxID_CANCEL, ParamsListDialog::OnCancelButton)
	EVT_BUTTON(wxID_CLOSE, ParamsListDialog::OnCancelButton)
	EVT_CLOSE(ParamsListDialog::OnClose)
	EVT_LIST_ITEM_SELECTED(XRCID("PARAM_LIST_CTRL"), ParamsListDialog::OnListItemSelected)
	EVT_LIST_ITEM_ACTIVATED(XRCID("PARAM_LIST_CTRL"), ParamsListDialog::OnListItemActivated)
	EVT_LIST_ITEM_RIGHT_CLICK(XRCID("PARAM_LIST_CTRL"), ParamsListDialog::OnListItemClick)
	EVT_LIST_ITEM_ACTIVATED(XRCID("PARAM_LIST_CTRL"), ParamsListDialog::OnListItemClick)
END_EVENT_TABLE()
