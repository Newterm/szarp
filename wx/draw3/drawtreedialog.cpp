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

#include <list>

#include <wx/colordlg.h>
#include <wx/filesys.h>
#include <wx/file.h>
#include <wx/xrc/xmlres.h>
#include <wx/treectrl.h>


#include "ids.h"
#include "classes.h"
#include "coobs.h"
#include "cfgmgr.h"
#include "drawtreedialog.h"

class TreeItemData : public wxTreeItemData {
public:
	DrawTreeNode *node;
	TreeItemData(DrawTreeNode *_node) : node(_node) {}
};

DrawTreeDialog::DrawTreeDialog(wxWindow *parent, ConfigManager* cfg_mgr) : m_cfg_mgr(cfg_mgr) {
	wxXmlResource::Get()->LoadObject(this, parent, _T("DrawTreeDialog"), _T("wxDialog"));
	m_tree_ctrl = XRCCTRL(*this, "DrawTreeCtrl", wxTreeCtrl);
	m_cfg_mgr->RegisterConfigObserver(this);
}

void DrawTreeDialog::CreateTree() {
	std::list<wxString> selected_path = GetSelectedItemPath();
	m_tree_ctrl->DeleteAllItems();
	DrawTreeNode& root_node = m_cfg_mgr->GetConfigByPrefix(m_cfg_prefix)->GetDrawTree();
	wxTreeItemId root_id = m_tree_ctrl->AddRoot(_T(""), -1, -1, new TreeItemData(&root_node));
	AddChildren(root_id, &root_node);
	wxTreeItemId id = GetItemFromPath(selected_path);
	if (id.IsOk())
		m_tree_ctrl->SelectItem(id);
}

void DrawTreeDialog::AddChildren(wxTreeItemId parent, DrawTreeNode* node) {
	for (std::vector<DrawTreeNode*>::const_iterator i = node->GetChildren().begin(); i != node->GetChildren().end(); i++) {
		wxTreeItemId id = m_tree_ctrl->AppendItem(parent, (*i)->GetName(), -1, -1, new TreeItemData(*i));
		AddChildren(id, *i);
	}
}

std::list<wxString> DrawTreeDialog::GetSelectedItemPath() {
	wxTreeItemId id = m_tree_ctrl->GetSelection();
	wxTreeItemId root = m_tree_ctrl->GetRootItem();
	if (!id.IsOk())
		return std::list<wxString>();	
	std::list<wxString> r;
	while (id != root) {
		r.push_front(m_tree_ctrl->GetItemText(id));
		id = m_tree_ctrl->GetItemParent(id);
		if (!id.IsOk())
			return std::list<wxString>();	
	}
	return r;
}

wxTreeItemId DrawTreeDialog::GetItemFromPath(const std::list<wxString>& path) {
	if (path.size() == 0)
		return wxTreeItemId();
	wxTreeItemId pid = m_tree_ctrl->GetRootItem();
	for (std::list<wxString>::const_iterator i = path.begin(); i != path.end() && pid.IsOk(); i++) {
		wxString text = *i;
		wxTreeItemIdValue cookie;
		wxTreeItemId id = m_tree_ctrl->GetFirstChild(pid, cookie);
		while (id.IsOk()) {
			wxString _text = m_tree_ctrl->GetItemText(id);
			if (_text == text)
				break;
			id = m_tree_ctrl->GetNextChild(id, cookie);	
		}
		pid = id;
	}
	return pid;
}

DrawSet* DrawTreeDialog::GetSelectedSet() {
	wxTreeItemId s = m_tree_ctrl->GetSelection();
	if (!s.IsOk())
		return NULL;
	return dynamic_cast<TreeItemData*>(m_tree_ctrl->GetItemData(s))->node->GetDrawSet();
}

wxTreeItemId DrawTreeDialog::SearchSetDown(wxTreeItemId current, DrawSet* set, wxTreeItemId exclude) {
	DrawTreeNode* cnode = dynamic_cast<TreeItemData*>(m_tree_ctrl->GetItemData(current))->node;
	if (cnode->GetElementType() == DrawTreeNode::LEAF) {
		if (cnode->GetDrawSet() == set)
			return current;
		else
			return wxTreeItemId();
	}
	wxTreeItemIdValue cookie;
	wxTreeItemId id = m_tree_ctrl->GetFirstChild(current, cookie);
	while (id.IsOk()) {
		if (id != exclude) {
			wxTreeItemId cid = SearchSetDown(id, set);
			if (cid.IsOk())
				return cid;
		}
		id = m_tree_ctrl->GetNextChild(current, cookie);
	}
	return id;
}

wxTreeItemId DrawTreeDialog::SearchSetUp(wxTreeItemId current, wxTreeItemId coming_from, DrawSet *set) {
	wxTreeItemId id = SearchSetDown(current, set, coming_from);
	if (id.IsOk())
		return id;
	if (current == m_tree_ctrl->GetRootItem())
		return wxTreeItemId();
	else
		return SearchSetUp(m_tree_ctrl->GetItemParent(current), current, set);
}

void DrawTreeDialog::SetSelectedSet(DrawSet* set) {
	DrawsSets* ds = set->GetDrawsSets();
	wxTreeItemId id;
	if (ds->GetPrefix() != m_cfg_prefix) {
		m_cfg_prefix = ds->GetPrefix();
		CreateTree();
		id = SearchSetDown(m_tree_ctrl->GetRootItem(), set);
	} else {
		wxTreeItemId sid = m_tree_ctrl->GetSelection();
		if (sid.IsOk())
			id = SearchSetUp(sid, wxTreeItemId(), set);
		else
			id = SearchSetDown(m_tree_ctrl->GetRootItem(), set);
	}
	if (id.IsOk()) {
		m_tree_ctrl->EnsureVisible(id);
		m_tree_ctrl->SelectItem(id);
	}
}

void DrawTreeDialog::OnOk(wxCommandEvent&) {
	wxTreeItemId s = m_tree_ctrl->GetSelection();
	if (!s.IsOk())
		return;
	DrawTreeNode* node = dynamic_cast<TreeItemData*>(m_tree_ctrl->GetItemData(s))->node;
	if (node->GetElementType() == DrawTreeNode::LEAF)
		EndModal(wxID_OK);
}

void DrawTreeDialog::OnCancel(wxCommandEvent&) {
	EndModal(wxID_CANCEL);
}

void DrawTreeDialog::OnHelp(wxCommandEvent&) {
	//TODO
}

void DrawTreeDialog::SetRemoved(wxString prefix, wxString name) {
	if (prefix == m_cfg_prefix)
		CreateTree();
}

void DrawTreeDialog::SetRenamed(wxString prefix, wxString from, wxString to, DrawSet *set) {
	if (prefix == m_cfg_prefix)
		CreateTree();
}

void DrawTreeDialog::SetModified(wxString prefix, wxString name, DrawSet *set) {
	if (prefix == m_cfg_prefix)
		CreateTree();
}

void DrawTreeDialog::SetAdded(wxString prefix, wxString name, DrawSet *added) {
	if (prefix == m_cfg_prefix)
		CreateTree();
}

void DrawTreeDialog::ConfigurationWasReloaded(wxString prefix) {
	if (prefix == m_cfg_prefix)
		CreateTree();
}

DrawTreeDialog::~DrawTreeDialog() {
	m_cfg_mgr->DeregisterConfigObserver(this);
}

BEGIN_EVENT_TABLE(DrawTreeDialog, wxDialog)
	EVT_BUTTON(wxID_OK, DrawTreeDialog::OnOk)
	EVT_BUTTON(wxID_CANCEL, DrawTreeDialog::OnCancel)
	EVT_BUTTON(wxID_HELP, DrawTreeDialog::OnHelp)
END_EVENT_TABLE()
