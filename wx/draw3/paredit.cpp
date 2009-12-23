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
 * draw3
 * SZARP

 *
 * $Id$
 *
 * Creating new defined window
 */

#include <wx/colordlg.h>
#include <wx/filesys.h>
#include <wx/file.h>
#include <wx/xrc/xmlres.h>

#include <libxml/tree.h>
#include <sys/types.h>

#include <time.h>

#include "szhlpctrl.h"

#include "cconv.h"
#include "ids.h"
#include "classes.h"
#include "coobs.h"
#include "drawpick.h"
#include "dbinquirer.h"
#include "database.h"
#include "paredit.h"
#include "cfgmgr.h"
#include "paramslist.h"
#include "defcfg.h"
#include "szframe.h"
#include "cfgdlg.h"
#include "dbmgr.h"
#include "incsearch.h"

#include "codeeditor.h"

#ifdef MINGW32
#include <mingw32_missing.h>
#endif

char degree_char[] = { 0xc2, 0xb0, 0x0 };

ParamEdit::ParamEdit(wxWindow *parent, ConfigManager *cfg, DatabaseManager *dbmgr) : DBInquirer(dbmgr)
{

	SetHelpText(_T("draw3-ext-parametersset"));
	
	wxXmlResource::Get()->LoadDialog(this, parent, _T("param_edit"));
	
	m_unit_input = XRCCTRL(*this, "text_ctrl_unit", wxTextCtrl);
	assert(m_unit_input);

	wxPanel *panel_editor = XRCCTRL(*this, "panel_editor", wxPanel);
	wxBoxSizer *box_sizer = new wxBoxSizer(wxHORIZONTAL);

	m_formula_input = new CodeEditor(panel_editor);
	box_sizer->Add(m_formula_input, 1, wxALIGN_CENTER | wxALL| wxEXPAND, 0);
	panel_editor->SetSizer(box_sizer);

	m_prec_spin = XRCCTRL(*this, "spin_ctrl_prec", wxSpinCtrl);
	
	m_spin_ctrl_start_hours = XRCCTRL(*this, "spin_ctrl_start_hours", wxSpinCtrl);
	
	m_spin_ctrl_start_minutes = XRCCTRL(*this, "spin_ctrl_start_minutes", wxSpinCtrl);
	
	m_datepicker_ctrl_start_date = XRCCTRL(*this, "datepicker_ctrl_start_date", wxDatePickerCtrl);
	
	m_checkbox_start = XRCCTRL(*this, "checkbox_start", wxCheckBox);

	m_button_base_config = XRCCTRL(*this, "button_base_config", wxButton);

	m_button_formula_undo = XRCCTRL(*this, "formula_undo_button", wxButton);

	m_button_formula_redo = XRCCTRL(*this, "formula_redo_button", wxButton);

	m_button_formula_new_param = XRCCTRL(*this, "formula_new_param_button", wxButton);

	m_formula_type_choice = XRCCTRL(*this, "FORMULA_TYPE_CHOICE", wxChoice);

	m_param_name_input = XRCCTRL(*this, "parameter_name", wxTextCtrl);
	
	m_datepicker_ctrl_start_date->Enable(false);
	m_datepicker_ctrl_start_date->SetValue(wxDateTime::Now());
	m_spin_ctrl_start_hours->Enable(false);
	m_spin_ctrl_start_minutes->Enable(false);
	m_spin_ctrl_start_hours->SetValue(0);
	m_spin_ctrl_start_minutes->SetValue(0);

	wxButton* degree_button = XRCCTRL(*this, "button_degree", wxButton);
	degree_button->SetLabel(wxString(wxConvUTF8.cMB2WC(degree_char), *wxConvCurrent));

	m_cfg_mgr = cfg;

	m_inc_search = NULL;

	m_error = false;

	m_params_list = NULL;

	SetSize(1000, 800);
	SetIcon(szFrame::default_icon);
}

void ParamEdit::OnIdle(wxIdleEvent &e) {
	if (m_error) {
		m_error = false;

		wxMessageDialog* dlg = new wxMessageDialog(this, m_error_string, _("Error"), wxOK);
		dlg->ShowModal();
		delete dlg;
	}

}

void ParamEdit::OnButtonBaseConfig(wxCommandEvent& event) {
	ConfigDialog *cfg_dlg = new ConfigDialog(this, m_cfg_mgr->GetConfigTitles(), DefinedDrawsSets::DEF_PREFIX);

	cfg_dlg->ShowUserDefinedSets(false);

	int ret = cfg_dlg->ShowModal();
	if (ret != wxID_OK) {
		cfg_dlg->Destroy();
		return;
	}

	m_base_prefix = cfg_dlg->GetSelectedPrefix();
	m_button_base_config->SetLabel(cfg_dlg->GetSelectedTitle());

	cfg_dlg->Destroy();

}

void ParamEdit::Close() {
	if (IsModal()) 
		EndModal(wxID_OK);
	else {
		SetReturnCode(wxID_OK);
		Show(false);
	}
}

void ParamEdit::OnOK(wxCommandEvent &e) {

	if (m_formula_input->GetText().Trim().IsEmpty()) {
		wxMessageBox(_("You must provide a formula."), _("Formula missing."), 
			     wxOK | wxICON_ERROR, this);
		return;
	}

	if (m_param_name_input->GetValue().IsEmpty()) {
		wxMessageBox(_("You must provide a param name."), _("Parameter name missing."),
			     wxOK | wxICON_ERROR, this);
		return;
	}

	if (m_base_prefix.IsEmpty()) {
		wxMessageBox(_("You must set a reference configuration."), _("Reference configuration missing."),
			     wxOK | wxICON_ERROR, this);
		return;
	}

	if (m_creating_new || m_edited_param->GetParamName().AfterLast(L':') != m_param_name_input->GetValue()) {
		DefinedDrawsSets *dds = m_cfg_mgr->GetDefinedDrawsSets();
		wxString pname = dds->GetNameForParam(m_base_prefix, m_param_name_input->GetValue());
		m_param_name_input->SetValue(pname);
	}

	if (m_formula_input->GetModify() == false) {
		if (m_creating_new == false)
			ApplyModifications();
		EndModal(wxID_OK);
	}

	DatabaseQuery *q = new DatabaseQuery;
	q->type = DatabaseQuery::COMPILE_FORMULA;
	q->draw_info = NULL;
	DatabaseQuery::CompileFormula& cf = q->compile_formula;

	cf.error = NULL;
	cf.formula = wcsdup(m_formula_input->GetText().c_str());

	QueryDatabase(q);

	return;
}

void ParamEdit::OnCancel(wxCommandEvent & event) {
	if (IsModal()) 
		EndModal(wxID_CANCEL);
	else {
		SetReturnCode(wxID_CANCEL);
		Show(false);
	}
}

int ParamEdit::Edit(DefinedParam * param)
{
	m_edited_param = param;
	DrawsSets *ds = m_cfg_mgr->GetConfigByPrefix(param->GetBasePrefix());

	m_base_prefix = param->GetBasePrefix();

	m_button_base_config->SetLabel(ds->GetID());
	m_prec_spin->SetValue(param->GetPrec());
 	m_formula_input->SetText(param->GetFormula());
	m_param_name_input->SetValue(param->GetParamName().AfterLast(L':'));
	m_unit_input->SetValue(param->GetUnit());
	m_formula_type_choice->SetSelection(TParam::LUA_VA ? 0 : 1);
	
	
	if (param->GetStartTime() > 0) {
		struct tm *start_date;
		time_t tmp_time = param->GetStartTime();
		start_date = localtime(&tmp_time);
	
		m_spin_ctrl_start_minutes->SetValue(start_date->tm_min);
		m_spin_ctrl_start_hours->SetValue(start_date->tm_hour);
	
		m_datepicker_ctrl_start_date->SetValue(wxDateTime(param->GetStartTime()));
		m_checkbox_start->SetValue(true);
		
		m_datepicker_ctrl_start_date->Enable(true);
		m_spin_ctrl_start_hours->Enable(true);
		m_spin_ctrl_start_minutes->Enable(true);
	}

	m_creating_new = false;

	m_button_base_config->Disable();

	int ret = ShowModal();

	m_button_base_config->Enable();

	return ret;
}

void ParamEdit::Cancel() {
	EndModal(wxID_CANCEL);
}

int ParamEdit::GetPrec() {
	return m_prec_spin->GetValue();
}

wxString ParamEdit::GetUnit() {
	return m_unit_input->GetValue();
}

wxString ParamEdit::GetFormula() {
	return m_formula_input->GetText();
}

TParam::FormulaType ParamEdit::GetFormulaType() {
	return m_formula_type_choice->GetSelection() == 0 ? TParam::LUA_VA : TParam::LUA_AV;
}

DrawInfo* ParamEdit::GetCurrentDrawInfo() {
	return NULL;
}

time_t ParamEdit::GetCurrentTime() {
	return -1;
}

time_t ParamEdit::GetStartTime() {
	if(m_checkbox_start->GetValue()){
		struct tm start_date;
		
		start_date.tm_sec = 0;
		start_date.tm_min = m_spin_ctrl_start_minutes->GetValue();
		start_date.tm_hour = m_spin_ctrl_start_hours->GetValue();
		
		wxDateTime dt = m_datepicker_ctrl_start_date->GetValue();
		
		start_date.tm_mday = dt.GetDay();
		start_date.tm_mon = dt.GetMonth();
		start_date.tm_year = dt.GetYear() - 1900;
		start_date.tm_wday = 0;
		start_date.tm_yday = 0;
		start_date.tm_isdst = -1;
		
		return mktime(&start_date);
	} else {
		return -1;
	}
}

void ParamEdit::ApplyModifications() {
	assert(m_edited_param);

	DefinedParam *p = m_edited_param;

	DefinedDrawsSets *def_sets = m_cfg_mgr->GetDefinedDrawsSets();

	std::map<wxString, bool> us;

	DrawSetsHash& dsh = def_sets->GetRawDrawsSets();
	for (DrawSetsHash::iterator i = dsh.begin();
			i != dsh.end();
			i++) {
		DrawInfoArray* dia = i->second->GetDraws();
		for (size_t j = 0; j < dia->size(); j++) {
			DefinedDrawInfo *dp = dynamic_cast<DefinedDrawInfo*>((*dia)[j]);

			if (dp->GetParam() == p) {
				dp->SetParamName(wxString(_("User:Param:")) + GetParamName());
				us[i->first] = true;
			}
		}
	}

	p->SetPrec(GetPrec());
	p->SetFormula(GetFormula());
	p->SetFormulaType(GetFormulaType());
	p->SetUnit(GetUnit());
	p->SetStartTime(GetStartTime());

	std::vector<DefinedParam*> dpv;
	dpv.push_back(p);
	m_database_manager->RemoveParams(dpv);

	p->SetParamName(_("User:Param:") + GetParamName());
	p->CreateParam();
	m_database_manager->AddParams(dpv);

	for (std::map<wxString, bool>::iterator i = us.begin();
			i != us.end();
			++i) {
				
		DefinedDrawSet *ds = dynamic_cast<DefinedDrawSet*>(dsh[i->first]);
		SetsNrHash& dsa = ds->GetPrefixes();

		for (SetsNrHash::iterator j = dsa.begin();
				j != dsa.end();
				j++)
			m_cfg_mgr->NotifySetModified(j->first, i->first, ds);

		m_cfg_mgr->NotifySetModified(DefinedDrawsSets::DEF_PREFIX, i->first, ds);
	}
	
}

void ParamEdit::DatabaseResponse(DatabaseQuery *q) {
	assert(q->type == DatabaseQuery::COMPILE_FORMULA);

	DatabaseQuery::CompileFormula& cf = q->compile_formula;

	bool ok = cf.ok;
	if (ok == false) {
		m_error = true;
		m_error_string = wxString::Format(_("Error while compiling param %s"), cf.error);
	}

	free(cf.formula);
	free(cf.error);
	delete q;

	if (ok) {
		if (m_creating_new == false)
			ApplyModifications();
		EndModal(wxID_OK);
	}

}

wxString ParamEdit::GetBasePrefix() {
	return m_base_prefix;
}

int ParamEdit::StartNewParameter() {

	m_formula_input->AppendText(_T("v = "));

	m_formula_input->GotoPos(m_formula_input->GetLength());

	m_prec_spin->SetValue(0);

	m_creating_new = true;

	m_edited_param = NULL;

	return ShowModal();

}

void ParamEdit::OnFormulaUndo(wxCommandEvent &event) {
	m_formula_input->Undo();
}

void ParamEdit::OnFormulaRedo(wxCommandEvent &event) {
	m_formula_input->Redo();
}

void ParamEdit::OnFormulaInsertParam(wxCommandEvent &event) {

	DrawsSets* drawsets = m_cfg_mgr->GetConfigByPrefix(m_base_prefix);
	assert(drawsets);
	wxString ct = drawsets->GetID();

	if (m_inc_search == NULL)
		m_inc_search = new IncSearch(m_cfg_mgr, ct, this, -1, _("Find"), false, false);
	else
		m_inc_search->SetConfigName(ct);
	

	if (m_inc_search->ShowModal() != wxID_OK)
		return;

	long prev = -1;
	DrawInfo *draw;
	while ((draw = m_inc_search->GetDrawInfo(&prev)) != NULL) {
		DrawParam *p = draw->GetParam();
		wxString pname = p->GetParamName();
		m_formula_input->AddText(wxString::Format(_T("p(\"%s:%s\", t, pt) "), draw->GetBasePrefix().c_str(), pname.c_str()));
	}

}

void ParamEdit::OnFormulaInsertUserParam(wxCommandEvent &event) {

	if (m_params_list == NULL)
		m_params_list = new ParamsListDialog(this, m_cfg_mgr->GetDefinedDrawsSets(), m_database_manager, true);

	m_params_list->SetCurrentPrefix(m_base_prefix);
	if (m_params_list->ShowModal() != wxID_OK) {
		return;
	}
		
	DefinedParam *p = m_params_list->GetSelectedParam();
	
	wxString pname = p->GetParamName();

// 	wxTextAttr ds = m_formula_input->GetDefaultStyle();
// 	m_formula_input->SetDefaultStyle(wxTextAttr(*wxRED));
	m_formula_input->AddText(wxString::Format(_T("p(\"%s:%s\", t, pt) "), p->GetBasePrefix().c_str(), pname.c_str()));
// 	m_formula_input->SetDefaultStyle(ds);

}

void ParamEdit::SetCurrentConfig(wxString prefix) {
	m_base_prefix = prefix;

	m_button_base_config->SetLabel(m_cfg_mgr->GetConfigByPrefix(prefix)->GetID());

	if (m_inc_search) 
		m_inc_search->SetConfigName(m_cfg_mgr->GetConfigByPrefix(prefix)->GetID());
}

void ParamEdit::OnFormulaAdd(wxCommandEvent &event) {
	m_formula_input->AddText(_T("+ "));
}

void ParamEdit::OnStartDateEnabled(wxCommandEvent &event) {
	bool action = !m_datepicker_ctrl_start_date->IsEnabled();
	m_datepicker_ctrl_start_date->Enable(action);
	m_spin_ctrl_start_minutes->Enable(action);
	m_spin_ctrl_start_hours->Enable(action);
}

void ParamEdit::OnFormulaSubtract(wxCommandEvent &event) {
	m_formula_input->AddText(_T("- "));
}

void ParamEdit::OnFormulaMultiply(wxCommandEvent &event) {
	m_formula_input->AddText(_T("* "));
}

void ParamEdit::OnFormulaDivide(wxCommandEvent &event) {
	m_formula_input->AddText(_T("/ "));
}

wxString ParamEdit::GetParamName() {
	return m_param_name_input->GetValue();
}

void ParamEdit::OnDegButton(wxCommandEvent &event) {
	m_unit_input->WriteText(wxString(wxConvUTF8.cMB2WC(degree_char), *wxConvCurrent));
}

ParamEdit::~ParamEdit() {
	if (m_params_list)
		m_params_list->Destroy();

}

void ParamEdit::OnHelpButton(wxCommandEvent &event) {
	wxHelpProvider::Get()->ShowHelp(this);
}

BEGIN_EVENT_TABLE(ParamEdit, wxDialog)
    EVT_BUTTON(wxID_CANCEL, ParamEdit::OnCancel)
    EVT_BUTTON(wxID_OK, ParamEdit::OnOK)
    EVT_BUTTON(wxID_HELP, ParamEdit::OnHelpButton)
    EVT_BUTTON(XRCID("formula_undo_button"), ParamEdit::OnFormulaUndo)
    EVT_BUTTON(XRCID("formula_redo_button"), ParamEdit::OnFormulaRedo)
    EVT_BUTTON(XRCID("formula_add_button"), ParamEdit::OnFormulaAdd)
    EVT_BUTTON(XRCID("formula_subtract_button"), ParamEdit::OnFormulaSubtract)
    EVT_BUTTON(XRCID("formula_multiply_button"), ParamEdit::OnFormulaMultiply)
    EVT_BUTTON(XRCID("formula_divide_button"), ParamEdit::OnFormulaDivide)
    EVT_BUTTON(XRCID("formula_insert_param_button"), ParamEdit::OnFormulaInsertParam)
    EVT_BUTTON(XRCID("formula_insert_user_param_button"), ParamEdit::OnFormulaInsertUserParam)
    EVT_BUTTON(XRCID("button_base_config"), ParamEdit::OnButtonBaseConfig)
    EVT_BUTTON(XRCID("button_degree"), ParamEdit::OnDegButton)
    EVT_CHECKBOX(XRCID("checkbox_start"), ParamEdit::OnStartDateEnabled)
    EVT_IDLE(ParamEdit::OnIdle)
END_EVENT_TABLE()

