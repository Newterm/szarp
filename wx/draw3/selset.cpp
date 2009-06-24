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

#include "defcfg.h"
#include "selset.h"
#include "seldraw.h"
#include "drawpnl.h"
#include "ids.h"
#include "cconv.h"

#include "wx/file.h"
#include "wx/log.h"

IMPLEMENT_DYNAMIC_CLASS(SelectSetWidget, wxChoice)

SelectSetWidget::SelectSetWidget(ConfigManager *cfg,
			wxString confid,
			DrawPanel *parent,
			wxWindowID id,
			DrawSet *set,
			int width)
        : wxChoice(parent, id, wxDefaultPosition, wxSize(width, -1), 0,	NULL, 
		wxWANTS_CHARS, wxDefaultValidator, _T("SelectSetWidget"))
{
    this->cfg = cfg;
    this->confid = confid;
    this->seldraw = NULL;

    this->panel = parent;
	
    SetToolTip(_("Select set of draws to display"));

    SetConfig();

    if (set)
	    SelectSet(set);
    else
	    SetSelection(0);

    Connect(drawID_SELSET, wxEVT_COMMAND_CHOICE_SELECTED,
		        wxCommandEventHandler(SelectSetWidget::OnSetChanged));

#if 0
    /* adjust strings that are to long */
#ifdef MINGW32
/* set experimentally */
#define CHOICE_MARGIN	12
#else
#define CHOICE_MARGIN	40
#endif
    for (int i = 0; i < (int)GetCount(); i++) {
	AdjustString(i, width - CHOICE_MARGIN);
    }
#undef CHOICE_MARGIN
#endif
}

void SelectSetWidget::SelectWithoutEvent(int selected) {
	Disconnect(drawID_SELSET, wxEVT_COMMAND_CHOICE_SELECTED,
		wxCommandEventHandler(SelectSetWidget::OnSetChanged));

	SetSelection(selected);
	Connect(drawID_SELSET, wxEVT_COMMAND_CHOICE_SELECTED,
		wxCommandEventHandler(SelectSetWidget::OnSetChanged));
}

SelectSetWidget::~SelectSetWidget() {

}

void SelectSetWidget::SetConfig() {
    SortedSetsArray * sorted = cfg->GetConfig(confid)->GetSortedDrawSetsNames();

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
    assert (n >= 0);

    return (DrawSet *) GetClientData(n);
}

void
SelectSetWidget::OnSetChanged(wxCommandEvent &event)
{
	SetChanged();
	panel->SetFocus();
}

void
SelectSetWidget::SetSelectDrawWidget(SelectDrawWidget *seldraw)
{
    this->seldraw = seldraw;
}

void SelectSetWidget::SetChanged() {
	if (seldraw)
		seldraw->SetChanged();
	panel->SetChanged();
}

void 
SelectSetWidget::SelectSet(DrawSet *set) {
    for (size_t i = 0; i < GetCount(); ++i)
	    if (GetClientData(i) == set) {
		    SetSelection(i);
		    break;
	    }
}

void 
SelectSetWidget::SelectSet(wxString confid, DrawSet *set) {
	this->confid = confid;
	SetConfig();
	SelectSet(set);
	SetChanged();
}

void
SelectSetWidget::AdjustString(int n, int w)
{
    int x, y;
    size_t l;
    wxClientDC dc(this);
    /* this is necessary on Windows and harmless on Linux */
    dc.SetFont(GetFont());
    wxString s = GetString(n);
    dc.GetTextExtent(s, &x, &y);
    
    if (x > w) {
	l = s.length();
	s.Truncate(l - 2);
	s += _T("...");
	l++;
	
	while (x > w) {
	    if (l <= 4) {
		return;
	    }
	    s.Remove(l - 4, 1);
	    l--;
	    dc.GetTextExtent(s, &x, &y);
	}
	SetString(n, s);
    }
}

