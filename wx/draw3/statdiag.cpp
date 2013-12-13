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
#include <wx/statline.h>
#include <wx/minifram.h>

#include "szhlpctrl.h"

#include "szframe.h"
#include "cconv.h"

#include "ids.h"
#include "classes.h"
#include "coobs.h"

#include "drawtime.h"
#include "timeformat.h"
#include "dbinquirer.h"
#include "database.h"
#include "drawdnd.h"
#include "draw.h"
#include "datechooser.h"
#include "cfgmgr.h"
#include "dbmgr.h"
#include "incsearch.h"
#include "drawdnd.h"
#include "progfrm.h"
#include "statdiag.h"


StatDialog::StatDialog(wxWindow *parent, wxString prefix, DatabaseManager *db, ConfigManager *cfg, RemarksHandler *rh, TimeInfo time, DrawInfoList user_draws) :
	szFrame(parent, 
		wxID_ANY, 
		_("Statistics window"),
		wxDefaultPosition,
		wxDefaultSize,
		wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL),
	DBInquirer(db),
	m_period(time.period),
	m_start_time(time.begin_time),
	m_current_time(time.begin_time),
	m_end_time(time.end_time),
	m_tofetch(0)
{
	SetHelpText(_T("draw3-ext-meanvalues"));

	cfg->RegisterConfigObserver(this);

	wxPanel* panel = new wxPanel(this);

	wxString period_choices[PERIOD_T_SEASON] =
	{ _("DECADE"), _("YEAR"), _("MONTH"), _("WEEK"), _("DAY"), _("30 MINUTES") };

	wxStaticText *label; 
	wxStaticLine *line;
	wxButton *button;

	m_config_manager = cfg;
	m_database_manager = db;

	m_draw_search = new IncSearch(m_config_manager, rh, prefix, this, wxID_ANY, _("Choose draw"), false, true, true, m_database_manager);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	label = new wxStaticText(panel, wxID_ANY, wxString(_T(" ")) + _("Choose graph parameters") + _T(": "));

	sizer->Add(label, 0, wxALIGN_CENTER | wxALL, 5);

	wxStaticBoxSizer *sizer_1 = new wxStaticBoxSizer(wxVERTICAL, panel, wxString(_T(" ")) + _("Draw") + _T(": "));
	button = new wxButton(panel, STAT_DRAW_BUTTON, _("Change draw"));
	sizer_1->Add(button, 1, wxEXPAND);
	
	sizer->Add(sizer_1, 0, wxEXPAND | wxALL, 5);

	wxStaticBoxSizer *sizer_2 = new wxStaticBoxSizer(wxVERTICAL, panel, wxString(_T(" ")) + _("Time range") + _T(": "));

	wxFlexGridSizer* sizer_2_1 = new wxFlexGridSizer(2, 5, 5);
	sizer_2_1->AddGrowableCol(1);
	label = new wxStaticText(panel, wxID_ANY, _("Start time:"));
	sizer_2_1->Add(label, 0, wxALIGN_CENTER | wxALL, 5);
	button = new wxButton(panel, STAT_START_TIME, FormatTime(m_start_time.GetTime(), m_period));
	button->SetFocus();
	sizer_2_1->Add(button, 1, wxEXPAND);

	label = new wxStaticText(panel, wxID_ANY, _("End time:"));
	sizer_2_1->Add(label, 0, wxALIGN_CENTER | wxALL, 5);
	button = new wxButton(panel, STAT_END_TIME, FormatTime(m_end_time.GetTime(), m_period));
	sizer_2_1->Add(button, 1, wxEXPAND);

	sizer_2->Add(sizer_2_1, 0, wxEXPAND | wxALL, 5);

	sizer->Add(sizer_2, 0, wxEXPAND | wxALL, 5);

	line = new wxStaticLine(panel);
	sizer->Add(line, 0, wxEXPAND, 5);

	wxStaticBoxSizer* sizer_3 = new wxStaticBoxSizer(wxVERTICAL, panel, wxString(_T(" ")) + _("Period") + _T(": "));
	line = new wxStaticLine(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL);
	m_period_choice = new wxChoice(panel, STAT_CHOICE_PERIOD, wxDefaultPosition, wxDefaultSize, PERIOD_T_SEASON, period_choices);

	sizer_3->Add(m_period_choice, 0, wxALIGN_CENTER | wxALL, 5);
	sizer->Add(sizer_3, 0, wxEXPAND | wxALL, 5);
	
	wxStaticBoxSizer* sizer_4 = new wxStaticBoxSizer(wxVERTICAL, panel, wxString(_T(" ")) + _("Values") + _T(": "));
	wxPanel *subpanel = new wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS);

	wxFlexGridSizer *subpanel_sizer = new wxFlexGridSizer(2, 5, 5);
	subpanel_sizer->AddGrowableCol(2);

	label = new wxStaticText(subpanel, wxID_ANY, wxString(_("Minimum value")) + _T(":"));
	subpanel_sizer->Add(label, wxALIGN_LEFT | wxALL, 10);
	m_min_value_text = new wxStaticText(subpanel, wxID_ANY, _T(""));
	subpanel_sizer->Add(m_min_value_text, wxEXPAND | wxALL, 10);

	label = new wxStaticText(subpanel, wxID_ANY, wxString(_("Average value")) + _T(":"));
	subpanel_sizer->Add(label, wxALIGN_LEFT | wxLEFT | wxRIGHT, 20);
	m_avg_value_text = new wxStaticText(subpanel, wxID_ANY, _T(""));
	subpanel_sizer->Add(m_avg_value_text, wxEXPAND | wxLEFT | wxRIGHT, 20);

	label = new wxStaticText(subpanel, wxID_ANY, wxString(_("Standard deviation")) + _T(":"));
	subpanel_sizer->Add(label, wxALIGN_LEFT | wxLEFT | wxRIGHT, 20);
	m_stddev_value_text = new wxStaticText(subpanel, wxID_ANY, _T(""));
	subpanel_sizer->Add(m_stddev_value_text, wxEXPAND | wxLEFT | wxRIGHT, 20);

	label = new wxStaticText(subpanel, wxID_ANY, wxString(_("Maximum value")) + _T(":"));
	subpanel_sizer->Add(label, wxALIGN_LEFT | wxLEFT | wxRIGHT, 20);
	m_max_value_text = new wxStaticText(subpanel, wxID_ANY, _T(""));
	subpanel_sizer->Add(m_max_value_text, wxEXPAND | wxLEFT | wxRIGHT, 20);

	label = new wxStaticText(subpanel, wxID_ANY, wxString(_("Hour sum")) + _T(":"));
	subpanel_sizer->Add(label, wxALIGN_LEFT | wxLEFT | wxRIGHT, 20);
	m_hsum_value_text = new wxStaticText(subpanel, wxID_ANY, _T(""));
	subpanel_sizer->Add(m_hsum_value_text, wxEXPAND | wxLEFT | wxRIGHT, 20);

	subpanel->SetSizer(subpanel_sizer);

	sizer_4->Add(subpanel, 0, wxEXPAND, 0);
	
	sizer->Add(sizer_4, 0, wxEXPAND | wxALL, 10);

	wxBoxSizer *sizer_5 = new wxBoxSizer(wxHORIZONTAL);
	button = new wxButton(panel, wxID_OK, _("OK"));
	button->SetDefault();
	sizer_5->Add(button, 0, wxALL, 10);
	button = new wxButton(panel, wxID_CLOSE , _("Close"));
	sizer_5->Add(button, 0, wxALL, 10);

	button = new wxButton(panel, wxID_HELP, _("Help"));
	sizer_5->Add(button, 0, wxALL, 10);

	sizer->Add(sizer_5, 1, wxALIGN_CENTER | wxALL, 10);
	panel->SetSizer(sizer);

	wxSizer *ws = new wxBoxSizer(wxHORIZONTAL);
	ws->Add(panel, 1, wxEXPAND);
	SetSizer(ws);
	ws->SetSizeHints(this);
	
	SetDrawInfo( user_draws.GetSelectedDraw() );

	// use enum position
	m_period_choice->SetSelection(time.period);

	DrawInfoDropTarget* dt = new DrawInfoDropTarget(this);
	SetDropTarget(dt);

	Show(true);
}

time_t StatDialog::GetCurrentTime() {
	return -1;
}

DrawInfo* StatDialog::GetCurrentDrawInfo() {
	return m_draw;
}

void StatDialog::DatabaseResponse(DatabaseQuery *q) {

	if (m_tofetch == 0) {
		delete q;
		return;
	}

	for (std::vector<DatabaseQuery::ValueData::V>::iterator i = q->value_data.vv->begin();
			i != q->value_data.vv->end();
			i++) {

		if (std::isnan(i->response))
			continue;

		if (m_count++ == 0) {
			m_max = i->response;
			m_min = i->response;
			m_sum = 0;
			m_sum2 = 0;
			m_hsum = i->sum;

			m_max_time = wxDateTime(i->time);
			m_min_time = wxDateTime(i->time);
		} else {
			if (m_max < i->response) {
				m_max = i->response;
				m_max_time = wxDateTime(i->time);
			}

			if (m_min > i->response) {
				m_min = i->response;
				m_min_time = wxDateTime(i->time);
			}

		}

		m_sum += i->response;
		m_sum2 += i->response * i->response;
		m_hsum += i->sum;
	} 

	m_pending -= q->value_data.vv->size();
	m_totalfetched += q->value_data.vv->size();

	delete q;

	assert(m_pending >= 0);

#if 0
	bool canceled;
	m_progress_dialog->Update(wxMin(99, int(float(m_totalfetched) / (m_tofetch + 1) * 100)), _T(""), &canceled);
#endif
	m_progress_frame->SetProgress(m_totalfetched * 100 / m_tofetch);

	if (m_totalfetched == m_tofetch) {

		assert(m_pending == 0);

		if (m_count) {
			wxString unit = m_draw->GetUnit();
			double sdev = sqrt(m_sum2 / m_count - m_sum / m_count * m_sum / m_count);

			m_min_value_text->SetLabel(wxString::Format(_T("%s %s (%s)"),
						m_draw->GetValueStr(m_min, _T("- -")).c_str(),
						unit.c_str(),
						FormatTime(m_min_time, m_period).c_str()));
			m_stddev_value_text->SetLabel(wxString::Format(_T("%s %s"),
						m_draw->GetValueStr(sdev, _T("- -")).c_str(),
						unit.c_str()));
			m_avg_value_text->SetLabel(wxString::Format(_T("%s %s"),
						m_draw->GetValueStr(m_sum / m_count, _T("- -")).c_str(),
						unit.c_str()));
			m_max_value_text->SetLabel(wxString::Format(_T("%s %s (%s)"), 
						m_draw->GetValueStr(m_max, _T("- -")).c_str(),
						unit.c_str(),
						FormatTime(m_max_time, m_period).c_str()));
			if (m_draw->GetSpecial() == TDraw::HOURSUM) {
				if (unit.Replace(_T("/h"), _T("")) == 0)
					unit += _T("h");
				m_hsum_value_text->SetLabel(wxString::Format(_T("%s %s"), m_draw->GetValueStr(m_hsum / m_draw->GetSumDivisor()).c_str(), unit.c_str()));
			}
		} else {
			m_min_value_text->SetLabel(_("no data"));
			m_avg_value_text->SetLabel(_("no data"));
			m_stddev_value_text->SetLabel(_("no data"));
			m_max_value_text->SetLabel(_("no data"));
			if (m_draw->GetSpecial() == TDraw::HOURSUM) 
				m_hsum_value_text->SetLabel(_("no data"));
		}
		m_progress_frame->Destroy();

		m_tofetch = 0;

	} else if (m_pending == 0)
		ProgressFetch();
}

void StatDialog::StartFetch() {
	if (m_draw == NULL) {
		wxMessageBox(_("You need to choose draw first."), _("Draw missing"), wxOK | wxICON_INFORMATION, this);
		return;
	}

	m_start_time.AdjustToPeriod();
	m_current_time = m_start_time;

	m_end_time.AdjustToPeriod();
	
	m_count = 0;
	m_pending = 0;
	m_tofetch = m_current_time.GetDistance(m_end_time) + 1;
	m_totalfetched = 0;

	if (m_tofetch <= 0)
		return;

	//m_progress_dialog = new wxProgressDialog(_T("Caclulating"), _T("Calculation in progress"), 100, this, wxPD_AUTO_HIDE);
	m_progress_frame = new ProgressFrame(this);
	m_progress_frame->Centre();
	m_progress_frame->Show(true);

	ProgressFetch();
}

void StatDialog::ProgressFetch() {

	TimeIndex idx(m_period);

	DatabaseQuery* q = CreateDataQuery(m_draw, m_period);

	while (m_current_time <= m_end_time && m_pending < 200) {
		AddTimeToDataQuery(q, m_current_time.GetTime().GetTicks());

		m_pending++;
		m_current_time = m_current_time + idx.GetTimeRes() + idx.GetDateRes();
	}

	QueryDatabase(q);
}

void StatDialog::OnDateChange(wxCommandEvent &event) {
	wxButton *button;
	wxDateTime time;

	bool start_time = false;

	if (event.GetId() == STAT_START_TIME) {
		button = wxDynamicCast(FindWindow(STAT_START_TIME), wxButton);
		start_time = true;
		time = m_start_time.GetTime();
	} else  {
		button = wxDynamicCast(FindWindow(STAT_END_TIME), wxButton);
		time = m_end_time.GetTime();
	}

	DateChooserWidget *dcw = 
		new DateChooserWidget(
				this,
				_("Select date"),
				1,
				-1,
				-1,
				10
		);
	
	bool ret = dcw->GetDate(time);
	dcw->Destroy();

	if (ret == false)
		return;

	if (start_time) {
		m_start_time = DTime(m_period, time);
		m_start_time.AdjustToPeriod();
		button->SetLabel(FormatTime(time, m_period));
	} else {
		m_end_time = DTime(m_period, time);
		m_end_time.AdjustToPeriod();
		button->SetLabel(FormatTime(time, m_period));
	}


}

void StatDialog::OnDrawChange(wxCommandEvent &event) {
	if (m_draw_search->ShowModal() == wxID_CANCEL)
		return;
#ifdef __WXMSW__
	//what the hell
	Raise();
#endif

	long previous = -1;
	DrawInfo *choosen = m_draw_search->GetDrawInfo(&previous);
	if (choosen == NULL || choosen == m_draw)
		return;
	
	SetDrawInfo(choosen);
}

void StatDialog::SetDrawInfo(DrawInfo *draw) {
	wxButton *button = wxDynamicCast(FindWindowById(STAT_DRAW_BUTTON), wxButton);
	m_draw = draw;

	wxString bn;
	if (draw) 
		bn = draw->GetName();
	else
		bn = _T("");
	button->SetLabel(bn);
	m_min_value_text->SetLabel(_T(""));	
	m_max_value_text->SetLabel(_T(""));	
	m_avg_value_text->SetLabel(_T(""));	
	m_hsum_value_text->SetLabel(_T(""));	
	
}

void StatDialog::OnPeriodChange(wxCommandEvent &event) {
	m_period = (PeriodType) m_period_choice->GetSelection();
	wxButton *button;

	button = wxDynamicCast(FindWindow(STAT_START_TIME), wxButton);
	m_start_time.SetPeriod(m_period);
	button->SetLabel(FormatTime(m_start_time.GetTime(), m_period));

	button = wxDynamicCast(FindWindow(STAT_END_TIME), wxButton);
	m_end_time.SetPeriod(m_period);
	button->SetLabel(FormatTime(m_end_time.GetTime(), m_period));
}

void StatDialog::OnCloseButton(wxCommandEvent &event) {
	Destroy();
}

void StatDialog::OnCalculate(wxCommandEvent &event) {

	if (m_draw == NULL) {
		wxMessageBox(_("You need to choose draw first"), _T("Draw missing"), wxOK | wxICON_INFORMATION, this);
		return;
	}

	if (m_start_time > m_end_time) {
		wxMessageBox(_("Start date must be less than or equal to end date"), _("Invalid date"), wxICON_INFORMATION | wxOK, this);
		return;
	}

	StartFetch();
}

wxDragResult StatDialog::SetDrawInfo(wxCoord x, wxCoord y, DrawInfo *draw_info, wxDragResult def) {
	SetDrawInfo(draw_info);
	return def;
}

StatDialog::~StatDialog() {
	m_draw_search->Destroy();
	m_config_manager->DeregisterConfigObserver(this);
}

void StatDialog::OnClose(wxCloseEvent &event) {
	Destroy();
}

void StatDialog::ConfigurationIsAboutToReload(wxString prefix) {
	if (m_draw == NULL)
		return;

	m_param_prefix = m_draw->GetBasePrefix();
	if (m_param_prefix != prefix)
		return;

}

void StatDialog::ConfigurationWasReloaded(wxString prefix) {
	if (m_draw == NULL)
		return;

	if (m_param_prefix != prefix)
		return;

	if (m_tofetch > 0) {
		m_progress_frame->Destroy();
		m_tofetch = 0;
	}

	SetDrawInfo(NULL);
}

void StatDialog::OnHelpButton(wxCommandEvent &event) {
	wxHelpProvider::Get()->ShowHelp(this);
}

void StatDialog::OnShow(wxShowEvent &event) {
	wxWindow *w = FindWindowById(STAT_DRAW_BUTTON);
	w->SetFocus();
}

BEGIN_EVENT_TABLE(StatDialog, szFrame) 
	LOG_EVT_BUTTON(STAT_START_TIME, StatDialog , OnDateChange, "statdiag:start_time" )
	LOG_EVT_BUTTON(STAT_END_TIME, StatDialog , OnDateChange, "statdiag:end_time" )
	LOG_EVT_BUTTON(STAT_DRAW_BUTTON, StatDialog , OnDrawChange, "statdiag:draw_button" )
	LOG_EVT_BUTTON(wxID_OK, StatDialog , OnCalculate, "statdiag:ok" )
	LOG_EVT_BUTTON(wxID_CLOSE, StatDialog , OnCloseButton, "statdiag:close" )
	LOG_EVT_BUTTON(wxID_HELP, StatDialog , OnHelpButton, "statdiag:help" )
	LOG_EVT_SHOW(StatDialog , OnShow, "statdiag:show" )
	LOG_EVT_CHOICE(STAT_CHOICE_PERIOD, StatDialog , OnPeriodChange, "statdiag:choice" )
	LOG_EVT_CLOSE(StatDialog , OnClose, "statdiag:close" )
END_EVENT_TABLE()
