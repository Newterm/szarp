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
 */

#include <wx/colordlg.h>
#include <wx/filesys.h>
#include <wx/file.h>
#include <wx/xrc/xmlres.h>
#include <regex>

#include <libxml/tree.h>
#include <sys/types.h>

#include <time.h>

#include "szhlpctrl.h"

#include "cconv.h"
#include "ids.h"
#include "classes.h"
#include "coobs.h"
#include "drawobs.h"
#include "sprecivedevnt.h"
#include "parameditctrl.h"
#include "seteditctrl.h"
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
#include "drawtime.h"
#include "drawsctrl.h"
#include "timeformat.h"
#include "drawtime.h"
#include "remarks.h"

#include "codeeditor.h"

#ifdef MINGW32
#include <mingw32_missing.h>
#endif

char degree_char[] = { (char)0xc2, (char)0xb0, (char)0x0 };

class non_zero_search_condition : public szb_search_condition {
public:
	bool operator()(const double& v) const { return std::isnan(v) ? false : v ; }
};

ParamEdit::ParamEdit(wxWindow *parent, ConfigManager *cfg, DatabaseManager *dbmgr, RemarksHandler* remarks_handler) : DBInquirer(dbmgr)
{
	SetHelpText(_T("draw3-ext-parametersset"));

	m_widget_mode = EDITING_PARAM;
	m_search_direction = NOT_SEARCHING;

	m_cfg_mgr = cfg;
	m_draws_ctrl = NULL;
	m_remarks_handler = remarks_handler;

	InitWidget(parent);

	wxSizer* sizer = GetSizer();
	wxButton *forward_button = XRCCTRL(*this, "wxID_FORWARD", wxButton);
	sizer->Show(forward_button->GetContainingSizer(), false, true);
	sizer->Show(m_found_date_label->GetContainingSizer(), false, true);
	sizer->Layout();

}

ParamEdit::ParamEdit(wxWindow *parent, ConfigManager *cfg, DatabaseManager *dbmgr, DrawsController *dc) : DBInquirer(dbmgr)
{
	SetHelpText(_T("draw3-ext-expression-searching"));

	m_cfg_mgr = cfg;
	m_draws_ctrl = dc;
	m_base_prefix = dc->GetCurrentDrawInfo()->GetBasePrefix();

	m_widget_mode = EDITING_SEARCH_EXPRESSION;
	m_search_direction = NOT_SEARCHING;

	InitWidget(parent);

	wxSizer* sizer = GetSizer();

	wxButton *ok_button = XRCCTRL(*this, "wxID_OK", wxButton);
	sizer->Show(ok_button->GetContainingSizer(), false, true);
	sizer->Show(m_param_name_input->GetContainingSizer(), false, true);
	sizer->Show(m_unit_input->GetContainingSizer(), false, true);
	sizer->Show(m_datepicker_ctrl_start_date->GetContainingSizer(), false, true);
	sizer->Show(m_button_base_config->GetContainingSizer(), false, true);
	sizer->Show(m_formula_type_choice->GetContainingSizer(), false, true);
	sizer->Show(m_prec_spin->GetContainingSizer(), false, true);
	sizer->Show(XRCCTRL(*this, "param_name_label", wxStaticText), false, true);

#define HIDE_WINDOW(ID, CLASS) \
	{ \
		wxSizer* sizer = XRCCTRL(*this, ID, CLASS)->GetContainingSizer(); \
		sizer->Show(XRCCTRL(*this, ID, CLASS), false, true); \
		sizer->Layout(); \
	};
	HIDE_WINDOW("static_line_unit", wxStaticLine)
	HIDE_WINDOW("static_line_start", wxStaticLine)
	HIDE_WINDOW("static_line_precision", wxStaticLine)

	XRCCTRL(*this, "parameter_formula_label", wxStaticText)->SetLabel(_("Expression:"));

	m_formula_input->SetSize(900, 200);

	SetSize(900, 200);

	sizer->Layout();

	m_formula_input->AppendText(_T("v = "));

	SetTitle(_("Searching date"));

}

void ParamEdit::InitWidget(wxWindow *parent) {

	wxXmlResource::Get()->LoadDialog(this, parent, _T("param_edit"));

	m_unit_input = XRCCTRL(*this, "text_ctrl_unit", wxTextCtrl);
	assert(m_unit_input);

	wxStaticText *code_editor_stub = XRCCTRL(*this, "code_editor_stub", wxStaticText);
	wxSizer* stub_sizer = code_editor_stub->GetContainingSizer();
	size_t i = 0;
	while (stub_sizer->GetItem(i) && stub_sizer->GetItem(i)->GetWindow() != code_editor_stub)
		i++;
	assert(stub_sizer->GetItem(i));
	wxWindow *stub_parent = code_editor_stub->GetParent();

	m_formula_input = new CodeEditor(stub_parent);
	m_formula_input->SetSize(code_editor_stub->GetSize());

	stub_sizer->Detach(code_editor_stub);
	code_editor_stub->Destroy();
	stub_sizer->Insert(i, m_formula_input, 1, wxALL | wxEXPAND, 4 );
	stub_sizer->Layout();

	m_found_date_label = XRCCTRL(*this, "found_date_label", wxStaticText);

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

	m_user_param_label = XRCCTRL(*this, "user_param_label", wxStaticText);

	m_datepicker_ctrl_start_date->Enable(false);
	m_datepicker_ctrl_start_date->SetValue(wxDateTime::Now());
	m_spin_ctrl_start_hours->Enable(false);
	m_spin_ctrl_start_minutes->Enable(false);
	m_spin_ctrl_start_hours->SetValue(0);
	m_spin_ctrl_start_minutes->SetValue(0);

	wxButton* degree_button = XRCCTRL(*this, "button_degree", wxButton);
	degree_button->SetLabel(wxString(wxConvUTF8.cMB2WC(degree_char), *wxConvCurrent));

	m_inc_search = NULL;

	m_error = false;

	m_params_list = NULL;

	SetIcon(szFrame::default_icon);
	Centre();
}

void ParamEdit::OnIdle(wxIdleEvent &e) {
	wxMessageDialog *dlg = NULL;

	if (m_error) {
		m_error = false;
		dlg = new wxMessageDialog(this, m_error_string, _("Error"), wxOK);
	} else if (m_info_string.size()) {
		wxString s = m_info_string;
		m_info_string = wxString();
		dlg = new wxMessageDialog(this, s, _("Information"), wxOK);
	}

	if (dlg) {
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

void ParamEdit::CreateDefinedParam() {

	DefinedParam *np = new DefinedParam(GetBasePrefix(),
			GetParamNameFirstAndSecondPart() + GetParamName(),
			GetUnit(),
			GetFormula(),
			GetPrec(),
			GetFormulaType(),
			GetStartTime(),
			m_network_param);
	np->CreateParam();

	if (m_network_param) {
		m_remarks_handler->GetConnection()->InsertOrUpdateParam(np, this, false);
		delete np;
	} else {
		if (!m_creating_new) {
			m_cfg_mgr->SubstiuteDefinedParams(std::vector<DefinedParam*>(1, m_edited_param), std::vector<DefinedParam*>(1, np));
			m_edited_param = np;
		} else {
			np->setUserDefined(true);
			m_cfg_mgr->GetDefinedDrawsSets()->AddDefinedParam(np);
			m_database_manager->AddParams(std::vector<DefinedParam*>(1, np));
		}
	}

}

bool ParamEdit::ParamExists(const std::string &param)
{
	return m_cfg_mgr->GetIPKs()->GetParam(SC::L2S(param), true) != NULL;
}

void ParamEdit::ValidateParamNames()
{
	std::vector<std::string> params;
	std::string text = std::string(m_formula_input->GetText().mb_str());
	size_t found = 0;

	//find all param names in formula. Param lies between "p(" and ", t, pt)"
	while( (found = text.find(std::string("p("), found)) != std::string::npos)
	{
		// Find start quote of the param
		unsigned const int pstart = text.find(std::string("\""), found+1);
		if(pstart == std::string::npos)
			throw IllFormedParamException(_("Could not find quote starting param name after function P at position ")
			                              + wxString::Format(wxT("%i"), found));

		// Find end quote
		unsigned const int pend = text.find(std::string("\""), pstart+1);
		if(pend == std::string::npos)
			throw IllFormedParamException(_("Could not find quote ending param name after position ")+wxString::Format(wxT("%i"), pstart));

		// Find end bracket of p function
		found = text.find(')', pend+1);
		if(found == std::string::npos)
			throw IllFormedParamException(_("Could not find closing bracket after position ")+wxString::Format(wxT(" %i"), pend));


		// Count commas for arguments (2 commas for 3 arguments)
		auto pend_it = text.begin();
		std::advance(pend_it, pend);

		auto found_it = text.begin();
		std::advance(found_it, found);

		if (std::count(pend_it, found_it, ',') != size_t{2})
			throw IllFormedParamException(_("Function p is ill-formed, arguments missing between positions ")
			                              + wxString::Format(wxT(" %i "), pend)
			                              + _("and") +wxString::Format(wxT(" %i"), found));

		// Everything was OK
		params.push_back(text.substr(pstart+1, pend - pstart - 1));
	}

	for(unsigned int i = 0; i < params.size(); ++i)
	{
		if(!ParamExists(params[i])) throw IllFormedParamException(_("Could not find param ")+wxString(params[i].c_str(), wxConvUTF8));
	}
}

void ParamEdit::OnOK(wxCommandEvent &e) {


	if (m_edited_param == NULL && m_creating_new == false) {
		EndModal(wxID_OK);
		return;
	}
	
	try {
		ValidateParamNames();
	} catch (const SzException& e) {
		wxMessageBox(_("Failed to add parameter, reason was: ") + wxString::FromUTF8(e.what()), _("Operation failed"),
			wxOK | wxICON_ERROR, this);
		return;
	}

	if (!ValidateComma(m_formula_input->GetText())) {
		int answer = wxMessageBox(_("Comma next to a number found. Do you want to correct it?"), _("Comma found"),
			wxYES_NO | wxICON_WARNING, this);
		if(answer == wxYES)
		{
			return;
		}
	}

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

	std::string found_character;
	if (ContainsForbiddenCharacters(m_param_name_input->GetValue(), found_character)) {
		wxString error_character(found_character);
		wxMessageBox(_("Param name cannot contain forbidden characters. Found ") + error_character,  _("Forbidden characters found"),
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
		CreateDefinedParam();
		if (m_network_param == false)
			EndModal(wxID_OK);
	} else {
		SendCompileFormulaQuery();
	}
}

void ParamEdit::OnCancel(wxCommandEvent & event) {
	int answer = wxMessageBox(_("Are you sure you want to close this window?"), _("Cancel window"),
		wxYES_NO | wxICON_WARNING, this);
	if(answer == wxNO)
	{
		return;
	}

	if (IsModal())
		EndModal(wxID_CANCEL);
	else {
		SetReturnCode(wxID_CANCEL);
		Show(false);
	}
}


bool ParamEdit::ValidateComma(const wxString &s){
	std::wregex re(L",[0-9]", std::regex_constants::icase);
	return !std::regex_search(s.wc_str(), re);
}

bool ParamEdit::ContainsForbiddenCharacters(const wxString &s, std::string &found_character) {
	// If you want to add another forbidden character, put it just in the array below
	std::string forbidden_characters[] = { "\"", "\\" };

	for (auto forbidden_char: forbidden_characters) {
		size_t found_forbidden_character = s.ToStdString().find(forbidden_char);
		if (found_forbidden_character != std::string::npos) {
			found_character = forbidden_char;
			return true;
		}
	}

	return false;
}

void ParamEdit::TransferToWindow(DefinedParam *param) {
	DrawsSets *ds = m_cfg_mgr->GetConfigByPrefix(param->GetBasePrefix());

	m_button_base_config->SetLabel(ds->GetID());
	m_prec_spin->SetValue(param->GetPrec());
 	m_formula_input->SetText(param->GetFormula());
	m_user_param_label->SetLabel(param->GetParamName().BeforeLast(L':') + L':');
	m_param_name_input->SetValue(param->GetParamName().AfterLast(L':'));
	m_unit_input->SetValue(param->GetUnit());
	m_formula_type_choice->SetSelection(param->GetFormulaType() == FormulaType::LUA_VA ? 0 : 1);


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

}

int ParamEdit::Edit(DefinedParam * param)
{
	TransferToWindow(param);

	m_edited_param = param;

	m_base_prefix = param->GetBasePrefix();

	m_creating_new = false;

	m_button_base_config->Disable();

	m_network_param = param->IsNetworkParam();
	if (m_network_param)
		m_param_name_input->Enable(false);

	int ret = ShowModal();

	m_button_base_config->Enable();

	return ret;
}

int ParamEdit::View(DefinedParam * param)
{
	m_edited_param = NULL;
	m_creating_new = false;

	TransferToWindow(param);

	m_button_base_config->Enable(false);
	m_prec_spin->Enable(false);
 	m_formula_input->Enable(false);
	m_user_param_label->Enable(false);
	m_param_name_input->Enable(false);
	m_unit_input->Enable(false);
	m_formula_type_choice->Enable(false);

	m_spin_ctrl_start_minutes->Enable(false);
	m_spin_ctrl_start_hours->Enable(false);

	m_datepicker_ctrl_start_date->Enable(false);
	m_checkbox_start->Enable(false);

	m_datepicker_ctrl_start_date->Enable(false);
	m_spin_ctrl_start_hours->Enable(false);
	m_spin_ctrl_start_minutes->Enable(false);

	m_button_base_config->Disable();

	wxSizer *main_sizer = GetSizer();
	wxSizer *buttons_sizer = m_button_formula_redo->GetContainingSizer();
	main_sizer->Show(buttons_sizer, false, true);

	wxWindow *cancel_button = FindWindow(wxID_CANCEL);
	wxSizer *button_sizer = cancel_button->GetContainingSizer();
	button_sizer->Show(cancel_button, false, true);
	button_sizer->Layout();

	m_formula_input->SetReadOnly(true);

	main_sizer->Layout();

	return ShowModal();

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

FormulaType ParamEdit::GetFormulaType() {
	return m_formula_type_choice->GetSelection() == 0 ? FormulaType::LUA_VA : FormulaType::LUA_AV;
}

DrawInfo* ParamEdit::GetCurrentDrawInfo() {
	return NULL;
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

wxString ParamEdit::GetParamNameFirstAndSecondPart() {
	return m_user_param_label->GetLabel();
}

void ParamEdit::FormulaCompiledForParam(DatabaseQuery *q) {
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
		CreateDefinedParam();
		if (!m_network_param)
			EndModal(wxID_OK);
	}
}

void ParamEdit::FormulaCompiledForExpression(DatabaseQuery *q) {

	DatabaseQuery::CompileFormula& cf = q->compile_formula;
	bool ok = cf.ok;
	if (ok == false) {
		m_error = true;
		m_error_string = wxString::Format(_("Invalid expression %s"), cf.error);
	}
	free(cf.formula);
	free(cf.error);

	delete q;
	if (!ok) {
		return;
	}

	DefinedParam* param = new DefinedParam(m_draws_ctrl->GetCurrentDrawInfo()->GetBasePrefix(),
		L"TEMPORARY:SEARCH:EXPRESSION",
		L"",
		GetFormula(),
		0,
		FormulaType::LUA_AV,
		-1);
	param->CreateParam();

	std::vector<DefinedParam*> dpv = std::vector<DefinedParam*>(1, param);
	m_cfg_mgr->SubstituteOrAddDefinedParams(dpv);

	DefinedDrawInfo* ddi = new DefinedDrawInfo(L"",
			L"",
			wxColour(),
			0,
			1,
			TDraw::NONE,
			L"",
			param,
			m_cfg_mgr->GetDefinedDrawsSets());

	q = new DatabaseQuery();
	q->type = DatabaseQuery::SEARCH_DATA;
	q->draw_info = ddi;
	q->param = param->GetIPKParam();
	q->draw_no = -1;
	ToNanosecondTime(wxInvalidDateTime,
		q->search_data.end_second,
		q->search_data.end_nanosecond
		);
	q->search_data.period_type = m_draws_ctrl->GetPeriod();
	q->search_data.search_condition = new non_zero_search_condition;

	DTime dt = m_current_search_date.IsValid() ? DTime(m_draws_ctrl->GetPeriod(), m_current_search_date.GetTicks()) : m_draws_ctrl->GetCurrentTime();
	dt.AdjustToPeriod();
	TimeIndex time_index(m_draws_ctrl->GetPeriod());
	switch (m_search_direction) {
		case SEARCHING_LEFT:
			ToNanosecondTime((dt - time_index.GetTimeRes() - time_index.GetDateRes()).GetTime(),
				q->search_data.start_second,
				q->search_data.start_nanosecond);
			q->search_data.direction = -1;
			break;
		case SEARCHING_RIGHT:
			ToNanosecondTime((dt - time_index.GetTimeRes() - time_index.GetDateRes()).GetTime(),
				q->search_data.start_second,
				q->search_data.start_nanosecond);
			q->search_data.direction = 1;
			break;
		case NOT_SEARCHING:
			assert(false);
			break;
	}
	QueryDatabase(q);
}

void ParamEdit::SearchResultReceived(DatabaseQuery *q) {
	wxDateTime response = ToWxDateTime(q->search_data.response_second, q->search_data.response_nanosecond);
	if (response.IsValid()) {
		m_current_search_date = response;
		m_draws_ctrl->Set(m_current_search_date);
		m_found_date_label->SetLabel(wxString::Format(_("Found time: %s"), FormatTime(m_current_search_date, m_draws_ctrl->GetPeriod()).c_str()));
	} else {
		m_info_string = wxString::Format(_("%s"), _("No date found"));
	}
	DefinedParam* dp = dynamic_cast<DefinedParam*>(q->draw_info->GetParam());
	std::vector<DefinedParam*> dpv(1, dp);
	m_database_manager->RemoveParams(dpv);
	m_cfg_mgr->GetDefinedDrawsSets()->RemoveParam(dp);
	delete q->search_data.search_condition;
	delete q->draw_info;
	delete q;

	m_search_direction = NOT_SEARCHING;
}

void ParamEdit::DatabaseResponse(DatabaseQuery *q) {

	switch (m_widget_mode) {
		case EDITING_PARAM:
			FormulaCompiledForParam(q);
			break;
		case EDITING_SEARCH_EXPRESSION:
			switch (q->type) {
				case DatabaseQuery::COMPILE_FORMULA:
					FormulaCompiledForExpression(q);
					break;
				case DatabaseQuery::SEARCH_DATA:
					SearchResultReceived(q);
					break;
				default:
					assert(false);
			}
			break;
	}
}

wxString ParamEdit::GetBasePrefix() const {
	return m_base_prefix;
}

int ParamEdit::StartNewParameter(bool network_param) {

	m_formula_input->AppendText(_T("v = "));

	m_formula_input->GotoPos(m_formula_input->GetLength());

	m_prec_spin->SetValue(0);

	m_creating_new = true;

	m_network_param = network_param;

	m_edited_param = NULL;

	if (network_param) {
		wxString user_name = m_remarks_handler->GetUsername();
		m_user_param_label->SetLabel(wxString::Format(_("User:%s:"), user_name.c_str()));
	} else {
		m_user_param_label->SetLabel(_("User:Param:"));
	}

	return ShowModal();

}

void ParamEdit::OnFormulaUndo(wxCommandEvent &event) {
	m_formula_input->Undo();
}

void ParamEdit::OnFormulaRedo(wxCommandEvent &event) {
	m_formula_input->Redo();
}

void ParamEdit::PrepareSearchFormula() {
	DrawInfo* draw_info = m_draws_ctrl->GetCurrentDrawInfo();
	if (draw_info == NULL || draw_info->IsValid() == false)
		return;

	m_formula_input->ClearAll();
	m_formula_input->AddText(_T("v = "));

	DrawParam *p = draw_info->GetParam();
	wxString pname = p->GetParamName();

	m_formula_input->AddText(wxString::Format(_T("p(\"%s:%s\", t, pt) "), draw_info->GetBasePrefix().c_str(), pname.c_str()));
}

void ParamEdit::OnFormulaInsertParam(wxCommandEvent &event) {

	DrawsSets* drawsets = m_cfg_mgr->GetConfigByPrefix(m_base_prefix);
	assert(drawsets);
	wxString ct = drawsets->GetID();

	if (m_inc_search == NULL)
		m_inc_search = new IncSearch(m_cfg_mgr, m_remarks_handler, ct, this, -1, _("Find"), false, false);
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
		m_params_list = new ParamsListDialog(this, m_cfg_mgr->GetDefinedDrawsSets(), m_database_manager, m_remarks_handler, true);

	m_params_list->SetCurrentPrefix(m_base_prefix);
	if (m_params_list->ShowModal() != wxID_OK) {
		return;
	}

	DefinedParam *p = m_params_list->GetSelectedParam();

	wxString pname = p->GetParamName();

	m_formula_input->AddText(wxString::Format(_T("p(\"%s:%s\", t, pt) "), p->GetBasePrefix().c_str(), pname.c_str()));

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

void ParamEdit::ResetStartDate() {
	m_current_search_date = wxInvalidDateTime;
	m_found_date_label->SetLabel(L"");
}

DefinedParam* ParamEdit::GetModifiedParam() {
	assert(m_edited_param);
	return m_edited_param;
}

ParamEdit::~ParamEdit() {
	if (m_params_list)
		m_params_list->Destroy();

}

void ParamEdit::OnHelpButton(wxCommandEvent &event) {
	wxHelpProvider::Get()->ShowHelp(this);
}

void ParamEdit::OnHelpSearchButton(wxCommandEvent &event) {
	wxHelpProvider::Get()->ShowHelp(this);
}

void ParamEdit::SendCompileFormulaQuery() {
	DatabaseQuery *q = new DatabaseQuery;
	q->type = DatabaseQuery::COMPILE_FORMULA;
	q->draw_info = NULL;
	DatabaseQuery::CompileFormula& cf = q->compile_formula;
	cf.error = NULL;
	cf.formula = wcsdup(m_formula_input->GetText().c_str());

	QueryDatabase(q);
}

void ParamEdit::OnForwardButton(wxCommandEvent& event) {
	if (m_search_direction != NOT_SEARCHING)
		return;

	m_search_direction = SEARCHING_RIGHT;
	SendCompileFormulaQuery();
}

void ParamEdit::OnBackwardButton(wxCommandEvent& event) {
	if (m_search_direction != NOT_SEARCHING)
		return;

	m_search_direction = SEARCHING_LEFT;
	SendCompileFormulaQuery();
}

void ParamEdit::OnCloseButton(wxCommandEvent& event) {
	Show(false);
}

void ParamEdit::ParamInsertUpdateError(wxString error) {
	wxMessageBox(_("Error while editing network set: ") + error, _("Editing network set error"),
		wxOK | wxICON_ERROR, this);
}

void ParamEdit::ParamInsertUpdateFinished(bool ok) {
	if (ok) {
		m_remarks_handler->GetConnection()->FetchNewParamsAndSets(NULL);
		EndModal(wxID_OK);
	} else {
		wxMessageBox(_("You don't have sufficient privileges too insert/update this param."), _("Insufficient privileges"),
			wxOK | wxICON_ERROR, this);
	}
}

BEGIN_EVENT_TABLE(ParamEdit, wxDialog)
    EVT_BUTTON(wxID_CANCEL, ParamEdit::OnCancel)
    EVT_BUTTON(wxID_OK, ParamEdit::OnOK)
    EVT_BUTTON(wxID_HELP, ParamEdit::OnHelpButton)
    EVT_BUTTON(XRCID("help_serach_button"), ParamEdit::OnHelpSearchButton)
    EVT_BUTTON(wxID_FORWARD, ParamEdit::OnForwardButton)
    EVT_BUTTON(wxID_BACKWARD, ParamEdit::OnBackwardButton)
    EVT_BUTTON(wxID_CLOSE, ParamEdit::OnCloseButton)
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

