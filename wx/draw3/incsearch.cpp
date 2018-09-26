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
 * incsearch - Incremental Search - engine & widget 
 * SZARP

 * Michal Blajerski <nameless@praterm.com.pl>
 *
 * $Id$
 */

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <regex>

#include <algorithm>

#include <cconv.h>

#include "szhlpctrl.h"

#include "ids.h"
#include "classes.h"
#include "drawobs.h"
#include "seteditctrl.h"
#include "parameditctrl.h"
#include "seteditctrl.h"
#include "sprecivedevnt.h"
#include "cfgmgr.h"
#include "coobs.h"
#include "cfgnames.h"
#include "defcfg.h"
#include "drawpick.h"
#include "incsearch.h"
#include "remarks.h"

class IncKeyboardHandler: public wxEvtHandler
{
public:
	IncKeyboardHandler(IncSearch* inc, bool eo = false) {
		m_inc = inc;
		m_escape_only = eo;
	}

	void OnChar(wxKeyEvent &event) {
		if ((event.GetKeyCode() == WXK_RETURN) && !m_escape_only)
			m_inc->OnEnter();
		else if (event.GetKeyCode() == WXK_ESCAPE)
			m_inc->Show(false);
		else if (event.GetKeyCode() == WXK_UP)
			m_inc->SelectPreviousDraw();
		else if (event.GetKeyCode() == WXK_DOWN)
			m_inc->SelectNextDraw();
		else if (event.GetKeyCode() == WXK_PAGEDOWN)
			m_inc->GoPageDown();
		else if (event.GetKeyCode() == WXK_PAGEUP)
			m_inc->GoPageUp();
		else
			event.Skip();
	}
private:
	bool m_escape_only;
	IncSearch* m_inc;
	DECLARE_EVENT_TABLE()

};


IncSearch::ListCtrl::ListCtrl(ItemsArray * array, wxWindow * parent, int id,
			      const wxPoint & position, const wxSize & size,
			      long style) : wxListView(parent, id, position, size, style), m_cur_items(array)
{
}

wxString IncSearch::ListCtrl::OnGetItemText(long item, long column) const
{
	if (item > (long)m_cur_items->GetCount())
		return wxEmptyString;

	return (*m_cur_items)[item]->GetName();
}

IncSearch::IncSearch(ConfigManager * _cfg, RemarksHandler* remarks_handler, const wxString & _confname,
		     wxWindow * parent, wxWindowID id, const wxString & title,
		     bool window_search, bool show_defined, bool conf_pick, DatabaseManager *_db_mgr)
:  wxDialog(parent, id, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	window_created = false;

	m_start_set = NULL;

	m_start_draw_info = NULL;

	m_window_search = window_search;

	selected_index = wxNOT_FOUND;

	db_mgr = _db_mgr;
	this->cfg = _cfg;
	DrawsSets *c = cfg->GetConfig(_confname);
	this->confid = c->GetPrefix();
	m_cur_items = NULL;

	int size;
	if (m_window_search) {
		size = 400;
		SetHelpText(_T("draw3-base-charts"));
	} else {
		size = 300;
		//TODO: there should be help for graphs search
		SetHelpText(_T("draw3-base-find-parameter"));
	}

	assert(cfg != NULL);

	top_sizer = new wxBoxSizer(wxVERTICAL);
	input_sizer = new wxBoxSizer(wxHORIZONTAL);
	button_sizer = new wxBoxSizer(wxHORIZONTAL);
	//wxStaticBoxSizer *inc_sizer = new wxStaticBoxSizer(wxHORIZONTAL, this);

	input_text =
	    new wxTextCtrl(this, incsearch_TEXT, _T(""), wxDefaultPosition,
			   wxSize(size, 10));

	reset_button = new wxButton(this, wxRESET, _("Reset"));
	ok_button = new wxButton(this, wxOK, _("Ok"),
					   wxDefaultPosition);
	close_button = new wxButton(this, wxCANCEL, _("Close"),
					      wxDefaultPosition);
	help_button = new wxButton(this, wxHELP, _("Help"),
					      wxDefaultPosition);

	button_sizer->Add(ok_button, 0, wxALL | wxGROW, 8);
	button_sizer->Add(close_button, 0, wxALL | wxGROW, 8);
	button_sizer->Add(help_button, 0, wxALL | wxGROW, 8);

	//inc_sizer->Add(item_list, 1, wxEXPAND | wxALL, 1);
	input_sizer->Add(input_text, 0, wxALL | wxGROW, 1);
	input_sizer->Add(reset_button, 0, wxALL | wxGROW, 1);

	db_picker = NULL;
	if (conf_pick) {
		db_picker = new wxChoice(this, incsearch_CHOICE,
					   wxDefaultPosition,
					   wxDefaultSize, 0, NULL,
					   wxCB_SORT);
		db_picker->Append(GetConfNames());
		db_picker->SetStringSelection(_confname);

		top_sizer->Add(db_picker, 0, wxALL | wxALIGN_CENTER, 8);
	}

	m_show_defined = show_defined;

	top_sizer->Add(input_sizer, 0, wxALL | wxALIGN_CENTER , 8);

//we must delay creation of ListCtrl under wxGTK
#ifdef __WXMSW__
	item_list = new ListCtrl(&items_array, this, incsearch_LIST, wxDefaultPosition,
				wxDefaultSize,
				wxLC_VIRTUAL | wxLC_REPORT | wxLC_NO_HEADER);

	top_sizer->Add(item_list, 1, wxEXPAND | wxALL | wxALIGN_CENTER, 8);
#endif

	top_sizer->Add(button_sizer, 0, wxALL | wxALIGN_CENTER, 8);

	SetSizer(top_sizer);
	top_sizer->SetSizeHints(this);
	input_text->SetFocus();
	ok_button->SetDefault();

	input_text->PushEventHandler(new IncKeyboardHandler(this));
	reset_button->PushEventHandler(new IncKeyboardHandler(this, true));
	ok_button->PushEventHandler(new IncKeyboardHandler(this, true));
	close_button->PushEventHandler(new IncKeyboardHandler(this, true));

	m_remarks_handler = remarks_handler;

	cfg->RegisterConfigObserver(this);

#ifdef __WXMSW__
	FinishWindowCreation();
	SetSize(900, 600);
#else
	SetSize(800, 600);
#endif

}

void IncSearch::SetColumnWidth()
{
	wxClientDC dc(item_list);

	int mw = 0;

	for (size_t i = 0; i < items_array.GetCount(); ++i) {
		int w, h;
		dc.GetTextExtent(items_array[i]->GetName(), &w, &h);
		mw = std::max(w, mw);
	}

	item_list->SetColumnWidth(0, mw + 10);
}

void IncSearch::OnSearch(wxCommandEvent & event) {
	Search();
}

void IncSearch::Search() {
	wxString match = input_text->GetValue();

	if (m_cur_items != NULL && m_cur_items != &items_array)
		delete m_cur_items;

	if (match == wxEmptyString) {
		m_cur_items = &items_array;
		UpdateItemList();
		return;
	}

	m_cur_items = new ItemsArray();

#ifndef __MINGW32__
	try {

		std::wstring match_wstring = match.wc_str();
		if( match_wstring.front() == wchar_t('*') ) {
			match_wstring.insert(0, 1, '\\');
		}
		std::wregex re = std::wregex(match_wstring, std::regex_constants::icase);

		for (size_t i = 0; i < items_array.GetCount(); ++i) {
			const std::wstring& name = items_array[i]->GetStdWName();
			if (std::regex_search(name, re)) {
				m_cur_items->Add(items_array[i]);
			}
		}

	} catch(const std::regex_error& e) {
		// ignore (m_cur_items is empty so we show nothing)
	}
#else
	const std::wstring& castedMatch = match.ToStdWstring();
	for (size_t i = 0; i < items_array.GetCount(); ++i) {
		const std::wstring& name = items_array[i]->GetStdWName();
		if (FindStringIC(name, castedMatch)) {
			m_cur_items->Add(items_array[i]);
		}
	}
#endif

	UpdateItemList();
};

bool IncSearch::FindStringIC(const std::wstring& paramName, const std::wstring& pattern)
{
	auto it = std::search(
			paramName.begin(), paramName.end(),
			pattern.begin(), pattern.end(),
			[](char ch1, char ch2) { return std::tolower(ch1) == std::tolower(ch2);}
	);

	return (it != paramName.end());
}

void IncSearch::OnOk(wxCommandEvent & eventt)
{
	OnEnter();
}

void IncSearch::OnActivated(wxListEvent & eventt)
{
	OnEnter();
}

void IncSearch::OnEnter(void)
{

	selected_index = item_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);

	if (IsModal()) 
		EndModal(selected_index != wxNOT_FOUND ? wxID_OK : wxID_CANCEL);
	else 
		Close();

}

void IncSearch::UpdateItemList()
{
	item_list->Freeze();
	item_list->DeleteAllItems();
	item_list->SetItems(m_cur_items);
	item_list->SetItemCount(m_cur_items->GetCount());
	item_list->Thaw();
	if (m_cur_items->GetCount())
		item_list->Select(0, true);
}

wxString IncSearch::GetConfPrefix(wxString name)
{
	const ConfigNameHash& hash = cfg->GetConfigTitles();
	for (ConfigNameHash::const_iterator i = hash.begin(); i != hash.end(); ++i)
		if (i->second == name)
			return i->first;

	assert(false);
}

DrawInfo *IncSearch::GetDrawInfo(long *prev_draw, DrawSet **set) const
{
	*prev_draw =
	    item_list->GetNextItem(*prev_draw, wxLIST_NEXT_ALL,
				   wxLIST_STATE_SELECTED);
	if (*prev_draw != wxNOT_FOUND) {
		Item *i = (*m_cur_items)[*prev_draw];

		if (set)
			*set = i->draw_set;

		return i->draw_info;
	} else 
		return NULL;
}

wxArrayString IncSearch::GetConfNames()
{
	wxArrayString confNames;
	const ConfigNameHash& hash = cfg->GetConfigTitles();
	for (ConfigNameHash::const_iterator i = hash.begin(); i != hash.end(); ++i) {
		if (i->first != DefinedDrawsSets::DEF_PREFIX)
			confNames.Add(i->second);
	}
	return confNames;
}

void IncSearch::OnCancel(wxCommandEvent & evt)
{
	Close();
}

void IncSearch::OnChoice(wxCommandEvent & evt)
{
	confid = GetConfPrefix(evt.GetString());

	LoadParams();
	Search();
	SetColumnWidth();
}

void IncSearch::OnReset(wxCommandEvent & evt)
{
	if (input_text->GetValue().IsEmpty())
		return;

	input_text->Clear();

	OnSearch(evt);

	UpdateItemList();
}

void IncSearch::AddWindowItems(SortedSetsArray *sorted) {
	int count = sorted->size();

	for (int i = 0; i < count; i++) {
		Item *item = new Item();
		item->draw_set = sorted->Item(i);
		item->draw_info = NULL;
		items_array.Add(item);
	}

}

void IncSearch::AddDrawsItems(SortedSetsArray *sorted) {
	int count = sorted->size();
	for (int i = 0; i < count; i++) { 
		DrawSet* set = sorted->Item(i);
		for (size_t j = 0; j < set->GetDraws()->size(); j++) {
			DrawInfo* info = cfg->GetDraw(confid, set->GetName(), j);
			if (info == NULL) 
				continue;
				
			Item *item = new Item();
			item->draw_info = info;
			item->draw_set = set;
			items_array.Add(item);
		}
	}
}

void IncSearch::LoadParams()
{
	assert(cfg != NULL);
	for (size_t i = 0; i < items_array.GetCount(); ++i)
		delete items_array[i];
	items_array.Clear();

	SortedSetsArray sorted(DrawSet::CompareSets);
	if (m_window_search) {
		sorted = cfg->GetConfigByPrefix(confid)->GetSortedDrawSetsNames(true);
		AddWindowItems(&sorted);
	} else {
		sorted = cfg->GetConfigByPrefix(confid)->GetSortedDrawSetsNames(m_show_defined);
		AddDrawsItems(&sorted);
	}

	if (m_cur_items != NULL && m_cur_items != &items_array)
		delete m_cur_items;
	m_cur_items = &items_array;

	selected_index = wxNOT_FOUND;
}

void IncSearch::OnClose(wxCloseEvent & event)
{
	if (event.CanVeto()) {
		event.Veto();
		Show(false);
	} else {
		Destroy();
	}
}

void IncSearch::SelectNextDraw() {
	int selected = item_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (selected == item_list->GetItemCount() - 1
			|| selected == -1)
		return;
	item_list->Select(selected, false);
	selected = selected + 1;
	item_list->Select(selected, true);
	item_list->EnsureVisible(wxMin(selected, item_list->GetItemCount() - 1));
}

void IncSearch::GoPageDown() {
	int selected = item_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (selected == -1)
		return;

	int ipp = item_list->GetCountPerPage();
	item_list->Select(selected, false);
	selected = wxMin(item_list->GetItemCount() - 1, selected + ipp);
	item_list->Select(selected, true);
	item_list->EnsureVisible(selected);
}

void IncSearch::SelectPreviousDraw() {
	int selected = item_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (selected == -1
			|| selected == 0)
		return;

	item_list->Select(selected, false);
	selected = selected - 1;
	item_list->Select(selected, true);
	item_list->EnsureVisible(selected);
}

void IncSearch::GoPageUp() {
	int selected = item_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (selected == -1)
		return;

	int ipp = item_list->GetCountPerPage();
	item_list->Select(selected, false);
	selected = wxMax(0, selected - ipp);
	item_list->Select(selected, true);
	item_list->EnsureVisible(selected);
}

void IncSearch::StartWith(DrawSet *set) {
	m_start_set = set;
	m_start_draw_info = NULL;
	if (window_created)
		SyncToStartDraw();

}

void IncSearch::StartWith(DrawSet *set, DrawInfo* info) {
	m_start_set = set;
	m_start_draw_info = info;
	if (window_created)
		SyncToStartDraw();
}

void IncSearch::SetConfigName(wxString cn) {
	wxChoice *db_picker = wxDynamicCast(FindWindow(incsearch_CHOICE), wxChoice);
	if (db_picker == NULL)
		return;

	bool sel = db_picker->SetStringSelection(cn);
	assert(sel);

	if (!window_created)
		return;

	wxCommandEvent evt;
	evt.SetString(cn);

	OnChoice(evt);
}

void IncSearch::SetRemoved(wxString prefix, wxString name) {
	if (!window_created)
		return;

	if (prefix != confid)
		return;

	LoadParams();
	Search();
}

void IncSearch::SetRenamed(wxString prefix, wxString from, wxString to, DrawSet *set) {
	if (!window_created)
		return;

	if (prefix != confid)
		return;

	LoadParams();
	Search();
}

void IncSearch::SetModified(wxString prefix, wxString name, DrawSet *set) {
	if (!window_created)
		return;

	if (prefix != confid)
		return;

	LoadParams();
	Search();
}

void IncSearch::SetAdded(wxString prefix, wxString name, DrawSet *set) {
	if (!window_created)
		return;

	if (prefix != confid)
		return;

	LoadParams();
	Search();
}

void IncSearch::OnWindowCreate(wxWindowCreateEvent &event) {
	if (event.GetEventObject() == this)
		FinishWindowCreation();
}

void IncSearch::SelectEntry(wxString string_to_select) {
	const wxString& match = input_text->GetValue();
	bool matches = false;

	if (match != wxEmptyString) {
		try {
			std::wregex re(match.wc_str(), std::regex_constants::icase);
			matches = std::regex_search(string_to_select.wc_str(), re);
		} catch(const std::regex_error& e) {
			matches = false;
		}
	}

	if (!matches) {
		input_text->Clear();
		Search();
	}

	size_t i;
	for (i = 0; i < m_cur_items->GetCount(); i++)
		if (string_to_select == (*m_cur_items)[i]->GetName())
			break;

	if (i == m_cur_items->GetCount())
		return;

	int selected = item_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (selected != wxNOT_FOUND)
		item_list->Select(selected, false);
	item_list->Select(i, true);
	item_list->EnsureVisible(i);
}

void IncSearch::SyncToStartDraw() {
	wxString nprefix;
	if (m_start_set)
		nprefix = m_start_set->GetDrawsSets()->GetPrefix();

	if (m_start_set && confid != nprefix) {
		if (db_picker) {
			ConfigNameHash hash = cfg->GetConfigTitles();
			SetConfigName(hash[nprefix]);
		} else {
			confid = nprefix;
			LoadParams();
		}
	}

	wxString string_to_select;
	if (m_window_search && m_start_set) {
		Item i;
		i.draw_set = m_start_set;
		i.draw_info = NULL;
		string_to_select = i.GetName();
	} else if (!m_window_search && m_start_set && m_start_draw_info) {
		Item i;
		i.draw_set = m_start_set;
		i.draw_info = m_start_draw_info;
		string_to_select = i.GetName();
	}

	m_start_set = NULL;
	m_start_draw_info = NULL;

	if (string_to_select.IsEmpty())
		return;

	SelectEntry(string_to_select);
}

void IncSearch::FinishWindowCreation() {

#if __WXGTK__
	wxSize bs, is, ws, dbs(0,0);


	bs = button_sizer->GetSize();

	is = input_sizer->GetSize();

	if (db_picker)
		dbs = db_picker->GetSize();

	ws = GetSize();

	item_list = new ListCtrl(&items_array, this, incsearch_LIST, wxDefaultPosition,
				wxSize(ws.GetWidth() - 30, ws.GetHeight() - bs.GetHeight() - is.GetHeight() - dbs.GetHeight() - 50),
				 wxLC_VIRTUAL | wxLC_REPORT | wxLC_NO_HEADER);

#endif
	item_list->InsertColumn(0, _T(""));
	item_list->PushEventHandler(new IncKeyboardHandler(this, true));

	LoadParams();
	SetColumnWidth();
	UpdateItemList();

#ifdef __WXGTK__
	if (db_picker == NULL)
		top_sizer->Insert(1, item_list, 1, wxALL | wxEXPAND, 8);
	else
		top_sizer->Insert(2, item_list, 1, wxALL | wxEXPAND, 8);
#endif


	item_list->MoveAfterInTabOrder(input_text);
	ok_button->MoveAfterInTabOrder(item_list);
	close_button->MoveAfterInTabOrder(ok_button);
	help_button->MoveAfterInTabOrder(close_button);
	reset_button->MoveAfterInTabOrder(help_button);

	IncKeyboardHandler* list_key_hand = new IncKeyboardHandler(this);
	item_list->PushEventHandler(list_key_hand);

#if 0
#ifdef __WXGTK__
	SetSize(400, 600);
#endif
#endif
	top_sizer->Layout();

	window_created = true;

	SyncToStartDraw();
}

void IncSearch::ConfigurationWasReloaded(wxString prefix) {
	if (window_created == false)
		return;

	LoadParams();
	Search();
}

DrawSet* IncSearch::GetSelectedSet() {
	if (selected_index == wxNOT_FOUND)
		return NULL;

	Item *i = (*m_cur_items)[selected_index];
	return i->draw_set;
}

void IncSearch::OnRightItemClick(wxListEvent &event) {
	long idx = event.GetIndex();
	if (idx < 0)
		return;

	selected_index = idx;

	Item *i = (*m_cur_items)[idx];

	if (m_window_search) {
		DefinedDrawSet *dds = dynamic_cast<DefinedDrawSet*>(i->draw_set);

		if (dds == NULL)
			return; 
	} else {
		DefinedDrawInfo *di = dynamic_cast<DefinedDrawInfo*>(i->draw_info);
		if (di == NULL)
			return;
		
	}

	wxMenu* menu = new wxMenu();
	menu->Append(incsearch_MENU_REMOVE, _("Remove"));
	menu->Append(incsearch_MENU_EDIT, _("Edit"));
	PopupMenu(menu);

}

void IncSearch::OnEditMenu(wxCommandEvent &e) {
	Item *i = (*m_cur_items)[selected_index];
	
	DrawsSets* dss = cfg->GetConfigByPrefix(confid);
	DefinedDrawSet *ds = dynamic_cast<DefinedDrawSet*>(i->draw_set);
	if (dss == NULL)
		return;

	if (m_window_search) {
		DrawPicker *dp = new DrawPicker(this, cfg, db_mgr, m_remarks_handler);
		dp->EditSet(ds, confid);
		dp->Destroy();
	} else {
		DefinedDrawInfo *di = dynamic_cast<DefinedDrawInfo*>(i->draw_info);
		if (di == NULL)
			return;

		DrawPicker *dp = new DrawPicker(this, cfg, db_mgr, m_remarks_handler);
		dp->EditDraw(di, confid);
		dp->Destroy();
	}
}

void IncSearch::OnRemoveMenu(wxCommandEvent &e) {

	Item *i = (*m_cur_items)[selected_index];
	
	DrawsSets* dss = cfg->GetConfigByPrefix(confid);
	DefinedDrawSet *ds = dynamic_cast<DefinedDrawSet*>(i->draw_set);
	if (dss == NULL)
		return;

	DefinedDrawsSets *dds = ds->GetDefinedDrawsSets();
	if (m_window_search || ds->GetDraws()->size() == 1) {
		int answer = wxMessageBox(_("Do you really want to remove this set?"),
			     _("Set removal"), wxYES_NO | wxICON_QUESTION, this);
		if (answer != wxYES)
			return;

		if (ds->IsNetworkSet())
			m_remarks_handler->GetConnection()->InsertOrUpdateSet(ds, this, true);
		else
			dds->RemoveSet(i->draw_set->GetName());
		
	} else {
		int answer = wxMessageBox(_("Do you really want to remove this graph?"),
			     _("Graph removal"), wxYES_NO | wxICON_QUESTION, this);
		if (answer != wxYES)
			return;

		DefinedDrawInfo *di = dynamic_cast<DefinedDrawInfo*>(i->draw_info);
		if (di == NULL)
			return;
		if (ds->IsNetworkSet()) {
			size_t idx;
			for (idx = 0; idx < ds->GetDraws()->size(); idx++)
				if (ds->GetDraws()->at(idx) == di)
					break;

			if (idx == ds->GetDraws()->size())
				return;

			DefinedDrawSet* s = ds->MakeDeepCopy();
			s->Remove(idx);
			m_remarks_handler->GetConnection()->InsertOrUpdateSet(s, this, false);

			delete s;
		} else {
			dds->RemoveDrawFromSet(di);
		}
	}


}

void IncSearch::OnHelpButton(wxCommandEvent &event) {
	wxHelpProvider::Get()->ShowHelp(this);
}

void IncSearch::SetInsertUpdateError(wxString error) {
	wxMessageBox(_("Error in communicatoin with server: ") + error, _("Failed to update paramaters"),
		wxOK | wxICON_ERROR, this);
}

void IncSearch::SetInsertUpdateFinished(bool ok) {
	if (ok)
		m_remarks_handler->GetConnection()->FetchNewParamsAndSets(this);
	else
		wxMessageBox(_("You are not allowed to remove this set"), _("Failed to remove paramater"),
			wxOK | wxICON_ERROR, this);
}

void IncSearch::SetsParamsReceiveError(wxString error) {
	wxMessageBox(_("Error in communicatoin with server: ") + error, _("Failed to update paramaters"),
		wxOK | wxICON_ERROR, this);
}

void IncSearch::SetsParamsReceived(bool) {

}


IncSearch::~IncSearch()
{
	for (size_t i = 0; i < items_array.GetCount(); ++i)
		delete items_array[i];

	cfg->DeregisterConfigObserver(this);	

#if 0
	XXX - to do -cleanup
	item_list->PopEventHandler(list_key_hand);
	input_text->PopEventHandler(list_key_hand);
	reset_button->PopEventHandler(button_key_hand);
	ok_button->PopEventHandler(button_key_hand);
	close_button->PopEventHandler(button_key_hand);

#endif
}



BEGIN_EVENT_TABLE(IncSearch, wxDialog)
    EVT_BUTTON(wxOK, IncSearch::OnOk)
    EVT_BUTTON(wxRESET, IncSearch::OnReset)
    EVT_BUTTON(wxCANCEL, IncSearch::OnCancel)
    EVT_BUTTON(wxHELP, IncSearch::OnHelpButton)
    EVT_LIST_ITEM_ACTIVATED(incsearch_LIST, IncSearch::OnActivated)
    EVT_LIST_ITEM_RIGHT_CLICK(incsearch_LIST, IncSearch::OnRightItemClick)
    EVT_CLOSE(IncSearch::OnClose)
    EVT_CHOICE(incsearch_CHOICE, IncSearch::OnChoice)
    EVT_TEXT(incsearch_TEXT, IncSearch::OnSearch)
    EVT_MENU(incsearch_MENU_EDIT, IncSearch::OnEditMenu)
    EVT_MENU(incsearch_MENU_REMOVE, IncSearch::OnRemoveMenu)
#ifdef __WXGTK__
    EVT_WINDOW_CREATE(IncSearch::OnWindowCreate)
#endif
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(IncKeyboardHandler, wxEvtHandler)
	EVT_CHAR(IncKeyboardHandler::OnChar)
END_EVENT_TABLE()


IMPLEMENT_DYNAMIC_CLASS(IncSearch, wxDialog)
