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

#include <sstream>

#include <wx/colordlg.h>
#include <wx/filesys.h>
#include <wx/file.h>
#include <wx/xrc/xmlres.h>

#include <libxml/tree.h>
#include <sys/types.h>

#include "szarp_config.h"
#include "cconv.h"

#include "szframe.h"
#include "szhlpctrl.h"

#include "ids.h"
#include "classes.h"
#include "coobs.h"
#include "drawobs.h"
#include "dbinquirer.h"
#include "codeeditor.h"
#include "cfgmgr.h"
#include "defcfg.h"
#include "drawpick.h"
#include "cfgdlg.h"
#include "paramslist.h"
#include "paredit.h"
#include "incsearch.h"

class PickKeyboardHandler: public wxEvtHandler
{
	public:
		PickKeyboardHandler(DrawPicker* pick, bool eo=false)
		{
			m_pick=pick;
			m_escape_only=eo;
		}
		void OnChar(wxKeyEvent &event)
		{
			wxCommandEvent evt;
			if((event.GetKeyCode() == WXK_RETURN) && !m_escape_only)
				m_pick->OnOK(evt);
			else if(event.GetKeyCode() == WXK_ESCAPE)
				m_pick->OnCancel(evt);
			else
				event.Skip();
		}
	private:
		bool m_escape_only;
		DrawPicker* m_pick;
		DECLARE_EVENT_TABLE()
};


DrawEdit::DrawEdit(wxWindow * parent, wxWindowID id, const wxString title,
			ConfigManager *cfg)
{
	wxXmlResource::Get()->LoadObject(this, parent, _T("draw_edit"), _T("wxDialog"));
	
	m_parent = (DrawPicker *) parent;

	wxColour *colors = cfg->GetDefaultColors();
	m_color_dialog = new wxColourDialog(this);

	for (int i = 0; i < MAX_DRAWS_COUNT; ++i)
		m_color_dialog->GetColourData().SetCustomColour(i, colors[i]);

	// long name input
	m_long_txt = XRCCTRL(*this, "text_ctrl_long", wxTextCtrl);

	// short name input
	m_short_txt = XRCCTRL(*this, "text_ctrl_short", wxTextCtrl);

	// min value input
	m_min_input = XRCCTRL(*this, "text_ctrl_min", wxTextCtrl);
	
	// max value input
	m_max_input = XRCCTRL(*this, "text_ctrl_max", wxTextCtrl);

	// SimleColorPicker calling button
	m_color_button = XRCCTRL(*this, "button_colour", wxButton);

	// full name of draw
	m_title_label = XRCCTRL(*this, "label_parameter", wxStaticText);

	m_summary_checkbox = XRCCTRL(*this, "checkbox_summary_value", wxCheckBox);

	m_unit_input = XRCCTRL(*this, "text_ctrl_unit", wxTextCtrl);
	
	m_cfg_mgr = cfg;

}

void DrawEdit::OnColor(wxCommandEvent & event)
{
	m_color_dialog->GetColourData().SetColour(m_color_button->GetBackgroundColour());

	int ret = m_color_dialog->ShowModal();
	if (ret != wxID_OK)
		return;

	wxColor color = m_color_dialog->GetColourData().GetColour();
	if (color != wxNullColour) {
		m_color_set = true;
		m_color_button->SetBackgroundColour(color);
	}
}

void DrawEdit::Close() {
	if (IsModal()) 
		EndModal(wxID_OK);
	else {
		SetReturnCode(wxID_OK);
		Show(false);
	}
}

bool DrawEdit::IsSummaried() {
	return m_summary_checkbox->IsChecked();
}

void DrawEdit::OnOK(wxCommandEvent & event)
{
	if (m_long_txt->GetValue().IsEmpty()) {
		wxMessageBox(_("You must provide a draw name."), _("Wrong value"),
			     wxOK | wxICON_ERROR, this);
		return;
	}

	if (m_short_txt->GetValue().IsEmpty()) {
		wxMessageBox(_("You must provide a short draw name."), _("Wrong value"),
			     wxOK | wxICON_ERROR, this);
		return;
	}
		
	if (m_color_set == false) {
		wxMessageBox(_("You must set a draw's colour."), _("Wrong value"),
			     wxOK | wxICON_ERROR, this);
		return;
	}

	std::wistringstream miss(m_min_input->GetValue().c_str());
	miss >> m_min;
	if (!miss.eof() || miss.fail()) {
		wxMessageBox(_("Min must be a number."), _("Wrong value"),
			     wxOK | wxICON_ERROR, this);
		return;
	}

	std::wistringstream mass(m_max_input->GetValue().c_str());
	mass >> m_max;
	if (!mass.eof() || mass.fail()) {
		wxMessageBox(_("Max must be a number."), _("Wrong value"),
			     wxOK | wxICON_ERROR, this);
		return;
	}

	if (m_min >= m_max) {
		wxMessageBox(_("Max must greater then min."), _("Wrong value"),
			     wxOK | wxICON_ERROR, this);
		return;
	}

	Close();
}

void DrawEdit::OnCancel(wxCommandEvent & event) {
	if (IsModal()) 
		EndModal(wxID_CANCEL);
	else {
		SetReturnCode(wxID_CANCEL);
		Show(false);
	}
}

int DrawEdit::CreateFromScratch(wxString long_name, wxString unit_name) {
	m_long_txt->SetValue(long_name);
	m_short_txt->SetValue(_T(""));
	m_min_input->SetValue(_T(""));
	m_max_input->SetValue(_T(""));
	m_unit_input->SetValue(unit_name);

	m_color_button->SetBackgroundColour(wxNullColour);
	m_color_set = false;

	m_summary_checkbox->SetValue(false);

	m_edited_draw = NULL;

	return ShowModal();

}

int DrawEdit::Edit(DefinedDrawInfo * draw)
{
	wxString min_str, max_str;
	// setting default values
	m_title_label->SetLabel(draw->GetParamName());

	wxString old_short_name = draw->GetShortName();
	wxString old_long_name = draw->GetName();
	wxColour old_color = draw->GetDrawColor();

	m_long_txt->SetValue(old_long_name);
	m_short_txt->SetValue(old_short_name);
	m_min_input->SetValue(draw->GetValueStr(draw->GetMin(), _T("")));
	m_max_input->SetValue(draw->GetValueStr(draw->GetMax(), _T("")));
	m_color_button->SetBackgroundColour(old_color);
	m_color_set = true;

	m_summary_checkbox->SetValue(draw->GetSpecial() == TDraw::HOURSUM);

	m_unit_input->SetValue(draw->GetUnit());

	m_edited_draw = draw;

	return ShowModal();
}

void DrawEdit::Cancel() {
	if (IsShown()) {
		if (m_color_dialog->IsShown())
			m_color_dialog->EndModal(wxID_CANCEL);
		EndModal(wxID_CANCEL);
	}
}

wxString DrawEdit::GetShortName() {
	return m_short_txt->GetValue();
}

wxString DrawEdit::GetLongName() {
	return m_long_txt->GetValue();
}

wxColour DrawEdit::GetColor() {
	return m_color_button->GetBackgroundColour();
}

double DrawEdit::GetMin() {
	return m_min;
}

wxString DrawEdit::GetUnit() {
	return m_unit_input->GetValue();
}

double DrawEdit::GetMax() {
	return m_max;
}

DrawPicker::DrawPicker(wxWindow* parent, ConfigManager * cfg, DatabaseManager *dbmgr)
		: m_modified(false)
{
	cfg->RegisterConfigObserver(this);

	wxXmlResource::Get()->LoadDialog(this, parent, _T("dialog_picker"));

	SetHelpText(_T("draw3-ext-chartset"));

	m_config_mgr = cfg;
	m_defined_set = NULL;

	m_draw_edit = new DrawEdit(this, -1, _("Edit"), cfg);

	m_database_manager = dbmgr;

	m_context = NULL;
	m_title_input = XRCCTRL(*this,"text_title",wxTextCtrl);

	// List sizer - list of draws 
	m_list = XRCCTRL(*this,"list_ctrl",wxListCtrl);
	
	m_list->InsertColumn(0, _("Parameters"));
	m_list->SetColumnWidth(0, 500);
	m_list->InsertColumn(1, _("Short name"));
	m_list->InsertColumn(2, _("Full name"));
	m_list->SetColumnWidth(2, 200);
	m_list->InsertColumn(3, _("Min value"));
	m_list->InsertColumn(4, _("Max value"));

	// increment search widget
	m_inc_search = NULL;
	
	PickKeyboardHandler* input_key_hand = new PickKeyboardHandler(this);
	PickKeyboardHandler* button_draw_key_hand = new PickKeyboardHandler(this,true);
	PickKeyboardHandler* button_param_key_hand = new PickKeyboardHandler(this,true);
	PickKeyboardHandler* list_key_hand = new PickKeyboardHandler(this);

	m_list->PushEventHandler(list_key_hand);
	m_title_input->PushEventHandler(input_key_hand);

	wxButton* draw_add_button = XRCCTRL(*this, "button_add_draw", wxButton);
	draw_add_button->PushEventHandler(button_draw_key_hand);

	wxButton* param_add_button = XRCCTRL(*this, "button_add_parameter", wxButton);
	param_add_button->PushEventHandler(button_param_key_hand);

	m_up = XRCCTRL(*this, "bitmap_button_up", wxBitmapButton);
	m_down = XRCCTRL(*this, "bitmap_button_down", wxBitmapButton);

	m_paramslist = NULL;

	m_title_input->SetFocus();

	SetIcon(szFrame::default_icon);

	Layout();
	Centre();
}

void DrawPicker::Select(int i)
{
	m_selected = i;

	if (i == 0) 
		m_up->Disable();
	else
		m_up->Enable();

	if (i == m_list->GetItemCount() - 1)
		m_down->Disable();
	else
		m_down->Enable();

	wxListItem item;
	item.SetId(m_selected);
	m_list->GetItem(item);
	wxFont font = GetFont();
	
	int n = m_list->GetItemCount();
	for (int i = 0; i < n; i++)
	{
		item.SetId(i);
		m_list->GetItem(item);
		item.SetFont(font);
		m_list->SetItem(item);
	}
	
	item.SetId(m_selected);
	m_list->GetItem(item);
	
	font.SetWeight(wxFONTWEIGHT_BOLD);

	item.SetFont(font);
	m_list->SetItemState(m_selected, 0, wxLIST_STATE_SELECTED);
	m_list->SetItem(item);
}

void DrawPicker::OnSelected(wxListEvent & event)
{
	Select(event.GetIndex());
}

void DrawPicker::OnContextMenu(wxListEvent & event)
{
	m_selected = event.GetIndex();

	if (m_selected < 0)
		return;

	if (m_context == NULL) {	// no menu yet
		m_context = new wxMenu();
		m_context->Append(drawpickTB_REMOVE, _("Remove"));
		m_context->Append(drawpickTB_EDIT, _("Edit graph"));
		m_context->Append(drawpickTB_PARAM_EDIT, _("Edit param"));
		m_context->Append(drawpickTB_UP, _("Up"));
		m_context->Append(drawpickTB_DOWN, _("Down"));
	}
	
	wxMenuItem* menu_up = m_context->FindItem(drawpickTB_UP);
	wxMenuItem* menu_down = m_context->FindItem(drawpickTB_DOWN);

	if (m_selected == 0)
		menu_up->Enable(false);
	else
		menu_up->Enable(true);

	if (m_selected == m_list->GetItemCount() - 1)
		menu_down->Enable(false);
	else
		menu_down->Enable(true);

	DefinedDrawInfo* ddi = m_defined_set->GetDraw(m_selected);

	DefinedParam *dp = dynamic_cast<DefinedParam*>(ddi->GetParam());
	if (dp)
		m_context->Enable(drawpickTB_PARAM_EDIT, true);
	else
		m_context->Enable(drawpickTB_PARAM_EDIT, false);


	PopupMenu(m_context);	// show menu
}


void DrawPicker::OnUp(wxCommandEvent & event)
{
	Swap(m_selected, m_selected - 1);
}

void DrawPicker::OnDown(wxCommandEvent & event)
{
	Swap(m_selected, m_selected + 1);
}

void DrawPicker::Swap(int a, int b)
{
	m_modified = true;
	assert(a < m_list->GetItemCount());
	assert(b < m_list->GetItemCount());
	if (a < 0 || b < 0)
		return;
	
	// for each column swap elements
	for (int i = 0; i < m_list->GetColumnCount(); i++) {
		wxListItem itemA, itemB;
		
		itemA.SetId(a);
		itemA.m_mask = wxLIST_MASK_TEXT;
		itemA.m_col = i;

		itemB.SetId(b);
		itemB.m_mask = wxLIST_MASK_TEXT;
		itemB.m_col = i;

		m_list->GetItem(itemA);
		m_list->GetItem(itemB);
	
		itemA.SetId(b);
		itemB.SetId(a);

		m_list->SetItem(itemA);
		m_list->SetItem(itemB);

	}
	m_defined_set->Swap(a, b);
	Select(b);
}

void DrawPicker::OnContextEdit(wxCommandEvent & event)
{
	Edit();
}

void DrawPicker::OnEdit(wxListEvent & event)
{
	m_selected = event.GetIndex();
	Edit();
}

void DrawPicker::Edit()
{
	if (m_selected < 0) {
		wxMessageBox(_("Select draw first."), _("No draw selected"),
			     wxOK | wxICON_ERROR, this);
		return;
	}

	DefinedDrawInfo *draw = dynamic_cast<DefinedDrawInfo*>(m_defined_set->GetDraw(m_selected));
	if (m_draw_edit->Edit(draw) != wxID_OK)
		return;

	draw->SetDrawColor(m_draw_edit->GetColor());
	draw->SetDrawName(m_draw_edit->GetLongName());
	draw->SetShortName(m_draw_edit->GetShortName());
	draw->SetUnit(m_draw_edit->GetUnit());

	if (m_draw_edit->IsSummaried())
		draw->SetSpecial(TDraw::HOURSUM);
	else 
		draw->SetSpecial(TDraw::NONE);

	draw->SetMax(m_draw_edit->GetMax());
	draw->SetMin(m_draw_edit->GetMin());
		
	m_modified = true;

	RefreshData();
}

void DrawPicker::OnRemove(wxCommandEvent & event)
{
	if (m_selected >= 0) {
		if (wxMessageBox(_("Do you really want to remove this draw?"), _("Removal"),
				wxYES_NO | wxICON_QUESTION, this) != wxYES) 
			return;

		m_modified = true;
		m_list->DeleteItem(m_selected);
		m_defined_set->Remove(m_selected);

		Select(0);
	} else
		wxMessageBox(_("Select draw first."), _("No draw selected"),
			     wxOK | wxICON_ERROR, this);
}

void DrawPicker::SetDraw(int index, DefinedDrawInfo* draw)
{
	wxString tmp;

	int count = m_list->GetItemCount();
	if (count <= index) 
		m_list->InsertItem(index, wxEmptyString);

	m_list->SetItem(index, 0,
			draw->GetBasePrefix() + 
			_T(": ") + draw->GetParamName());

	m_list->SetItem(index, 1, draw->GetShortName());
	m_list->SetItem(index, 2, draw->GetName());
	m_list->SetItem(index, 3, draw->GetValueStr(draw->GetMin(), _T("")));
	m_list->SetItem(index, 4, draw->GetValueStr(draw->GetMax(), _T("")));

	m_list->SetItemBackgroundColour(index, draw->GetDrawColor());

}

void DrawPicker::OnAddDraw(wxCommandEvent & event)
{
	if (m_inc_search->ShowModal() != wxID_OK)
		return;

	long prev = -1;

	int number = m_list->GetItemCount();
	DrawInfo *draw = m_inc_search->GetDrawInfo(&prev);
	std::vector<DrawInfo*> to_add;
	while (draw != NULL) {	// adding all selected draws
		to_add.push_back(draw);
		if (to_add.size() + number >= MAX_DRAWS_COUNT) {
			wxMessageBox(_("Too many draws."),
				     _("Too many draws."),
				     wxOK | wxICON_ERROR, this);
			return;
		}
		draw = m_inc_search->GetDrawInfo(&prev);
	}
	for (std::vector<DrawInfo*>::iterator i = to_add.begin();
			i != to_add.end();
			i++)
		m_defined_set->Add(*i);
	number += to_add.size();
	m_modified = true;

#ifdef MINGW32
	Raise();
#endif
	RefreshData();
}

void DrawPicker::OnAddParameter(wxCommandEvent& event) {
	
	if (m_paramslist == NULL)
		m_paramslist = new ParamsListDialog(this, m_config_mgr->GetDefinedDrawsSets(), m_database_manager, true);

	m_paramslist->SetCurrentPrefix(m_prefix);
	if (m_paramslist->ShowModal() != wxID_OK) {
		return;
	}

	DefinedParam *dp = m_paramslist->GetSelectedParam();
	if (dp == NULL)
		return;

	int ret = m_draw_edit->CreateFromScratch(dp->GetParamName().AfterLast(L':'), dp->GetUnit());
	if (ret != wxID_OK)
		return;

	DefinedDrawInfo *ddi = new DefinedDrawInfo(m_draw_edit->GetLongName(),
			m_draw_edit->GetShortName(),
			m_draw_edit->GetColor(),
			m_draw_edit->GetMin(),
			m_draw_edit->GetMax(),
			m_draw_edit->IsSummaried() ? TDraw::HOURSUM : TDraw::NONE,
			m_draw_edit->GetUnit(),
			dp,
			m_config_mgr->GetDefinedDrawsSets());

	m_defined_set->Add(ddi);
			
	m_modified = true;

	RefreshData();

}

void DrawPicker::RefreshData(bool update_title)
{
	m_list->DeleteAllItems();

	for (size_t i = 0; i < m_defined_set->GetDraws()->size(); i++)
		SetDraw(int(i), m_defined_set->GetDraw(i));

	if (m_editing_existing && update_title) {
		wxString title = m_defined_set->GetName();
		m_title_input->SetValue(title) ;
		SetTitle(_("Editing parameter set ") + title);
	}

}


void DrawPicker::Clear()
{
	m_list->DeleteAllItems();
	m_title_input->SetValue(wxEmptyString);

	m_up->Disable();
	m_down->Disable();

}

void DrawPicker::OnCancel(wxCommandEvent & event)
{
	delete m_defined_set;
	m_defined_set = NULL;
	EndModal(wxID_CANCEL);
}

void DrawPicker::OnOK(wxCommandEvent & event)
{
	if (Accept()) 
		EndModal(wxID_OK);
}

void DrawPicker::ConfigurationWasReloaded(wxString prefix) {

	if (m_defined_set == NULL)
		return;

	if (m_defined_set->RefersToPrefix(prefix) == false)
		return;

	m_draw_edit->Cancel();
	m_defined_set->SyncWithPrefix(prefix);

	RefreshData(false);
}

wxString DrawPicker::GetUniqueTitle(wxString user_title) {
	DefinedDrawsSets *dds = m_config_mgr->GetDefinedDrawsSets();

	wxString suggested_title;
	int nr = 2;
	while (true) {
		suggested_title = wxString::Format(_T("%s <%d>"), user_title.c_str(), nr++);
		DrawSetsHash& dsh = dds->GetDrawsSets();
		if (dsh.find(suggested_title) == dsh.end())
			break;
	}
	return suggested_title;

}

bool DrawPicker::FindTitle(wxString& user_title) {
	DefinedDrawsSets *dds = m_config_mgr->GetDefinedDrawsSets();

	if (dds->GetRawDrawsSets()[user_title] == NULL) 
		return true;

	wxString suggested_title = GetUniqueTitle(user_title);
	wxString message = wxString::Format(
			_("The set with given name '%s' already exists.\nWould you like to use name '%s' instead?\n"
			"If you don't want to use suggested name, choose No and provide different title."), 
			user_title.Mid(1).c_str(), /* strip '*' from title*/
			suggested_title.Mid(1).c_str() /*strip '*' */ );

	int ret = wxMessageBox(message, _("Set name already exists."), wxYES_NO, this);
	if (ret == wxYES) {
		user_title = suggested_title;
		return true;
	} else 
		return false;

}

bool DrawPicker::Accept()
{
	m_created_set_name = wxEmptyString;

	if (m_defined_set == NULL)
		return false;

	if (m_defined_set->GetDraws()->size() == 0) {
		wxMessageBox(_("You must add at least one draw."),
			     _("No draws"), wxOK | wxICON_ERROR, this);
		return false;

	} 

	DefinedDrawsSets *dds = m_config_mgr->GetDefinedDrawsSets();

	wxString title = m_title_input->GetValue();
	wxString old_title = m_defined_set->GetName();

	if (title == wxEmptyString) {
		wxMessageBox(_("You must set the title."), _("No title"),
			     wxOK | wxICON_ERROR, this);
		return false;
	}

	if (title[0] != wxChar('*'))
		title = _T("*") + title;

	if (old_title != title || !m_editing_existing)
		if (!FindTitle(title))
			return false;

	m_defined_set->SetName(title);

	if (old_title != title)
		m_modified = true;

	if (m_editing_existing) {	// only modyfing
		if (m_modified == false) {
			delete m_defined_set;
			m_defined_set = NULL;
			return true;
		}

		DefinedDrawSet *nds = m_defined_set;

		//to idicate that we have to ignore any events related to draws removal, subustiution cause
		//we are not editing anything
		m_defined_set = NULL;

		dds->SubstituteSet(old_title, nds);
	} else {	// adding new
		DefinedDrawSet *nds = m_defined_set;
		//to idicate that we have to ignore any events related to draws removal and subustiution cause
		//we are not editing anything
		m_defined_set = NULL;
	
		dds->AddSet(nds);

		m_created_set_name = title;
	}

	return true;
}

int DrawPicker::NewSet(wxString prefix) {

	m_prefix = prefix;
	wxString id = m_config_mgr->GetConfigByPrefix(prefix)->GetID();

	if (m_inc_search == NULL)
		m_inc_search = new IncSearch(m_config_mgr, id, this, incsearch_DIALOG, _("Find"), false, false, true, m_database_manager);
	else
		m_inc_search->SetConfigName(id);

	m_editing_existing = false;

	m_modified = false;

	SetTitle(_("Editing parameter set "));

	StartNewSet();

	m_title_input->SetFocus();

	return ShowModal();

}

void DrawPicker::StartNewSet() {
	Clear();
	delete m_defined_set;

	DefinedDrawsSets *c = m_config_mgr->GetDefinedDrawsSets();
	m_defined_set = new DefinedDrawSet(c);
}

int DrawPicker::EditDraw(DefinedDrawInfo *di, wxString prefix) {
	return EditSet(dynamic_cast<DefinedDrawSet*>(di->GetDrawsSets()->GetDrawsSets()[di->GetSetName()]), prefix);
}

wxString DrawPicker::GetNewSetName() {
	return m_created_set_name;
}

int DrawPicker::EditAsNew(DrawSet *set, wxString prefix) {
	m_editing_existing = false;

	m_modified = true;

	m_prefix = prefix;

	Clear();

	DefinedDrawsSets *c = m_config_mgr->GetDefinedDrawsSets();
	m_defined_set = new DefinedDrawSet(c);

	for (DrawInfoArray::iterator i = set->GetDraws()->begin();
			i != set->GetDraws()->end();
			i++) {
		if (DefinedDrawInfo* d = dynamic_cast<DefinedDrawInfo*>(*i))
			m_defined_set->Add(new DefinedDrawInfo(*d));
		else
			m_defined_set->Add(*i);
	}

	wxString cn = m_config_mgr->GetConfigByPrefix(prefix)->GetID();

	if (m_inc_search == NULL)
		m_inc_search = new IncSearch(m_config_mgr, cn, static_cast<wxWindow*>(this), incsearch_DIALOG, _("Find"), false, false, true, m_database_manager);
	else
		m_inc_search->SetConfigName(cn);

	wxString title = set->GetName();
	if (!title.size() || title[0] != L'*')
		title = L"*" + title;
	title = GetUniqueTitle(title);
	m_title_input->SetValue(title) ;
	SetTitle(_("Editing parameter set ") + title);

	RefreshData();

	return ShowModal();

}

int DrawPicker::EditSet(DefinedDrawSet *set, wxString prefix) {
	m_editing_existing = true;

	m_modified = false;

	Clear();

	m_defined_set = set->MakeDeepCopy();

	m_prefix = prefix;
	wxString cn = m_config_mgr->GetConfigByPrefix(prefix)->GetID();

	if (m_inc_search == NULL)
		m_inc_search = new IncSearch(m_config_mgr, cn, static_cast<wxWindow*>(this), incsearch_DIALOG, _("Find"), false, false, true, m_database_manager);
	else
		m_inc_search->SetConfigName(cn);

	RefreshData();

	return ShowModal();

}

void DrawPicker::SetRemoved(wxString prefix, wxString name) {
	if (m_defined_set == NULL)
		return;
	if (prefix != DefinedDrawsSets::DEF_PREFIX || m_defined_set->GetName() != name)
		return;

	m_draw_edit->Cancel();
	StartNewSet();
}

void DrawPicker::SetRenamed(wxString prefix, wxString from, wxString to, DrawSet *set) {
	if (m_defined_set == NULL)
		return;
	if (prefix != DefinedDrawsSets::DEF_PREFIX || m_defined_set->GetName() != from)
		return;

	m_draw_edit->Cancel();

	DefinedDrawSet *ds = dynamic_cast<DefinedDrawSet*>(set);
	assert(ds);

	delete m_defined_set;
	m_defined_set = ds->MakeDeepCopy();

	RefreshData();
}

void DrawPicker::SetModified(wxString prefix, wxString name, DrawSet *set) {
	if (m_defined_set == NULL)
		return;
	if (prefix != DefinedDrawsSets::DEF_PREFIX || m_defined_set->GetName() != name)
		return;

	m_draw_edit->Cancel();

	DefinedDrawSet *ds = dynamic_cast<DefinedDrawSet*>(set);
	assert(ds);

	delete m_defined_set;
	m_defined_set = ds->MakeDeepCopy();

	RefreshData();
}

void DrawPicker::OnEditParam(wxCommandEvent &e) {
	DefinedDrawInfo* ddi = m_defined_set->GetDraw(m_selected);

	DefinedParam *dp = dynamic_cast<DefinedParam*>(ddi->GetParam());

	ParamEdit* pe = new ParamEdit(this, m_config_mgr, m_database_manager);
	pe->Edit(dp);
	delete pe;

	RefreshData();
}

void DrawPicker::OnClose(wxCloseEvent &e) {
	if (e.CanVeto()) {
		e.Veto();
		Show(false);
	} else
		Destroy();
}

void DrawPicker::OnHelpButton(wxCommandEvent &event) {
	SetHelpText(_T("draw3-ext-chartset"));
	wxHelpProvider::Get()->ShowHelp(this);
}

DrawPicker::~DrawPicker() {
	m_config_mgr->DeregisterConfigObserver(this);

	if (m_paramslist)
		m_paramslist->Destroy();
}

BEGIN_EVENT_TABLE(DrawEdit, wxDialog)
    EVT_BUTTON(wxID_OK, DrawEdit::OnOK)
    EVT_BUTTON(wxID_CANCEL, DrawEdit::OnCancel)
    EVT_BUTTON(XRCID("button_colour"), DrawEdit::OnColor)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(DrawPicker, wxDialog)
    EVT_BUTTON(wxID_OK, DrawPicker::OnOK)
    EVT_BUTTON(wxID_CANCEL, DrawPicker::OnCancel)
    EVT_BUTTON(wxID_HELP, DrawPicker::OnHelpButton)
    EVT_BUTTON(XRCID("button_add_draw"), DrawPicker::OnAddDraw)
    EVT_BUTTON(XRCID("button_add_parameter"), DrawPicker::OnAddParameter)
    EVT_MENU(drawpickTB_REMOVE, DrawPicker::OnRemove)
    EVT_MENU(drawpickTB_EDIT, DrawPicker::OnContextEdit)
    EVT_MENU(drawpickTB_UP, DrawPicker::OnUp)
    EVT_MENU(drawpickTB_DOWN, DrawPicker::OnDown)
    EVT_MENU(drawpickTB_PARAM_EDIT, DrawPicker::OnEditParam)
    EVT_LIST_ITEM_ACTIVATED(XRCID("list_ctrl"), DrawPicker::OnEdit)
    EVT_LIST_ITEM_RIGHT_CLICK(XRCID("list_ctrl"), DrawPicker::OnContextMenu)
    EVT_LIST_ITEM_SELECTED(XRCID("list_ctrl"), DrawPicker::OnSelected)
    EVT_BUTTON(XRCID("bitmap_button_up"), DrawPicker::OnUp)
    EVT_BUTTON(XRCID("bitmap_button_down"), DrawPicker::OnDown)
    EVT_CLOSE(DrawPicker::OnClose)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(PickKeyboardHandler, wxEvtHandler)
    EVT_CHAR(PickKeyboardHandler::OnChar)
END_EVENT_TABLE()

