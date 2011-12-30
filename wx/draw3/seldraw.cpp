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
#include <wx/clipbrd.h>

#include <sstream>

#include "cconv.h"

#include "ids.h"
#include "classes.h"
#include "md5.h"


#include "drawobs.h"
#include "pscgui.h"
#include "database.h"
#include "drawtime.h"
#include "draw.h"
#include "dbinquirer.h"
#include "coobs.h"
#include "drawsctrl.h"
#include "drawswdg.h"
#include "cfgmgr.h"
#include "codeeditor.h"
#include "drawdnd.h"
#include "drawurl.h"
#include "defcfg.h"
#include "paredit.h"
#include "seldraw.h"
#include "paredit.h"
#include "remarks.h"

//IMPLEMENT_DYNAMIC_CLASS(SelectDrawValidator, wxValidator)

static const size_t MIN_DRAWS_COUNT = 12;

BEGIN_EVENT_TABLE(SelectDrawValidator, wxValidator)
	EVT_CHECKBOX(drawID_SELDRAWCB, SelectDrawValidator::OnCheck)
	EVT_SET_FOCUS(SelectDrawValidator::OnFocus)
	EVT_RIGHT_DOWN(SelectDrawValidator::OnMouseRightDown)
	EVT_MIDDLE_DOWN(SelectDrawValidator::OnMouseMiddleDown)
END_EVENT_TABLE()

SelectDrawValidator::SelectDrawValidator(DrawsWidget *drawswdg, int index, wxCheckBox* cb)
{
	assert (drawswdg != NULL);
	m_draws_wdg = drawswdg;
	assert (index >= 0);
	m_index = index;
	m_cb = cb;
	m_menu = NULL;
}

SelectDrawValidator::SelectDrawValidator(const SelectDrawValidator& val)
{
	Copy(val);
}

bool
SelectDrawValidator::Copy(const SelectDrawValidator& val)
{
	wxValidator::Copy(val);
	m_draws_wdg = val.m_draws_wdg;
	m_index = val.m_index;
	m_cb = val.m_cb;
	m_menu = NULL;
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
		m_draws_wdg->SetDrawEnable(m_index);
	else if (!m_draws_wdg->SetDrawDisable(m_index))
		((wxCheckBox *)(c.GetEventObject()))->SetValue(TRUE);
}

void
SelectDrawValidator::OnFocus(wxFocusEvent& c)
{
#ifndef __WXMSW__
	m_draws_wdg->SetFocus();
#endif
}

void
SelectDrawValidator::OnMouseRightDown(wxMouseEvent &event) {
	DrawInfo* di = m_draws_wdg->GetDrawInfo(m_index);
	if (di == NULL)
		return;

	delete m_menu;
	m_menu = new wxMenu();

	DrawParam* dp = di->GetParam();
	if (di->IsValid() && dp->GetIPKParam()->GetPSC())
		m_menu->Append(seldrawID_PSC,_("Set parameter"));

	m_menu->SetClientData(m_cb);

	if (m_draws_wdg->GetDrawBlocked(m_index)) {
		wxMenuItem* item = m_menu->AppendCheckItem(seldrawID_CTX_BLOCK_MENU, _("Draw blocked\tCtrl-B"));
		item->Check();
	} else {
		int non_blocked_count = 0;
		for (unsigned int i = 0; i < m_draws_wdg->GetDrawsCount(); ++i)
			if (!m_draws_wdg->GetDrawBlocked(i))
				non_blocked_count++;

		//one draw shall be non-blocked
		if (non_blocked_count > 1)
			m_menu->AppendCheckItem(seldrawID_CTX_BLOCK_MENU, _("Draw blocked\tCtrl-B"));
	}

	m_menu->Append(seldrawID_CTX_DOC_MENU, _("Parameter documentation\tCtrl-H"));
	m_menu->Append(seldrawID_CTX_COPY_PARAM_NAME_MENU, _("Copy parameter name\tCtrl+Shift+C"));

	if (dynamic_cast<DefinedParam*>(dp) != NULL)
		m_menu->Append(seldrawID_CTX_EDIT_PARAM, _("Edit parameter associated with graph\tCtrl-E"));

	m_cb->PopupMenu(m_menu);
}

void SelectDrawValidator::OnMouseMiddleDown(wxMouseEvent &event) {

	DrawInfo *draw_info = m_draws_wdg->GetDrawInfo(m_index);
	if (draw_info == NULL)
		return;

	DrawInfoDataObject didb(draw_info);

	wxDropSource ds(didb, m_cb);
	ds.DoDragDrop(0);

}

void SelectDrawValidator::Set(DrawsWidget *drawswdg, int index, wxCheckBox *cb) {
	m_draws_wdg = drawswdg;
	m_index = index;
	m_cb = cb;
}

SelectDrawValidator::~SelectDrawValidator() {
	delete m_menu;
}


BEGIN_EVENT_TABLE(SelectDrawWidget, wxWindow)
	LOG_EVT_MENU(seldrawID_CTX_BLOCK_MENU, SelectDrawWidget , OnBlockCheck, "seldraw:block" )
	LOG_EVT_MENU(seldrawID_PSC, SelectDrawWidget , OnPSC, "seldraw:psc" )
	LOG_EVT_MENU(seldrawID_CTX_DOC_MENU, SelectDrawWidget , OnDocs, "seldraw:doc" )
	LOG_EVT_MENU(seldrawID_CTX_COPY_PARAM_NAME_MENU, SelectDrawWidget , OnCopyParamName , "seldraw:cpyparam" )
	LOG_EVT_MENU(seldrawID_CTX_EDIT_PARAM, SelectDrawWidget , OnEditParam, "seldraw:edit" )
	EVT_TIMER(wxID_ANY, SelectDrawWidget::OnTimer)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(SelectDrawWidget, wxWindow)

SelectDrawWidget::SelectDrawWidget(ConfigManager *cfg, DatabaseManager *dbmgr, DrawsWidget *drawswdg, RemarksHandler *remarks_handler, wxWindow* parent, wxWindowID id): wxScrolledWindow(parent, id, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS, _T("SelectDrawWidget")), m_remarks_handler(remarks_handler)
{
	assert (cfg != NULL);
	m_cfg = cfg;
	assert(drawswdg != NULL);
	m_draws_wdg = drawswdg;
	assert(dbmgr != NULL);
	m_dbmgr = dbmgr;

	SetHelpText(_T("draw3-base-hidegraphs"));

	wxBoxSizer * sizer1 = new wxBoxSizer(wxVERTICAL);

	int width = GetCheckBoxWidth();
	m_cb_l.resize(MIN_DRAWS_COUNT);

	for (size_t i = 0; i < MIN_DRAWS_COUNT; i++) {
		m_cb_l[i] = new wxCheckBox();
		m_cb_l[i]->Create(this, drawID_SELDRAWCB,
			wxString::Format(_T("%d."), i + 1),
			wxDefaultPosition, wxSize(width, -1), 0,
			SelectDrawValidator(m_draws_wdg, i, m_cb_l[i]));
		m_cb_l[i]->Enable(FALSE);

		m_cb_l[i]->SetToolTip(wxEmptyString);
		m_cb_l[i]->SetBackgroundColour(DRAW3_BG_COLOR);


		sizer1->Add(m_cb_l[i], 0, wxTOP | wxLEFT | wxRIGHT, 1);

#ifdef MINGW32
		m_cb_l[i]->Refresh();
#endif
	}

	SetSizer(sizer1);
	sizer1->SetSizeHints(this);
	sizer1->Layout();

	m_timer = new wxTimer(this, wxID_ANY);

}

int SelectDrawWidget::GetCheckBoxWidth() {
	int y, width;
	wxClientDC dc(this);
	wxFont f = GetFont();
	dc.SetFont(GetFont());
	dc.GetTextExtent(_T("0123456789012345678901234567"), &width, &y);
	return width;
}

void SelectDrawWidget::SetChecked(int idx, bool checked) {
	if (m_cb_l[idx]->IsEnabled())
		m_cb_l[idx]->SetValue(checked);
}

void SelectDrawWidget::InsertSomeDraws(size_t start, size_t count) {
	wxSizer *sizer = GetSizer();
	int width = GetCheckBoxWidth();
	ConfigNameHash& cnm = const_cast<ConfigNameHash&>(m_cfg->GetConfigTitles());

	for (size_t i = start; count; count--, i++) {
		if (i >= m_cb_l.size()) {
			i = m_cb_l.size();
			m_cb_l.push_back(new wxCheckBox());
			m_cb_l[i]->Create(this, drawID_SELDRAWCB,
				wxString::Format(_T("%d."), i + 1),
				wxDefaultPosition, wxSize(width, -1), 0,
				SelectDrawValidator(m_draws_wdg, i, m_cb_l[i]));
			m_cb_l[i]->Enable(FALSE);
			m_cb_l[i]->SetToolTip(wxEmptyString);
			m_cb_l[i]->SetBackgroundColour(DRAW3_BG_COLOR);

			sizer->Add(m_cb_l[i], 0, wxTOP | wxLEFT | wxRIGHT, 1);
		}

		Draw* draw = m_dc->GetDraw(i);
		DrawInfo* draw_info = draw->GetDrawInfo();
		wxString label;
		if (draw_info->IsValid()) {
			m_cb_l[i]->Enable(TRUE);
			m_cb_l[i]->SetValue(draw->GetEnable());
			m_cb_l[i]->SetToolTip(cnm[draw_info->GetBasePrefix()] + _T(":") + draw_info->GetParamName());

			label = wxString::Format(_T("%d."), draw->GetInitialDrawNo() + 1) + draw_info->GetName();
			if (draw_info->GetParam()->GetIPKParam()->GetPSC())
				label += _T("*");
			if (draw->GetBlocked())
				label.Replace(wxString::Format(_T("%d."), draw->GetInitialDrawNo() + 1), wxString::Format(_("%d.[B]"), i + 1), false);
		} else {
			label = _T("***");
		}
		m_cb_l[i]->SetLabel(label);

		wxValidator* validator = m_cb_l[i]->GetValidator();
		if (validator)
			dynamic_cast<SelectDrawValidator*>(validator)->Set(m_draws_wdg, i, m_cb_l[i]);
		else
			m_cb_l[i]->SetValidator(SelectDrawValidator(m_draws_wdg, i, m_cb_l[i]));

		m_cb_l[i]->SetBackgroundColour(draw_info->GetDrawColor());

#ifdef MINGW32
		m_cb_l[i]->Refresh();
#endif

	}

	if (m_cb_l.size() > MIN_DRAWS_COUNT)
		SetScrollRate(10, 10);
	else
		SetScrollRate(0, 0);

	sizer->Layout();
}

void
SelectDrawWidget::SetChanged(DrawsController *draws_controller)
{
	m_dc = draws_controller;
	DrawSet *selected_set = m_dc->GetSet();

	wxSizer *sizer = GetSizer();

	for (size_t i = selected_set->GetDraws()->size(); i < MIN_DRAWS_COUNT; i++) {
		m_cb_l[i]->SetLabel(wxString::Format(_T("%d."), i + 1));
		m_cb_l[i]->Enable(FALSE);
		m_cb_l[i]->SetValue(FALSE);
		m_cb_l[i]->SetToolTip(_T(" "));
		m_cb_l[i]->SetBackgroundColour(DRAW3_BG_COLOR);
	}

	for (size_t i = MIN_DRAWS_COUNT; i < m_cb_l.size(); i++) {
		sizer->Detach(m_cb_l[i]);
		m_cb_l[i]->Destroy();
	}

	m_cb_l.resize(MIN_DRAWS_COUNT);

	InsertSomeDraws(0, std::min(selected_set->GetDraws()->size(), MIN_DRAWS_COUNT));

	if (selected_set->GetDraws()->size() > MIN_DRAWS_COUNT && !m_timer->IsRunning())
		m_timer->Start(250, true);		

	FitInside();
}

void SelectDrawWidget::OnTimer(wxTimerEvent&) {
	DrawSet *selected_set = m_dc->GetSet();
	if (!selected_set)
		return;

	if (selected_set->GetDraws()->size() <= m_cb_l.size())
		return;

	InsertSomeDraws(m_cb_l.size(), std::min(MIN_DRAWS_COUNT, selected_set->GetDraws()->size() - m_cb_l.size()));

	if (selected_set->GetDraws()->size() > m_cb_l.size())
		m_timer->Start(250, true);		

	FitInside();
}

void
SelectDrawWidget::SetDrawEnable(int index, bool enable)
{
	if (index >= 0 && index < int(m_cb_l.size()));
		m_cb_l[index]->Enable(enable);
}

int SelectDrawWidget::GetClicked(wxCommandEvent &event) {
	wxMenu *menu = wxDynamicCast(event.GetEventObject(), wxMenu);
	assert(menu);

	wxCheckBox* cb = (wxCheckBox*) menu->GetClientData();

	size_t i = 0;

	for (; i < m_cb_l.size(); ++i)
		if (cb == m_cb_l[i])
			break;
	if (i == m_cb_l.size())
		return -1;
	else
		return i;
}

void
SelectDrawWidget::OnBlockCheck(wxCommandEvent &event) {

	int i = GetClicked(event);
	if (i == -1)
		return;
	m_draws_wdg->BlockDraw(i, event.IsChecked());
#ifdef __WXMSW__
	m_draws_wdg->SetFocus();
#endif
}

void
SelectDrawWidget::BlockedChanged(Draw *draw) {
	SetBlocked(draw->GetDrawNo(), draw->GetBlocked());
#ifdef __WXMSW__
	m_draws_wdg->SetFocus();
#endif
}

void
SelectDrawWidget::SetBlocked(int idx, bool blocked) {
	if (idx >= int(m_cb_l.size()))
		return;

	wxString label = m_cb_l[idx]->GetLabel();
	if (blocked) {
		int draw_no = m_dc->GetDraw(idx)->GetInitialDrawNo();
		label.Replace(wxString::Format(_T("%d."), draw_no + 1), wxString::Format(_("%d.[B]"), draw_no + 1), false);
	} else {
		label.Replace(_T("[B]"), _T(""), false);
	}
	m_cb_l[idx]->SetLabel(label);

}

void
SelectDrawWidget::OnPSC(wxCommandEvent &event) {
	int i = GetClicked(event);
	if (i == -1)
		return;
	DrawInfo* info = m_draws_wdg->GetDrawInfo(i);
	m_cfg->EditPSC(info->GetBasePrefix(), info->GetParamName());
}

void SelectDrawWidget::OnEditParam(wxCommandEvent &event) {
	int i = GetClicked(event);
	if (i == -1)
		return;

	DrawInfo *d = m_draws_wdg->GetDrawInfo(i);

	DefinedParam *dp = dynamic_cast<DefinedParam*>(d->GetParam());
	if (dp == NULL)
		return;

	wxWindow *w = this;
	while (!w->IsTopLevel())
		w = w->GetParent();

	ParamEdit* pe = new ParamEdit(w, m_cfg, m_dbmgr, m_remarks_handler);
	pe->SetCurrentConfig(d->GetBasePrefix());
	pe->Edit(dp);
	delete pe;

}

void SelectDrawWidget::OnCopyParamName(wxCommandEvent &event) {
	int i = GetClicked(event);
	if (-1 == i)
		return;

	DrawInfo *d = m_draws_wdg->GetDrawInfo(i);
	m_draws_wdg->CopyParamName(d);
}

void SelectDrawWidget::OnDocs(wxCommandEvent &event) {
	int i = GetClicked(event);
	if (i == -1)
		return;

	OpenParameterDoc(i);
}

void SelectDrawWidget::ShowDefinedParamDoc(DefinedParam *param) {
	ParamEdit* pe = new ParamEdit(this, m_cfg, m_dbmgr, m_remarks_handler);
	pe->View(param);
	delete pe;
}

void SelectDrawWidget::GoToWWWDocumentation(DrawInfo *d) {
	if (d->IsValid() == false)
		return;
	TParam *p = d->GetParam()->GetIPKParam();
	TSzarpConfig* sc = p->GetSzarpConfig();

	wxString link = sc->GetDocumentationBaseURL();

	if (link.IsEmpty())
		link = _T("http://www.szarp.com");

	link += _T("/cgi-bin/param_docs.py?");
	link << _T("prefix=") << d->GetBasePrefix().c_str();

	link << _T("&param=") << encode_string(p->GetName().c_str());

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
		wxMessageBox(_("I was not able to start default browser"), _("Error"), wxICON_ERROR | wxOK, this);


}

DrawInfoList SelectDrawWidget::GetDrawInfoList() {
	DrawInfoList draw_info;

	DrawSet *selected_set = m_dc->GetSet();

	for (size_t i = 0; i < selected_set->GetDraws()->size(); ++i)
		if (m_cb_l[i]->GetValue()) {
			Draw* draw = m_dc->GetDraw(i);
			draw_info.push_back(draw->GetDrawInfo());
		}

	return draw_info;
}

void SelectDrawWidget::OpenParameterDoc(int i) {
	DrawInfo* d;
	if (i == -1)
		d = m_draws_wdg->GetCurrentDrawInfo();
	else
		d = m_draws_wdg->GetDrawInfo(i);

	if (d == NULL)
		return;

	if (DefinedParam* dp = dynamic_cast<DefinedParam*>(d->GetParam()))
		ShowDefinedParamDoc(dp);
	else
		GoToWWWDocumentation(d);
#ifdef __WXMSW__
	m_draws_wdg->SetFocus();
#endif
}

void SelectDrawWidget::NoData(Draw *d) {
	m_cb_l[d->GetDrawNo()]->Enable(!d->GetNoData());
}

void SelectDrawWidget::EnableChanged(Draw *draw) {
	SetChecked(draw->GetDrawNo(), draw->GetEnable());
}

void SelectDrawWidget::PeriodChanged(Draw *draw, PeriodType period) {
	if (draw->GetSelected())
		for (size_t i = 0; i < draw->GetDrawsController()->GetDrawsCount(); i++)
			m_cb_l[i]->Enable(true);
}

void SelectDrawWidget::DrawsSorted(DrawsController *controller) {
	SetChanged(controller);
}

void SelectDrawWidget::DrawInfoChanged(Draw *draw) {
	if (draw->GetSelected() == false)
		return;

	SetChanged(draw->GetDrawsController());
}
