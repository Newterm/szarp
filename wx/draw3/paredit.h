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

#ifndef __PAREDIT_H__
#define __PAREDIT_H__


#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/spinctrl.h>
#include <wx/datectrl.h>

/**
 * Widget for editing param properties.
 */
class ParamEdit : public wxDialog, public DBInquirer {
public:
		/** Widget constructor
		 * @param color_picker color picking widget
		 */
	ParamEdit(wxWindow *parent, ConfigManager* cfg, DatabaseManager *db);

		/** Changes accepted by user - OK button clicked*/
	void OnOK(wxCommandEvent & event);

	void OnCancel(wxCommandEvent & event);

	/**Start edition of a param*/
	int Edit(DefinedParam * param);

	/**Starts new parameter*/
	int StartNewParameter();

	wxString GetBasePrefix();

	wxString GetParamName();

	wxString SetParamName(wxString param_name);

	int GetPrec();

	wxString GetUnit();

	wxColour GetColor();

	wxString GetFormula();

	time_t GetStartTime();

	TParam::FormulaType GetFormulaType();

	/**Cancel edition of of draw*/
	void Cancel();

	void OnButtonBaseConfig(wxCommandEvent& event);

	time_t GetCurrentTime();

	DrawInfo* GetCurrentDrawInfo();

	bool IsSummaried();

	void DatabaseResponse(DatabaseQuery *query);

	void OnFormulaUndo(wxCommandEvent &event);

	void OnFormulaRedo(wxCommandEvent &event);

	void OnFormulaInsertParam(wxCommandEvent &event);

	void OnFormulaInsertUserParam(wxCommandEvent &event);

	void OnFormulaAdd(wxCommandEvent &event);

	void OnHelpButton(wxCommandEvent &event);

	void OnFormulaSubtract(wxCommandEvent &event);

	void OnFormulaMultiply(wxCommandEvent &event);

	void OnFormulaDivide(wxCommandEvent &event);
	
	void OnStartDateEnabled(wxCommandEvent &event);

	void SetCurrentConfig(wxString prefix);

	void AddParams(DefinedDrawSet *os, DefinedDrawSet *ns);

	void OnDegButton(wxCommandEvent &event);

	~ParamEdit();

private:
	void Close();

	void OnIdle(wxIdleEvent &e);

	void ApplyModifications();

	CodeEditor *m_formula_input;

	wxSpinCtrl *m_prec_spin;
	
	wxSpinCtrl *m_spin_ctrl_start_hours;
	
	wxSpinCtrl *m_spin_ctrl_start_minutes;
	
	wxDatePickerCtrl *m_datepicker_ctrl_start_date;
	
	wxCheckBox *m_checkbox_start;

	wxTextCtrl *m_unit_input;

	wxTextCtrl *m_param_name_input;

	wxButton *m_button_base_config;

	wxButton *m_button_formula_redo;

	wxButton *m_button_formula_undo;

	wxButton *m_button_formula_new_param;

	wxChoice *m_formula_type_choice;

	wxStaticText *m_user_param_label;

	DefinedParam *m_edited_param;

	ParamsListDialog *m_params_list;

	bool m_creating_new;

	bool m_error;

	wxString m_error_string;

		/** Button calling SimpleColorPicker
		 * @see SimpleColorPicker
		 */
	wxButton *m_color_button;

	ConfigManager *m_cfg_mgr;

	wxString m_base_prefix;

	IncSearch *m_inc_search;

		/** Event table - button clicked */
	DECLARE_EVENT_TABLE();
};

#endif
