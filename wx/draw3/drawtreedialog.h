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

#include "wxlogging.h"

#ifndef _DRAWTREEDIALOG_H
#define _DRAWTREEDIALOG_H

class DrawTreeDialog : public wxDialog, public ConfigObserver {
	wxTreeCtrl* m_tree_ctrl;
	ConfigManager* m_cfg_mgr;
	wxString m_cfg_prefix;
	void CreateTree();
	void AddChildren(wxTreeItemId parent, DrawTreeNode*);
	std::list<wxString> GetSelectedItemPath();
	wxTreeItemId GetItemFromPath(const std::list<wxString>& path);
	wxTreeItemId SearchSetDown(wxTreeItemId current, DrawSet* set, wxTreeItemId exclude = wxTreeItemId()); 
	wxTreeItemId SearchSetUp(wxTreeItemId current, wxTreeItemId coming_from, DrawSet *set);
public:
	DrawTreeDialog(wxWindow* parent, ConfigManager* cfg_mgr);
	DrawSet* GetSelectedSet();
	void SetSelectedSet(DrawSet *drawset);
	void OnOk(wxCommandEvent&);
	void OnCancel(wxCommandEvent&);
	void OnHelp(wxCommandEvent&);
	virtual void SetRemoved(wxString prefix, wxString name);
	virtual void SetRenamed(wxString prefix, wxString from, wxString to, DrawSet *set);
	virtual void SetModified(wxString prefix, wxString name, DrawSet *set);
	virtual void SetAdded(wxString prefix, wxString name, DrawSet *added);
	virtual void ConfigurationWasReloaded(wxString prefix);

	~DrawTreeDialog();
	DECLARE_EVENT_TABLE()

};

#endif

