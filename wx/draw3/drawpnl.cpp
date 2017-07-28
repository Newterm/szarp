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
 * Single panel for all program widgets.
 */

#include "szhlpctrl.h"

#include "ids.h"
#include "classes.h"

#include "drawtime.h"
#include "coobs.h"
#include "dbinquirer.h"
#include "drawobs.h"
#include "database.h"
#include "drawsctrl.h"
#include "summwin.h"
#include "cfgmgr.h"
#include "disptime.h"
#include "drawswdg.h"
#include "infowdg.h"
#include "seldraw.h"
#include "selset.h"
#include "timewdg.h"
#include "drawtb.h"
#include "drawdnd.h"
#include "incsearch.h"
#include "pscgui.h"
#include "dbmgr.h"
#include "drawpick.h"
#include "defcfg.h"
#include "piewin.h"
#include "relwin.h"
#include "wxgraphs.h"
#include "glgraphs.h"
#include "gcdcgraphs.h"
#include "szframe.h"
#include "drawfrm.h"
#include "remarks.h"
#include "remarksdialog.h"
#include "remarksfetcher.h"
#include "drawpnl.h"
#include "drawapp.h"
#include "drawtreedialog.h"
#include "paredit.h"
#include "draw.h"

#include <wx/xrc/xmlres.h>

#ifndef MINGW32
extern "C" void gtk_window_set_accept_focus(void *window, int setting);
#endif

/**
 * Handler for keyboard event. It is added to all widgets in panel with
 * PushEventHandler method.
 */
class DrawPanelKeyboardHandler : public wxEvtHandler {
      public:
	/* @param panel 'parent' panel
	 * @param id identifier for debugging purposes
	 */
	DrawPanelKeyboardHandler(DrawPanel * panel, wxString id = _T("no_id"));
	~DrawPanelKeyboardHandler();
	/** Event handler - handles keyboard events. */
	void OnChar(wxKeyEvent & event);
	void OnKeyUp(wxKeyEvent & event);
	bool OnKeyDown(wxKeyEvent & event);
	virtual bool ProcessEvent(wxEvent &event) {
		bool handled = false;
		if (event.GetEventType() == wxEVT_KEY_DOWN) {
			handled = OnKeyDown((wxKeyEvent&)event);
		}
		return (handled || wxEvtHandler::ProcessEvent(event));
	}

      protected:
	DECLARE_EVENT_TABLE()

	DrawPanel *panel;	/**< pointer to panel */
	wxString id;		/**< debug identifier */
};

BEGIN_EVENT_TABLE(DrawPanelKeyboardHandler, wxEvtHandler)
    LOG_EVT_CHAR(DrawPanelKeyboardHandler , OnChar, "drawpnl:key_char" )
    //EVT_KEY_DOWN(DrawPanelKeyboardHandler::OnKeyDown)
    EVT_KEY_UP(DrawPanelKeyboardHandler::OnKeyUp)
END_EVENT_TABLE()

    DrawPanelKeyboardHandler::DrawPanelKeyboardHandler(DrawPanel * panel,
						       wxString id)
:  wxEvtHandler()
{
	this->id = id;
	assert(panel != NULL);
	this->panel = panel;
}

DrawPanelKeyboardHandler::~DrawPanelKeyboardHandler()
{
}

void DrawPanelKeyboardHandler::OnChar(wxKeyEvent & event)
{
    wxLogInfo(_T("DEBUG: DrawPanelKeyboardEvent::OnChar [%s] key code %d"),
	    id.c_str(), event.GetKeyCode());

    switch (event.GetKeyCode()) {
	case WXK_DOWN :
	    panel->dw->SelectNextDraw();
	    break;
	case WXK_UP :
	    panel->dw->SelectPreviousDraw();
	    break;
	default :
	    event.Skip();
	    break;
    }
}

void DrawPanelKeyboardHandler::OnKeyUp(wxKeyEvent & event)
{
	wxLogInfo(_T("DEBUG: DrawPanelKeyboardEvent::OnKeyUp [%s] key code %d"),
		  id.c_str(), event.GetKeyCode());

	switch (event.GetKeyCode()) {
	case WXK_LEFT:
	case WXK_RIGHT:
	case WXK_HOME:
	case WXK_END:
	case WXK_PAGEUP:
	case WXK_PAGEDOWN:
		panel->dw->SetKeyboardAction(NONE);
		break;
	case '?':
		panel->dg->StopDrawingParamName();
		break;
	case '/':
		if (event.ShiftDown())
			panel->dg->StopDrawingParamName();
		else
			event.Skip();
	default:
		event.Skip();
		break;
	}
}

bool DrawPanelKeyboardHandler::OnKeyDown(wxKeyEvent & event)
{
	wxLogInfo(_T
		  ("DEBUG: DrawPanelKeyboardEvent::OnKeyDown [%s] key code %d"),
		  id.c_str(), event.GetKeyCode());
	switch (event.GetKeyCode()) {
	case WXK_LEFT:
		if(event.ShiftDown() && (panel->tw->GetSelection() == PERIOD_T_DAY || panel->tw->GetSelection() == PERIOD_T_30MINUTE))
			panel->dw->SetKeyboardAction(CURSOR_LONG_LEFT_KB);
		else
			panel->dw->SetKeyboardAction(CURSOR_LEFT_KB);
		break;
	case WXK_RIGHT:
		if(event.ShiftDown() && (panel->tw->GetSelection() == PERIOD_T_DAY || panel->tw->GetSelection() == PERIOD_T_30MINUTE))
			panel->dw->SetKeyboardAction(CURSOR_LONG_RIGHT_KB);
		else
			panel->dw->SetKeyboardAction(CURSOR_RIGHT_KB);
		break;
	case WXK_HOME:
		panel->dw->SetKeyboardAction(CURSOR_HOME_KB);
		break;
	case WXK_PAGEUP:
		if (event.ControlDown())
			panel->df->SelectPreviousTab();
		else
			panel->dw->SetKeyboardAction(SCREEN_LEFT_KB);
		break;
	case WXK_PAGEDOWN:
		if (event.ControlDown())
			panel->df->SelectNextTab();
		else
			panel->dw->SetKeyboardAction(SCREEN_RIGHT_KB);
		break;
	// enable/disable split cursor
	case WXK_BACK: {
		wxCommandEvent e;
		panel->ToggleSplitCursor(e);
		break;
	}
	case 'C':
		if (event.ControlDown())
			panel->dw->CopyToClipboard();
		break;
	case 'V':
		if (event.ControlDown())
			panel->dw->PasteFromClipboard();
		break;
	case 'O':
		if(event.AltDown())
			panel->OnJumpToDate();
		break;
	case 'B':
		if (event.ControlDown())
			panel->dw->SwitchCurrentDrawBlock();
		else
			return false;
		break;
	case 'H':
		if (event.ControlDown())
			panel->sw->OpenParameterDoc();
		else
			return false;
		break;
	case '/':
		if (event.ShiftDown())
			panel->dg->StartDrawingParamName();
		else
			return false;
		break;
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case '0':
		if (event.AltDown()) {
			size_t idx = event.GetKeyCode() - '1';
			size_t curr = panel->dw->GetSelectedDrawIndex();
			 // cannot disable not existing draw or current draw
			 if (idx >= panel->dw->GetDrawsCount() || curr == idx) {
				wxBell();
				break;
			}
			if (panel->dw->IsDrawEnabled(idx)) {
				if (panel->dw->SetDrawDisable(idx));
					panel->sw->SetChecked(idx, false);
			} else  {
				panel->dw->SetDrawEnable(idx);
				panel->sw->SetChecked(idx, true);
			}
		} else {
			return false;
		}
		break;
	case '[':
		{
			int index = panel->dw->GetSelectedDrawIndex();
			if (index >= 0) {
    				DrawSet *selected_set = panel->dw->GetDrawsController()->GetSet();
				for (size_t i = 0; i < selected_set->GetDraws()->size(); i++) {
					if (i == size_t(index))
						continue;
					panel->dw->SetDrawDisable(i);
				}
			}
		}
		break;
	case ']':
		{
    			DrawSet *selected_set = panel->dw->GetDrawsController()->GetSet();
			for (size_t i = 0; i < selected_set->GetDraws()->size(); i++) {
				panel->dw->SetDrawEnable(i);
			}
		}
		break;
	case WXK_F1:
		if (event.ShiftDown()) {
					printf("IKE!\n");
		}
		break;
	case WXK_F12:
		panel->df->ToggleMenuBarVisbility();
		break;
	default:
		event.Skip();
		return false;
		break;
	}
	return true;
}

DrawPanel::DrawPanel(DatabaseManager* _db_mgr, ConfigManager * _cfg, RemarksHandler * _rh,
		wxMenuBar *_menu_bar, wxString _prefix, const wxString& set, PeriodType pt,  time_t time,
		wxWindow * parent, wxWindowID id, DrawFrame *_df, int selected_draw)
	:  wxPanel(parent, id, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS),
	df(_df), iw(NULL), dw(NULL), dtw(NULL), ssw(NULL), sw(NULL), tw(NULL),
	dinc(NULL), sinc(NULL), db_mgr(_db_mgr), cfg(_cfg),
	prefix(_prefix), smw(NULL), rh(_rh), rmf(NULL), dtd(NULL), pw(NULL), realized(false), ee(NULL)
{
#ifdef WXAUI_IN_PANEL
	am.SetManagedWindow(this);
#endif
	menu_bar = _menu_bar;

	active = false;

	CreateChildren(set, pt, time, selected_draw);

	rw_show = pw_show = smw_show = false;

}

void DrawPanel::CreateChildren(const wxString& set, PeriodType pt, time_t time, int selected_draw)
{
	if (realized)
		return;

	realized = true;
	filter_popup_menu = ((wxMenuBar*) wxXmlResource::Get()->LoadObject(this,_T("filter_context"),_T("wxMenuBar")))->GetMenu(0);

#ifdef WXAUI_IN_PANEL
	hpanel = new wxPanel(this);
	wxBoxSizer *hsizer = new wxBoxSizer(wxHORIZONTAL);

	wxPanel *vpanel = new wxPanel(this);
#endif

	wxBoxSizer *vsizer = new wxBoxSizer(wxVERTICAL);

	/* calculate right panel size */
	wxClientDC dc(this);
	dc.SetFont(GetFont());
	int y, width;
	dc.GetTextExtent(_T("012345678901234567890123456789012345"), &width,
			 &y);

	/* create widgets */
	iw = new InfoWidget(
#ifdef WXAUI_IN_PANEL
			hpanel
#else
			this
#endif
			);


	smw = new SummaryWindow(this, this);
	pw = new PieWindow(this, this);
	rw = new RelWindow(this, this);

#ifndef MINGW32
	gtk_window_set_accept_focus(smw->GetHandle(), 0);
	gtk_window_set_accept_focus(pw->GetHandle(), 0);
	gtk_window_set_accept_focus(rw->GetHandle(), 0);
#endif

#ifdef MINGW32
	wxString style = _T("GCDC");
#else
	wxString style = wxConfig::Get()->Read(_T("GRAPHS_VIEW"), _T("GCDC"));
#endif

#ifdef HAVE_GLCANVAS
#ifdef HAVE_FTGL
	if (style == _T("3D") && wxGetApp().GLWorks()) {
		dg = new GLGraphs(this, cfg);
		GLGraphs *glg = new GLGraphs(this, cfg);
		dg = glg;
	}
	else
#endif
#endif
	if (style == _T("GCDC"))
		dg = new GCDCGraphs(this, cfg);
	else
		dg = new WxGraphs(this, cfg);

	tb = new DrawToolBar(this);

	rmf = new RemarksFetcher(rh, df);

	dw = new DrawsWidget(this, cfg, db_mgr, dg, rmf);

	dg->SetDrawsWidget(dw);

	dtw = new DisplayTimeWidget(
#ifdef WXAUI_IN_PANEL
			vpanel
#else
			this
#endif
			);
	ssw = new SelectSetWidget(cfg,
#ifdef WXAUI_IN_PANEL
			hpanel,
#else
			this,
#endif
			drawID_SELSET,
			width);

	sw = new SelectDrawWidget(cfg, db_mgr, dw, rh,
#ifdef WXAUI_IN_PANEL
			vpanel,
#else
			this,
#endif
			drawID_SELDRAW);

        tw = new TimeWidget(
#ifdef WXAUI_IN_PANEL
			vpanel,
#else
			this,
#endif
			dw,
			pt);


	/* Hide/Show interface according to state of checkboxes */
	if(menu_bar->FindItem(XRCID("ShowInterface"))->IsChecked()) {
		tw->Show(menu_bar->FindItem(XRCID("ShowAverage"))->IsChecked());
	} else {
		tw->Show(false);
		dtw->Show(false);
		sw->Show(false);
		iw->Show(false);
		ssw->Show(false);
	}

	/* add keyboard event handlers */
	DrawPanelKeyboardHandler *eh;
	eh = new DrawPanelKeyboardHandler(this, _T("dg"));
	(dynamic_cast<wxWindow*>(dg))->PushEventHandler(eh);
#ifdef WXAUI_IN_PANEL
	hsizer->Add(iw, 1, wxALL | wxEXPAND, 10);
	hsizer->Add(ssw, 0, wxALL | wxALIGN_RIGHT, 10);
	hpanel->SetSizer(hsizer);
	hsizer->SetSizeHints(hpanel);
#endif

	vsizer->Add(dtw, 0, wxALL | wxEXPAND);
	vsizer->Add(sw, 0, wxALL | wxEXPAND, 10);
	vsizer->Add(tw, 1, wxALL | wxEXPAND, 2);

#ifdef WXAUI_IN_PANEL
	vpanel->SetSizer(vsizer);
	vsizer->SetSizeHints(vpanel);

	wxSize size1 = hsizer->CalcMin();
	size1.SetWidth(-1);


	wxSize size2 = vsizer->CalcMin();
	size2.SetHeight(-1);
	am.AddPane(dg, wxAuiPaneInfo().Centre().CentrePane());
	am.AddPane(vpanel, wxAuiPaneInfo().Right().Layer(1).Resizable().MinSize(size2).CloseButton(false));
	am.AddPane(hpanel, wxAuiPaneInfo().Bottom().Layer(8).Resizable().MinSize(size1).CloseButton(false).BottomDockable().TopDockable().PaneBorder(false).Gripper(true).CaptionVisible(false));
	am.AddPane(tb, wxAuiPaneInfo().Bottom().Layer(9).ToolbarPane());

	am.Update();
#else
	wxSizer *sizer_1_1 = new wxBoxSizer(wxHORIZONTAL);

	sizer_1_1->Add(dynamic_cast<wxWindow*>(dg), 1, wxEXPAND);
	sizer_1_1->Add(vsizer, 0, wxEXPAND);

	wxSizer *sizer_1_2 = new wxBoxSizer(wxHORIZONTAL);
	sizer_1_2->Add(iw, 1, wxEXPAND, 10);
	sizer_1_2->Add(ssw, 0, wxALIGN_RIGHT, 10);

	wxSizer *sizer_1 = new wxBoxSizer(wxVERTICAL);
	sizer_1->Add(sizer_1_1, 1, wxEXPAND);
	sizer_1->Add(sizer_1_2, 0, wxEXPAND | wxALL, 10);
	sizer_1->Add(tb, 0, wxEXPAND);

	SetSizer(sizer_1);
	sizer_1->SetSizeHints(this);
#endif

	dw->AttachObserver(dg);
	dw->AttachObserver(iw);
	dw->AttachObserver(sw);
	dw->AttachObserver(ssw);
	dw->AttachObserver(smw);
	dw->AttachObserver(tw);
	dw->AttachObserver(pw);
	dw->AttachObserver(rw);
	dw->AttachObserver(rmf);
	dw->AttachObserver(this);

	dw->SetSet(set, prefix, time, pt, selected_draw);

}

void DrawPanel::OnJumpToDate()
{
	return dw->OnJumpToDate();
}

void DrawPanel::SetFocus() {
	dg->SetFocus();
}

DrawPanel::~DrawPanel()
{
	delete rmf;

	/* Remove event handlers */
	dynamic_cast<wxWindow*>(dg)->PopEventHandler(TRUE);
#ifdef WXAUI_IN_PANEL
	am.UnInit();
#endif

	if (dinc != NULL)
		dinc->Destroy();
	if (sinc != NULL)
		sinc->Destroy();
	if (dtd != NULL)
		dtd->Destroy();
	if (ee != NULL)
		ee->Destroy();
	smw->Destroy();
	rw->Destroy();
	pw->Destroy();

	delete dw;

}

void DrawPanel::OnRefresh(wxCommandEvent & evt) {
	dw->RefreshData(false);
}

void DrawPanel::OnShowAverage(wxCommandEvent &evt)
{
	if (tw) {
		bool is_checked = menu_bar->FindItem(XRCID("ShowAverage"))->IsChecked();

		wxConfigBase *cfg = wxConfig::Get();
		cfg->Write(_T("HIDE_AVERAGE"), !is_checked);
		cfg->Flush();

		if (menu_bar->FindItem(XRCID("ShowInterface"))->IsChecked()) {
			/* Create size event with size before resize */
			wxSizeEvent se(GetSize());

			tw->Show(is_checked);
			Fit();
			GetSizer()->SetSizeHints(this);

			/* Inform window about resize */
			wxPostEvent(GetParent(),se);
			/* This is needen in case of wxAuiNotebook (draw panel tabs) */
			wxPostEvent(GetParent()->GetParent(),se);
		}
	}
}

void DrawPanel::OnShowInterface(wxCommandEvent &evt)
{
	if (tw && iw && ssw && dtw && sw) {
		bool is_checked = menu_bar->FindItem(XRCID("ShowInterface"))->IsChecked();

		wxConfigBase *cfg = wxConfig::Get();
		cfg->Write(_T("HIDE_INTERFACE"), !is_checked);
		cfg->Flush();

		/* Create size event with size before resize */
		wxSizeEvent se(GetSize());

		if (is_checked) {
			tw->Show(menu_bar->FindItem(XRCID("ShowAverage"))->IsChecked());
		} else {
			tw->Show(is_checked);
		}

		menu_bar->FindItem(XRCID("ShowAverage"))->Enable(is_checked);

		iw->Show(is_checked);
		ssw->Show(is_checked);
		dtw->Show(is_checked);
		sw->Show(is_checked);

		Fit();
		GetSizer()->SetSizeHints(this);

		/* Inform window about resize */
		wxPostEvent(GetParent(),se);
		/* This is needen in case of wxAuiNotebook (draw panel tabs) */
		wxPostEvent(GetParent()->GetParent(),se);
	}
}

void DrawPanel::ClearCache() {
	dw->ClearCache();
	dw->RefreshData(false);
}

void DrawPanel::OnFind(wxCommandEvent & evt) {
	StartDrawSearch();
}

void DrawPanel::GetDisplayedDrawInfo(DrawInfo **di, wxDateTime& time) {
	*di = dw->GetCurrentDrawInfo();
	time = wxDateTime(dw->GetCurrentTime());
}

void DrawPanel::StartDrawSearch()
{
	assert(cfg != NULL);
	assert(prefix != wxEmptyString);

	wxWindow *tlw = GetParent();
	while (!tlw->IsTopLevel())
		tlw = tlw->GetParent();

	if (dinc == NULL) {
		wxWindow *parent = GetParent();
		wxFrame *frame = NULL;

		while (parent && (frame = wxDynamicCast(parent, wxFrame)) == NULL)
			parent = parent->GetParent();

		dinc = new IncSearch(cfg, rh, GetConfigName(), (wxFrame *) tlw, incsearch_DIALOG, _("Find"), false, true, false);
	}

	DrawInfo* di = dw->GetCurrentDrawInfo();
	DrawSet* set = dw->GetCurrentDrawSet();
	if (di)
		dinc->StartWith(set, di);
	int ret = dinc->ShowModal();
	if (ret != wxID_OK)
		return;

	long int prev = -1;
	di = dinc->GetDrawInfo(&prev, &set);
	if (di == NULL)
		return;

	dw->SetSet(set, di);

}

void DrawPanel::StartSetSearch() {
	assert(cfg != NULL);
	assert(prefix != wxEmptyString);

	wxWindow *tlw = GetParent();
	while (!tlw->IsTopLevel())
		tlw = tlw->GetParent();

	if (sinc == NULL) {
		wxWindow *parent = GetParent();
		wxFrame *frame = NULL;

		while (parent && (frame = wxDynamicCast(parent, wxFrame)) == NULL)
			parent = parent->GetParent();

		sinc = new IncSearch(cfg, rh, GetConfigName(), (wxFrame *) tlw, incsearch_DIALOG, _("Find"), true, true, false);
	}

	sinc->StartWith(dw->GetCurrentDrawSet());
	int ret = sinc->ShowModal();
	if (ret != wxID_OK)
		return;

	DrawSet *s = sinc->GetSelectedSet();
	if (s)
		dw->SetSet(s);
}

void DrawPanel::StartPSC()
{
	cfg->EditPSC(prefix);
}

void DrawPanel::ShowSummaryWindow(bool show) {
#ifndef MINGW32
	smw->Show(show);
	if (show)
		smw->Raise();
#else
	if (show) {
		smw->Show(show);
		GetParent()->Raise();
	} else
		smw->Show(show);
#endif
	if (active) {
		smw_show = show;
		wxMenuItem *item = menu_bar->FindItem(XRCID("Summary"));
		item->Check(smw->IsShown());
	}

}

void DrawPanel::OnSummaryWindow(wxCommandEvent & event)
{
	ShowSummaryWindow(!smw->IsShown());
}

void DrawPanel::ShowPieWindow(bool show) {
#ifndef MINGW32
	pw->Show(show);
	if (show)
		pw->Raise();
#else
	if (show) {
		pw->Show(show);
		GetParent()->Raise();
	} else
		pw->Show(show);
#endif
	if (active) {
		pw_show = show;
		wxMenuItem *item = menu_bar->FindItem(XRCID("Pie"));
		item->Check(pw->IsShown());
	}
}


void DrawPanel::ShowRelWindow(bool show) {
#ifndef MINGW32
	rw->Show(show);
	if (show)
		rw->Raise();
#else
	if (show) {
		rw->Show(show);
		GetParent()->Raise();
	} else
		rw->Show(show);
#endif
	if (active) {
		rw_show = show;
		wxMenuItem *item = menu_bar->FindItem(XRCID("Ratio"));
		item->Check(rw->IsShown());
	}
}




wxString DrawPanel::GetPrefix()
{
	return prefix;
}

wxString DrawPanel::GetConfigName()
{
	return cfg->GetConfigByPrefix(prefix)->GetID();
}

SummaryWindow *DrawPanel::GetSummaryWindow()
{
	return smw;
}

PeriodType DrawPanel::SetPeriod(PeriodType pt) {
	dw->SetPeriod(pt);
	return pt;
}

void DrawPanel::SetLatestDataFollow(bool follow) {
	dw->GetDrawsController()->SetFollowLatestData(follow);
}

void DrawPanel::ToggleSplitCursor(wxCommandEvent& WXUNUSED(event)) {
	bool is_double = dw->ToggleSplitCursor();
	ToggleSplitCursorIcon(is_double);

	GetSizer()->Layout();

#ifdef WXAUI_IN_PANEL
	wxAuiPaneInfo& info = am.GetPane(hpanel);
	wxSize size = hpanel->GetSizer()->CalcMin();

	size.SetWidth(-1);
	info.MinSize(size);
	if (!is_double) {
		info.MaxSize(size);
		info.MinSize(size);
	}
	am.Update();
#endif
}

void DrawPanel::ToggleSplitCursorIcon(bool is_double)
{
	if(is_double) {
		tb->DoubleCursorToolCheck();
		menu_bar->FindItem(XRCID("SplitCursor"))->Check(true);
	}
	else {
		tb->DoubleCursorToolUncheck();
		menu_bar->FindItem(XRCID("SplitCursor"))->Check(false);
	}
}

void DrawPanel::SelectSet(DrawSet *set) {
	dw->SetSet(set);
}

void DrawPanel::Print(bool preview) {
	dw->Print(preview);
}

DrawsController* DrawPanel::GetDrawsController() {
	return dw->GetDrawsController();
}

void DrawPanel::DrawInfoChanged(Draw *d) {
	if (d->GetSelected()) {
		prefix = d->GetDrawsController()->GetSet()->GetDrawsSets()->GetPrefix();
		tb->DoubleCursorToolUncheck();
		d->StopDoubleCursor();
		if (active) {
			menu_bar->Enable(XRCID("EditSet"),  IsUserDefined());
			menu_bar->Enable(XRCID("DelSet"),  IsUserDefined());
			menu_bar->Enable(XRCID("ExportSet"),  IsUserDefined());
			menu_bar->Check(XRCID("SplitCursor"), false);
			df->SetTitle(GetConfigName(), GetPrefix());
			//possibly
			PeriodChanged(d, d->GetPeriod());
		}
		df->UpdatePanelName(this);

	}
}

void DrawPanel::PeriodChanged(Draw *d, PeriodType pt) {
	if (d->GetSelected() == false)
		return;

	tb->DoubleCursorToolUncheck();
	menu_bar->Check(XRCID("SplitCursor"), false);

	if (active == false)
		return;
	int id = 0;
	switch (pt) {
		case PERIOD_T_DECADE:
			id = XRCID("DECADE_RADIO");
			break;
		case PERIOD_T_YEAR:
			id = XRCID("YEAR_RADIO");
			break;
		case PERIOD_T_MONTH:
			id = XRCID("MONTH_RADIO");
			break;
		case PERIOD_T_WEEK:
			id = XRCID("WEEK_RADIO");
			break;
		case PERIOD_T_DAY:
			id = XRCID("DAY_RADIO");
			break;
		case PERIOD_T_30MINUTE:
			id = XRCID("30MINUTE_RADIO");
			break;
		case PERIOD_T_5MINUTE:
			id = XRCID("5MINUTE_RADIO");
			break;
		case PERIOD_T_MINUTE:
			id = XRCID("MINUTE_RADIO");
			break;
		case PERIOD_T_30SEC:
			id = XRCID("30SEC_RADIO");
			break;
		case PERIOD_T_SEASON:
			id = XRCID("SEASON_RADIO");
			break;
		case PERIOD_T_LAST:
		case PERIOD_T_OTHER:
			assert(false);
	}
	wxMenuItem* radio = menu_bar->FindItem(id);
	if (!radio->IsChecked())
		radio->Check(true);
}

void DrawPanel::UpdateFilterMenuItem(int filter) {
	wxMenuItem *main_menu_item = NULL;

	switch (filter) {
		case 0:
        		main_menu_item = menu_bar->FindItem(XRCID("F0"));
			break;
		case 1:
        		main_menu_item = menu_bar->FindItem(XRCID("F1"));
			break;
		case 2:
			main_menu_item = menu_bar->FindItem(XRCID("F2"));
			break;
		case 3:
	        	main_menu_item = menu_bar->FindItem(XRCID("F3"));
			break;
		case 4:
	        	main_menu_item = menu_bar->FindItem(XRCID("F4"));
			break;
		case 5:
	        	main_menu_item = menu_bar->FindItem(XRCID("F5"));
			break;
	}

	assert(main_menu_item);
	main_menu_item->Check(true);

}

void DrawPanel::FilterChanged(DrawsController *d) {
	int filter = d->GetFilter();
	wxMenuItem *popup_menu_item = NULL;

	switch (filter) {
		case 0:
			popup_menu_item = filter_popup_menu->FindItem(XRCID("ContextF0"));
			break;
		case 1:
			popup_menu_item = filter_popup_menu->FindItem(XRCID("ContextF1"));
			break;
		case 2:
			popup_menu_item = filter_popup_menu->FindItem(XRCID("ContextF2"));
			break;
		case 3:
			popup_menu_item = filter_popup_menu->FindItem(XRCID("ContextF3"));
			break;
		case 4:
			popup_menu_item = filter_popup_menu->FindItem(XRCID("ContextF4"));
			break;
		case 5:
			popup_menu_item = filter_popup_menu->FindItem(XRCID("ContextF5"));
			break;
	}

	tb->SetFilterToolIcon(filter);

	if (active) {
		assert(popup_menu_item);
		popup_menu_item->Check(true);
	}

}

void DrawPanel::OnFilterChange(wxCommandEvent &event) {
	if (event.GetId() == wxID_ANY)
		return;

	int id = event.GetId();
	int filter = 0;
	if(id==XRCID("F0") || id==XRCID("ContextF0"))
		filter = 0;
	else if(id==XRCID("F1") || id==XRCID("ContextF1"))
		filter = 1;
	else if(id==XRCID("F2") || id==XRCID("ContextF2"))
		filter = 2;
	else if(id==XRCID("F3") || id==XRCID("ContextF3"))
		filter = 3;
	else if(id==XRCID("F4") || id==XRCID("ContextF4"))
		filter = 4;
	else if(id==XRCID("F5") || id==XRCID("ContextF5"))
		filter = 5;
	else
		assert(false);

	dw->SetFilter(filter);

}

void DrawPanel::OnDrawTree(wxCommandEvent&) {
	if (dtd == NULL)
		dtd = new DrawTreeDialog(this, cfg);
	dtd->SetSelectedSet(GetSelectedSet());
	if (dtd->ShowModal() == wxID_OK)
		SelectSet(dtd->GetSelectedSet());
}

bool DrawPanel::IsUserDefined() {
	DrawSet *sset = GetSelectedSet();
	return dynamic_cast<DefinedDrawSet*>(sset) != NULL;
}

void DrawPanel::OnToolFilterMenu(wxCommandEvent &event) {
	PopupMenu(filter_popup_menu);
}

DrawSet* DrawPanel::GetSelectedSet() {
	return dw->GetCurrentDrawSet();
}

DrawInfoList DrawPanel::GetDrawInfoList() {
	DrawInfoList dil = sw->GetDrawInfoList();
	DrawInfo* di;
	wxDateTime wdt;
	GetDisplayedDrawInfo( &di , wdt );
	dil.SetSelectedDraw( di );
	dil.SetStatsInterval(dw->GetDrawsController()->GetStatsInterval());
	dil.SetDoubleCursor(dw->GetDrawsController()->GetDoubleCursor());
	return dil;
}

void DrawPanel::Copy() {
	dw->CopyToClipboard();
}

void DrawPanel::Paste() {
	dw->PasteFromClipboard();
}

void DrawPanel::OnCopyParamName(wxCommandEvent &event) {
	dw->OnCopyParamName(event);
}

size_t DrawPanel::GetNumberOfUnits() {
	return dw->GetNumberOfUnits();
}

void DrawPanel::SetNumberOfUnits(size_t number_of_units) {
	return dw->SetNumberOfUnits(number_of_units);
}

PeriodType DrawPanel::GetPeriod() {
	return (PeriodType)tw->GetSelection();
}

DTime DrawPanel::GetBeginCurrentTime() {
	return dw->GetSelectedDraw()->GetStartTime();
}

DTime DrawPanel::GetEndCurrentTime() {
	return dw->GetSelectedDraw()->GetLastTime();
}

TimeInfo DrawPanel::GetCurrentTimeInfo() {
	return dw->GetSelectedDraw()->GetTimeInfo();
}

void DrawPanel::ShowRemarks() {
	rmf->ShowRemarks();
}

void DrawPanel::GoToLatestDate() {
	dw->GetDrawsController()->GoToLatestDate();
}

void DrawPanel::MoveCursorEnd() {
	dw->GetDrawsController()->MoveCursorEnd();
}

void DrawPanel::SetActive(bool _active) {
	active = _active;

	if (smw_show)
		ShowSummaryWindow(active);
	if (rw_show)
		ShowRelWindow(active);
	if (pw_show)
		ShowPieWindow(active);

	if (active) {
		DrawsController *dc = dw->GetSelectedDraw()->GetDrawsController();

		int filter = dc->GetFilter();
		UpdateFilterMenuItem(filter);

		menu_bar->Enable(XRCID("EditSet"),  IsUserDefined());
		menu_bar->Enable(XRCID("DelSet"),  IsUserDefined());
		menu_bar->Enable(XRCID("ExportSet"),  IsUserDefined());

		if (cfg->IsPSC(prefix))
			menu_bar->FindItem(XRCID("SetParams"))->Enable(true);
		else
			menu_bar->FindItem(XRCID("SetParams"))->Enable(false);

		wxMenuItem *item = menu_bar->FindItem(XRCID("Summary"));
		item->Check(smw->IsShown());

		menu_bar->FindItem(XRCID("SplitCursor"))->Check(dc->GetDoubleCursor());

		wxMenuItem *pmi = NULL;
		switch (dc->GetPeriod()) {
			case PERIOD_T_DECADE:
				pmi = menu_bar->FindItem(XRCID("DECADE_RADIO"));
				break;
			case PERIOD_T_YEAR:
				pmi = menu_bar->FindItem(XRCID("YEAR_RADIO"));
				break;
			case PERIOD_T_MONTH:
				pmi = menu_bar->FindItem(XRCID("MONTH_RADIO"));
				break;
			case PERIOD_T_WEEK:
				pmi = menu_bar->FindItem(XRCID("WEEK_RADIO"));
				break;
			case PERIOD_T_DAY:
				pmi = menu_bar->FindItem(XRCID("DAY_RADIO"));
				break;
			case PERIOD_T_30MINUTE:
				pmi = menu_bar->FindItem(XRCID("30MINUTE_RADIO"));
				break;
			case PERIOD_T_5MINUTE:
				pmi = menu_bar->FindItem(XRCID("5MINUTE_RADIO"));
				break;
			case PERIOD_T_MINUTE:
				pmi = menu_bar->FindItem(XRCID("MINUTE_RADIO"));
				break;
			case PERIOD_T_30SEC:
				pmi = menu_bar->FindItem(XRCID("3SEC_RADIO"));
				break;
			case PERIOD_T_SEASON:
				pmi = menu_bar->FindItem(XRCID("SEASON_RADIO"));
				break;
			default:
				break;
		}

		if (pmi)
			pmi->Check(true);

        	menu_bar->FindItem(XRCID("LATEST_DATA_FOLLOW"))->Check(dc->GetFollowLatestData());
	}

}

bool DrawPanel::Switch(wxString set, wxString prefix, time_t time, PeriodType pt, int selected_draw) {
	return dw->SetSet(set, prefix, time, pt, selected_draw);
}

wxString
DrawPanel::GetUrl(bool with_infinity) {
	return dw != NULL ? dw->GetUrl(with_infinity) : _T("");
}

void DrawPanel::SearchDate() {
	if (ee) {
		ee->PrepareSearchFormula();
		ee->Show();
		ee->Raise();
	} else {
		ee = new ParamEdit(this, cfg, db_mgr, dw->GetDrawsController());
		ee->PrepareSearchFormula();
		ee->Show();
	}
}

BEGIN_EVENT_TABLE(DrawPanel, wxPanel)
    LOG_EVT_MENU(drawTB_REFRESH, DrawPanel , OnRefresh, "drawpnl:refresh" )
    LOG_EVT_MENU(drawTB_FIND, DrawPanel , OnFind, "drawpnl:find" )
    LOG_EVT_MENU(drawTB_SUMWIN, DrawPanel , OnSummaryWindow, "drawpnl:summary" )
    LOG_EVT_MENU(XRCID("ContextF0"), DrawPanel , OnFilterChange, "drawpnl:f0" )
    LOG_EVT_MENU(XRCID("ContextF1"), DrawPanel , OnFilterChange, "drawpnl:f1" )
    LOG_EVT_MENU(XRCID("ContextF2"), DrawPanel , OnFilterChange, "drawpnl:f2" )
    LOG_EVT_MENU(XRCID("ContextF3"), DrawPanel , OnFilterChange, "drawpnl:f3" )
    LOG_EVT_MENU(XRCID("ContextF4"), DrawPanel , OnFilterChange, "drawpnl:f4" )
    LOG_EVT_MENU(XRCID("ContextF5"), DrawPanel , OnFilterChange, "drawpnl:f5" )
    LOG_EVT_MENU(drawTB_SPLTCRS, DrawPanel , ToggleSplitCursor, "drawpnl:tb_spltcrs" )
    LOG_EVT_MENU(drawTB_FILTER, DrawPanel , OnToolFilterMenu, "drawpnl:tb_filter" )
    LOG_EVT_MENU(drawTB_DRAWTREE, DrawPanel , OnDrawTree, "drawpnl:drawtree" )
END_EVENT_TABLE()
