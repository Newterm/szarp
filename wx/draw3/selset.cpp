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
 
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 *
 * $Id$
 * Widget for selecting set of draws.
 */

#include "cconv.h"

#include "ids.h"
#include "classes.h"

#include "drawobs.h"
#include "database.h"
#include "drawtime.h"
#include "draw.h"
#include "coobs.h"
#include "dbinquirer.h"
#include "drawsctrl.h"
#include "cfgmgr.h"
#include "defcfg.h"
#include "drawswdg.h"
#include "selset.h"
#include "drawpnl.h"

#include "wx/file.h"
#include "wx/log.h"

IMPLEMENT_DYNAMIC_CLASS(SelectSetWidget, wxChoice)

SelectSetWidget::SelectSetWidget(ConfigManager *cfg,
			DrawPanel *parent,
			wxWindowID id,
			int width)
        : wxChoice(parent, id, wxDefaultPosition, wxSize(width, -1), 0,	NULL, 
		wxWANTS_CHARS, wxDefaultValidator, _T("SelectSetWidget"))
{
    m_cfg = cfg;
    m_draws_controller = NULL;

    SetToolTip(_("Select set of draws to display"));

    Connect(drawID_SELSET, wxEVT_COMMAND_CHOICE_SELECTED,
		        wxCommandEventHandler(SelectSetWidget::OnSetChanged));

}

void SelectSetWidget::SetSelection(int selected) {
	Disconnect(drawID_SELSET, wxEVT_COMMAND_CHOICE_SELECTED,
		wxCommandEventHandler(SelectSetWidget::OnSetChanged));

	wxChoice::SetSelection(selected);
	Connect(drawID_SELSET, wxEVT_COMMAND_CHOICE_SELECTED,
		wxCommandEventHandler(SelectSetWidget::OnSetChanged));
}

SelectSetWidget::~SelectSetWidget() {

}

void SelectSetWidget::SetConfig() {
    SortedSetsArray * sorted = m_cfg->GetConfigByPrefix(m_prefix)->GetSortedDrawSetsNames();

    Clear();
    int count = sorted->size();

    for (int i = 0; i < count; i++) {
	Append(sorted->Item(i)->GetName());
	SetClientData(i, (void *) sorted->Item(i));
    }
    delete sorted;

}


DrawSet *
SelectSetWidget::GetSelected()
{
    int n = GetSelection();
    if (n < 0)
	    return NULL;

    return (DrawSet *) GetClientData(n);
}

void
SelectSetWidget::OnSetChanged(wxCommandEvent &event)
{
	if (m_draws_controller) {
		m_draws_controller->Set(GetSelected());
		GetParent()->SetFocus();
	}
}

void
SelectSetWidget::Attach(DrawsController* draws_controller) {
	DrawObserver::Attach(draws_controller);

	m_draws_controller = draws_controller;
}

void 
SelectSetWidget::DrawInfoChanged(Draw *draw) {
	if (!draw->GetSelected())
		return;

	SelectSet(draw->GetDrawsController()->GetSet());
}

void 
SelectSetWidget::SelectSet(DrawSet *set) {
	if (set->GetDrawsSets()->GetPrefix() != m_prefix) {
		m_prefix = set->GetDrawsSets()->GetPrefix();
		SetConfig();
	}

	if (GetSelected() != set) for (size_t i = 0; i < GetCount(); ++i)
		if (GetClientData(i) == set) {
			SetSelection(i);
			break;
		}
}

