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
/* * draw3
 * SZARP
 
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 *
 * $Id$
 * Widget for selecting draws.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <wx/config.h>
#include <sstream>

#include "md5.h"
#include "selset.h"
#include "seldraw.h"
#include "drawswdg.h"
#include "cfgmgr.h"
#include "ids.h"
#include "drawdnd.h"
#include "cconv.h"
#include "pscgui.h"
#include "drawpnl.h"
#include "drawurl.h"
#include "defcfg.h"
#include "paredit.h"

//IMPLEMENT_DYNAMIC_CLASS(SelectDrawValidator, wxValidator)

BEGIN_EVENT_TABLE(SelectDrawValidator, wxValidator)
	EVT_CHECKBOX(drawID_SELDRAWCB, SelectDrawValidator::OnCheck)
	EVT_RIGHT_DOWN(SelectDrawValidator::OnMouseRightDown)
	EVT_MIDDLE_DOWN(SelectDrawValidator::OnMouseMiddleDown)
END_EVENT_TABLE()

SelectDrawValidator::SelectDrawValidator(DrawsWidget *drawswdg, int index, wxCheckBox* cb)
{
	assert (drawswdg != NULL);
	this->drawswdg = drawswdg;
	assert ((index >= 0) && (index < MAX_DRAWS_COUNT));
	this->index = index;
	this->cb = cb;
	menu = NULL;
}

SelectDrawValidator::SelectDrawValidator(const SelectDrawValidator& val)
{
	Copy(val);
}

bool
SelectDrawValidator::Copy(const SelectDrawValidator& val)
{
	wxValidator::Copy(val);
	drawswdg = val.drawswdg;
	index = val.index;
	cb = val.cb;
	menu = NULL;
	return TRUE;
}

bool
SelectDrawValidator::Validate(wxWindow *parent)
{
    return TRUE;
}

bool
SelectDrawValidator::TransferToWindow()
{
    return TRUE;
}

bool
SelectDrawValidator::TransferFromWindow()
{
    return TRUE;
}	

void
SelectDrawValidator::OnCheck(wxCommandEvent& c)
{
    wxLogInfo(_T("DEBUG: OnCheck"));
    
    if (c.IsChecked()) 
	drawswdg->SetDrawEnable(index);
    else if (!drawswdg->SetDrawDisable(index))
	    	((wxCheckBox *)(c.GetEventObject()))->SetValue(TRUE);

    drawswdg->SetFocus();
}

void 
SelectDrawValidator::OnMouseRightDown(wxMouseEvent &event) {

	delete menu;
	menu = new wxMenu();
	
	DrawParam* dp = drawswdg->GetDrawInfo(index)->GetParam();
	if (dp->GetIPKParam()->GetPSC())
		menu->Append(seldrawID_PSC,_("Set parameter"));
	
	menu->SetClientData(cb);

	if (drawswdg->IsDrawBlocked(index)) {
		wxMenuItem* item = menu->AppendCheckItem(seldrawID_CTX_BLOCK_MENU, _("Draw blocked\tCtrl-B"));
		item->Check();
	} else {
		int non_blocked_count = 0;
		for (unsigned int i = 0; i < drawswdg->GetDrawsCount(); ++i)
			if (!drawswdg->IsDrawBlocked(i))
				non_blocked_count++;

		//one draw shall be non-blocked
		if (non_blocked_count > 1)
			menu->AppendCheckItem(seldrawID_CTX_BLOCK_MENU, _("Draw blocked\tCtrl-B"));
	}

	menu->Append(seldrawID_CTX_DOC_MENU, _("Parameter documentation\tCtrl-H"));

	if (dynamic_cast<DefinedParam*>(dp) != NULL)
		menu->Append(seldrawID_CTX_EDIT_PARAM, _("Edit parameter associated with graph\tCtrl-E"));
		
	cb->PopupMenu(menu);
}

void SelectDrawValidator::OnMouseMiddleDown(wxMouseEvent &event) {

	DrawInfo *draw_info = drawswdg->GetDrawInfo(index);
	if (draw_info == NULL)
		return;

	DrawInfoDataObject didb(draw_info);

	wxDropSource ds(didb, cb);
	ds.DoDragDrop(0);

}

void SelectDrawValidator::Set(DrawsWidget *drawswdg, int index, wxCheckBox *cb) {
	this->drawswdg = drawswdg;
	this->index = index;
	this->cb = cb;
}


SelectDrawValidator::~SelectDrawValidator() {
	delete menu;
}

BEGIN_EVENT_TABLE(SelectDrawWidget, wxWindow)
	EVT_MENU(seldrawID_CTX_BLOCK_MENU, SelectDrawWidget::OnBlockCheck)
	EVT_MENU(seldrawID_PSC, SelectDrawWidget::OnPSC)
	EVT_MENU(seldrawID_CTX_DOC_MENU, SelectDrawWidget::OnDocs)
	EVT_MENU(seldrawID_CTX_EDIT_PARAM, SelectDrawWidget::OnEditParam)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(SelectDrawWidget, wxWindow)


SelectDrawWidget::SelectDrawWidget(ConfigManager *cfg, DatabaseManager *dbmgr, wxString confid, 
		SelectSetWidget *selset, DrawsWidget *drawswdg,
		wxWindow* parent, wxWindowID id)
        : wxWindow(parent, id, wxDefaultPosition, wxDefaultSize, 
        wxWANTS_CHARS, _T("SelectDrawWidget"))
{
    assert (cfg != NULL);
    this->cfg = cfg;
    this->confid = confid;
    assert (selset != NULL);
    this->selset = selset;
    assert(drawswdg != NULL);
    this->drawswdg = drawswdg;
    assert(dbmgr != NULL);
    this->dbmgr = dbmgr;

    SetHelpText(_T("draw3-base-hidegraphs"));
	
    selset->SetSelectDrawWidget(this);

    wxBoxSizer * sizer1 = new wxBoxSizer(wxVERTICAL);
    drawswdg->SetDrawClear();

    wxClientDC dc(this);
    wxFont f = GetFont();
    dc.SetFont(GetFont());
    int y, width;
    dc.GetTextExtent(_T("0123456789012345678901234567"), &width,
			 &y);

    for (int i = 0; i < MAX_DRAWS_COUNT; i++) {
	cb_l[i].Create(this, drawID_SELDRAWCB,
		wxString::Format(_T("%d."), i + 1),
		wxDefaultPosition, wxSize(width, -1), 0,
		SelectDrawValidator(drawswdg, i, &cb_l[i]));
	cb_l[i].Enable(FALSE);

	cb_l[i].SetToolTip(wxEmptyString);
	cb_l[i].SetBackgroundColour(DRAW3_BG_COLOR);
	  

	sizer1->Add(&cb_l[i], 0, wxTOP | wxLEFT | wxRIGHT, 1);
	
#ifdef MINGW32
	cb_l[i].Refresh();
#endif
    }

    SetSizer(sizer1);
    sizer1->SetSizeHints(this);
    sizer1->Layout();

}

void SelectDrawWidget::SetChecked(int idx, bool checked) {
	if (cb_l[idx].IsEnabled())
		cb_l[idx].SetValue(checked);
}

void
SelectDrawWidget::SetChanged()
{
    DrawSet * selected_set = selset->GetSelected();
    assert (selected_set != NULL);
    
    drawswdg->SetDrawClear();

    ConfigNameHash& cnm = const_cast<ConfigNameHash&>(cfg->GetConfigTitles());
    
    for (int i = 0; i < MAX_DRAWS_COUNT; i++) {
	DrawInfo* draw_info = selected_set->GetDraw(i);
	selected_set->GetDrawName(i);
	
	if (NULL == draw_info) {
	    cb_l[i].SetLabel(wxString::Format(_T("%d."), i + 1));
	    cb_l[i].Enable(FALSE);
	    cb_l[i].SetValue(FALSE);
	    cb_l[i].SetToolTip(_T(" "));
	}
	else {
	    wxString label = wxString::Format(_T("%d."), i + 1) + draw_info->GetName();
	    if (draw_info->GetParam()->GetIPKParam()->GetPSC())
		    label += _T("*");
	    cb_l[i].SetLabel(label);
	    cb_l[i].Enable(TRUE);
	    cb_l[i].SetValue(TRUE);
	    cb_l[i].SetToolTip(cnm[draw_info->GetBasePrefix()] + _T(":") + draw_info->GetParamName());
	    drawswdg->SetDrawAdd(draw_info);
	}

	wxValidator* validator = cb_l[i].GetValidator();
	if (validator) 
		dynamic_cast<SelectDrawValidator*>(validator)->Set(drawswdg, i, &cb_l[i]);
	else
		cb_l[i].SetValidator(SelectDrawValidator(drawswdg, i, &cb_l[i]));

	cb_l[i].SetBackgroundColour(selected_set->GetDrawColor(i));

#ifdef MINGW32
	cb_l[i].Refresh();
#endif
    }
    
    drawswdg->SetDrawApply();
}

void 
SelectDrawWidget::SetDrawEnable(int index, bool enable) 
{
	assert(index >= 0 && index < MAX_DRAWS_COUNT);
	cb_l[index].Enable(enable);
}

void SelectDrawWidget::SelectDraw(wxString name)
{
	drawswdg->SelectDraw(name);
}

int SelectDrawWidget::GetClicked(wxCommandEvent &event) {
	wxMenu *menu = wxDynamicCast(event.GetEventObject(), wxMenu);
	assert(menu);

	wxCheckBox* cb = (wxCheckBox*) menu->GetClientData();

	int i = 0;

	for (; i < MAX_DRAWS_COUNT; ++i)
		if (cb == &cb_l[i])
			break;

	assert(i < MAX_DRAWS_COUNT);
	return i;
}

void 
SelectDrawWidget::OnBlockCheck(wxCommandEvent &event) {

	int i = GetClicked(event);
	drawswdg->BlockDraw(i, event.IsChecked());
}

void 
SelectDrawWidget::SetBlocked(int idx, bool blocked) {
	wxString label = cb_l[idx].GetLabel();
	if (blocked)
		label.Replace(wxString::Format(_T("%d."), idx + 1), wxString::Format(_("%d.[B]"), idx + 1), false);
	else
		label.Replace(_T("[B]"), _T(""), false);
	cb_l[idx].SetLabel(label);

}

void
SelectDrawWidget::OnPSC(wxCommandEvent &event) {
	int i = GetClicked(event);
	DrawInfo* info = drawswdg->GetDrawInfo(i);
	cfg->EditPSC(info->GetBasePrefix(), info->GetParamName());
}

void SelectDrawWidget::OnEditParam(wxCommandEvent &event) {
	int i = GetClicked(event);

	DrawInfo *d = drawswdg->GetDrawInfo(i);

	DefinedParam *dp = dynamic_cast<DefinedParam*>(d->GetParam());
	if (dp == NULL)
		return;

	wxWindow *w = this;
	while (!w->IsTopLevel())
		w = w->GetParent();

	ParamEdit* pe = new ParamEdit(w, cfg, dbmgr);
	pe->SetCurrentConfig(d->GetBasePrefix());
	pe->Edit(dp);
	delete pe;

}

void SelectDrawWidget::OnDocs(wxCommandEvent &event) {
	wxMenu *menu = wxDynamicCast(event.GetEventObject(), wxMenu);
	assert(menu);

	wxCheckBox* cb = (wxCheckBox*) menu->GetClientData();

	int i = 0;
	for (; i < MAX_DRAWS_COUNT; ++i)
		if (cb == &cb_l[i])
			break;

	OpenParameterDoc(i);
}

void SelectDrawWidget::OpenParameterDoc(int i) {
	DrawInfo* d;
	if (i == -1)
		d = drawswdg->GetCurrentDrawInfo();
	else
		d = drawswdg->GetDrawInfo(i);

	if (d == NULL)
		return;

	TParam *p = d->GetParam()->GetIPKParam();
	TSzarpConfig* sc = p->GetSzarpConfig();

	wxString link = sc->GetDocumentationBaseURL();

	if (link.IsEmpty())
		link = _T("https://www.szarp.com.pl");

	link += _T("/cgi-bin/param_docs.py?");
	link << _T("prefix=") << d->GetBasePrefix().c_str();

	link << _T("&param=") << encode_string(p->GetName().c_str());

	wxString username, password;


	TUnit* u;
	if ((u = p->GetParentUnit())) {
		TDevice* dev = u->GetDevice();
		link << _T("&path=") << encode_string(dev->GetPath().c_str());
	}

	int validity = 8 * 3600;
	std::wstringstream tss;
	tss << (wxDateTime::Now().GetTicks() / validity * validity);

	link << _T("&id=") << sz_md5(tss.str());

#if __WXMSW__
	if (wxLaunchDefaultBrowser(link) == false) 
#else
	if (wxExecute(wxString::Format(_T("xdg-open %s"), link.c_str())) == 0)
#endif
		wxMessageBox(_("I was not able to start default browser"), _("Error"), wxICON_ERROR | wxOK);

}
