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
#include <map>

#include "szhlpctrl.h"
#include "szframe.h"

#include "ids.h"
#include "classes.h"
#include "coobs.h"
#include "drawdnd.h"
#include "timeformat.h"
#include "drawtime.h"
#include "database.h"
#include "draw.h"
#include "dbinquirer.h"
#include "datechooser.h"
#include "progfrm.h"
#include "xydiag.h"
#include "cfgmgr.h"
#include "xygraph.h"
#include "incsearch.h"

/* initialize static members from XYDialog class */
int XYDialog::m_loaded_time = 0;
DTime XYDialog::m_saved_start_time = DTime();
DTime XYDialog::m_saved_end_time = DTime();

XYDialog::XYDialog(wxWindow *parent, wxString prefix, ConfigManager *cfg, DatabaseManager *db, RemarksHandler *rh, TimeInfo time, DrawInfoList user_draws,  XFrame *frame) :
		wxDialog(parent,
			wxID_ANY,
			_("X/Y graph parameters"),
			wxDefaultPosition,
			wxDefaultSize,
			wxDEFAULT_DIALOG_STYLE | wxTAB_TRAVERSAL | wxRESIZE_BORDER ),
		m_period(time.period),
		m_start_time(time.begin_time),
		m_end_time(time.end_time)
		{

	if (user_draws.GetDoubleCursor()) {
		m_start_time = DTime(m_period, wxDateTime(user_draws.GetStatsInterval().first));
		m_end_time = DTime(m_period, wxDateTime(user_draws.GetStatsInterval().second));
	}

	if (frame->GetDimCount() == 3)
		SetHelpText(_T("draw3-ext-chartxyz"));
	else
		SetHelpText(_T("draw3-ext-chartxy"));

	SetIcon(szFrame::default_icon);

	wxString period_choices[PERIOD_T_SEASON] = { _("DECADE"), _("YEAR"), _("MONTH"), _("WEEK"), _("DAY"), _("30 MINUTES") };

	m_start_time.AdjustToPeriod();
	m_end_time.AdjustToPeriod();

	wxStaticText *label; 
	wxStaticLine *line;
	wxButton *button;

	m_config_manager = cfg;
	
	m_database_manager = db;

	m_remarks_handler = rh;

	m_mangler = NULL;

	m_frame = frame;

	m_draw_search = new IncSearch(m_config_manager, m_remarks_handler, prefix, this, incsearch_DIALOG, _("Choose draw"), false, true, true, m_database_manager);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	label = new wxStaticText(this, wxID_ANY, _("Choose graph parameters:"));
	line  = new wxStaticLine(this);

	sizer->Add(label, 0, wxALIGN_CENTER | wxALL, 5);
	sizer->Add(line, 0, wxEXPAND | wxALL, 5);

#define __BUTTON_NAME(N) user_draws.size() >= N ? user_draws[N-1]->GetName() :wxString( _("Choose draw"))

	wxBoxSizer* sizer_1 = new wxBoxSizer(wxHORIZONTAL);
	label = new wxStaticText(this, wxID_ANY, _("Axis X:"));
	sizer_1->Add(label, 0, wxALIGN_CENTER | wxALL, 5);
	button = new wxButton(this, XY_XAXIS_BUTTON, __BUTTON_NAME(1));
	sizer_1->Add(button, 1, wxEXPAND);
	sizer->Add(sizer_1, 0, wxEXPAND | wxALL, 5);

	wxBoxSizer* sizer_2 = new wxBoxSizer(wxHORIZONTAL);
	label = new wxStaticText(this, wxID_ANY, _("Axis Y:"));
	sizer_2->Add(label, 0, wxALIGN_CENTER | wxALL, 5);
	button = new wxButton(this, XY_YAXIS_BUTTON, __BUTTON_NAME(2));
	sizer_2->Add(button, 1, wxEXPAND);
	sizer->Add(sizer_2, 0, wxEXPAND | wxALL, 5);

	if (m_frame->GetDimCount() == 3) {
		wxBoxSizer* sizer_3 = new wxBoxSizer(wxHORIZONTAL);
		label = new wxStaticText(this, wxID_ANY, _("Axis Z:"));
		sizer_3->Add(label, 0, wxALIGN_CENTER | wxALL, 5);
		button = new wxButton(this, XY_ZAXIS_BUTTON, __BUTTON_NAME(3));
		sizer_3->Add(button, 1, wxEXPAND);
		sizer->Add(sizer_3, 0, wxEXPAND | wxALL, 5);
	} 

#undef __BUTTON_NAME

	line = new wxStaticLine(this);

	sizer->Add(line, 0, wxEXPAND | wxALL, 5);
	wxBoxSizer* sizer_4 = new wxBoxSizer(wxHORIZONTAL);
	label = new wxStaticText(this, wxID_ANY, _("Choose period:"));
	sizer_4->Add(label, 0, wxALIGN_CENTER | wxALL, 5);
	line = new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL);
	m_period_choice = new wxChoice(this, XY_CHOICE_PERIOD, wxDefaultPosition, wxDefaultSize, PERIOD_T_SEASON, period_choices);
	sizer_4->Add(m_period_choice, 0, wxALIGN_CENTER, 5);
	sizer->Add(sizer_4, 0, wxALIGN_CENTER | wxALL, 5);

	line = new wxStaticLine(this);
	sizer->Add(line, 0, wxEXPAND | wxALL, 5);

	wxFlexGridSizer* sizer_3 = new wxFlexGridSizer(2, 5, 5);

	sizer_3->AddGrowableCol(1);
	label = new wxStaticText(this, wxID_ANY, _("Start time:"));
	sizer_3->Add(label, 0, wxALIGN_CENTER | wxALL, 5);
	button = new wxButton(this, XY_START_TIME, FormatTime(m_start_time.GetTime(), m_period));
	sizer_3->Add(button, 1, wxEXPAND);
	sizer->Add(sizer_3, 0, wxEXPAND | wxALL, 5);

	label = new wxStaticText(this, wxID_ANY, _("End time:"));
	sizer_3->Add(label, 0, wxALIGN_CENTER | wxALL, 5);
	button = new wxButton(this, XY_END_TIME, FormatTime(m_end_time.GetTime(), m_period));
	sizer_3->Add(button, 1, wxEXPAND);

	wxBoxSizer *button_sizer1 = new wxBoxSizer(wxHORIZONTAL);
	button = new wxButton(this, XY_SAVE_TIME, _("Save time"));
	button_sizer1->Add(button, 0, wxALL, 3);
	button = new wxButton(this, XY_LOAD_TIME, _("Load time"));
	button_sizer1->Add(button, 0, wxALL, 3);
	if (m_loaded_time == 0) {
		button->Disable();
		button->Update();
	}
	sizer->Add(button_sizer1, 0, wxALIGN_RIGHT | wxALL, 5);

	line = new wxStaticLine(this);
	sizer->Add(line, 0, wxEXPAND | wxALL, 5);

	m_avg_check = NULL;
	if (m_frame->GetDimCount() == 2) {
		m_avg_check = new wxCheckBox(this, wxID_ANY, _("Average values"));
		sizer->Add(m_avg_check, 0, wxALIGN_CENTER | wxALL, 5);

		line = new wxStaticLine(this);
		sizer->Add(line, 0, wxEXPAND | wxALL, 5);
	}

	wxBoxSizer *button_sizer = new wxBoxSizer(wxHORIZONTAL);
	button = new wxButton(this, wxID_OK, _("OK"));
	button_sizer->Add(button, 0, wxALL, 10);
	button = new wxButton(this, wxID_CANCEL, _("Cancel"));
	button_sizer->Add(button, 0, wxALL, 10);
	button = new wxButton(this, wxID_HELP, _("Help"));
	button_sizer->Add(button, 0, wxALL, 10);

	sizer->Add(button_sizer, 1, wxALIGN_CENTER | wxALL, 10);

	m_period_choice->SetSelection(m_period);
	m_period_choice->SetFocus();

	// init draws information
	for (int i = 0; i < m_frame->GetDimCount(); i++) {
		if ((int) (user_draws.size() -1)  >= i)
			m_di.push_back(user_draws[i]);
		else
			m_di.push_back(NULL);
	}

	if (m_frame->GetDimCount() == 3)
		SetTitle(_("XYZ Graph"));

	SetSizer(sizer);
	sizer->SetSizeHints(this);

}

void XYDialog::OnOK(wxCommandEvent &event) {
	bool ok = true;
	for( DrawInfoList::iterator i = m_di.begin() ; i != m_di.end() && (ok&=(bool)*i) ; ++i );

	if( !ok ) {
		if( m_frame->GetDimCount() == 2 )
			wxMessageBox(_("You have to specify both draws"), _("Draw missing"), wxICON_INFORMATION | wxOK, this);
		else
			wxMessageBox(_("You have to specify all draws") , _("Draw missing"), wxICON_INFORMATION | wxOK, this);
		return;
	}

	if( m_start_time > m_end_time ) {
		wxMessageBox(_("Start date must be less than or equal to end date"), _("Invalid date"), wxICON_INFORMATION | wxOK, this);
		return;
	}

	delete m_mangler;

	m_mangler = new DataMangler(m_database_manager,
			m_di,
			m_start_time.GetTime(),
			m_end_time.GetTime(),
			m_period,
			this,
			m_avg_check ? m_avg_check->IsChecked() : false);

	m_mangler->Go();
}

XYGraph* XYDialog::GetGraph() {
	return m_graph;
}

void XYDialog::DataFromMangler(XYGraph *graph) {
	if (graph->m_points_values.size() == 0) {
		wxMessageBox(_("There is no common data in selected draws from given time span"), _("No common data"), wxICON_INFORMATION | wxOK, this);
		delete graph;
		return;
	}

	m_graph = graph;
	EndModal(wxID_OK);
}

void XYDialog::ConfigurationIsAboutToReload(wxString prefix) {
	wxButton *button;

	for (size_t i = 0; i < m_di.size(); i++)
		if (m_di[i] && m_di[i]->GetBasePrefix() == prefix) {
			delete m_mangler;
			m_mangler = NULL;

			m_di[i] = NULL;

			button = wxDynamicCast(FindWindow(XY_XAXIS_BUTTON + i), wxButton);
			button->SetLabel(_T(""));
		}
}

void XYDialog::ConfigurationWasReloaded(wxString prefix) { }


void XYDialog::OnPeriodChange(wxCommandEvent &event) {
	m_period = (PeriodType) m_period_choice->GetSelection();
	wxButton *button;

	button = wxDynamicCast(FindWindow(XY_START_TIME), wxButton);
	m_start_time.AdjustToPeriod();
	button->SetLabel(FormatTime(m_start_time.GetTime(), m_period));

	button = wxDynamicCast(FindWindow(XY_END_TIME), wxButton);
	m_end_time.AdjustToPeriod();
	button->SetLabel(FormatTime(m_end_time.GetTime(), m_period));
}


void XYDialog::OnDateChange(wxCommandEvent &event) {
	wxButton *button;
	DTime *time;

	if (event.GetId() == XY_START_TIME) {
		button = wxDynamicCast(FindWindow(XY_START_TIME), wxButton);
		time = &m_start_time;
	} else {
		button = wxDynamicCast(FindWindow(XY_END_TIME), wxButton);
		time = &m_end_time;
	}

	DateChooserWidget *dcw = 
		new DateChooserWidget(
				this,
				_("Select date"),
				1,
				-1,
				-1,
				1
		);

	wxDateTime wxtime = time->GetTime();
	
	bool ret = dcw->GetDate(wxtime);
	dcw->Destroy();

	if (ret == false)
		return;

	*time = DTime(m_period, wxtime);
	time->AdjustToPeriod();

	button->SetLabel(FormatTime(time->GetTime(), m_period));

}

void XYDialog::OnSaveTimeButton(wxCommandEvent &event) {
	wxButton *button = wxDynamicCast(FindWindow(XY_LOAD_TIME), wxButton);

	if (m_loaded_time == 0) {
		button->Enable(true);
		button->Update();
		m_loaded_time = 1;
	}

	m_saved_start_time = m_start_time;
	m_saved_end_time = m_end_time;
}

void XYDialog::OnLoadTimeButton(wxCommandEvent &event) {
	wxButton *button;

	m_start_time = m_saved_start_time;
	m_end_time = m_saved_end_time;

	button = wxDynamicCast(FindWindow(XY_START_TIME), wxButton);
	button->SetLabel(FormatTime(m_start_time.GetTime(), m_period));

	button = wxDynamicCast(FindWindow(XY_END_TIME), wxButton);
	button->SetLabel(FormatTime(m_end_time.GetTime(), m_period));
}

void XYDialog::OnDrawChange(wxCommandEvent &event) {
	DrawInfo **draw;
	wxButton *button;

	int i = event.GetId() - XY_XAXIS_BUTTON;
	draw = &m_di[i];

	button = wxDynamicCast(FindWindowById(event.GetId()), wxButton);

	if (m_draw_search->ShowModal() == wxID_CANCEL)
		return;
#ifdef __WXMSW__
	//hilarious...
	Raise();
#endif

	long previous = -1;
	DrawInfo *choosen = m_draw_search->GetDrawInfo(&previous);
	if (choosen == NULL)
		return;
	
	*draw = choosen;
	button->SetLabel(choosen->GetName());

}

#if 0
wxDragResult XYDialog::SetDrawInfo(wxCoord x, wxCoord y, DrawInfo *draw_info, wxDragResult def) {
	wxLogWarning(_T("x:%d y:%d"), (int)x, (int)y);
	return wxDragNone;
}
#endif

XYDialog::~XYDialog() {
	delete m_mangler;
	m_config_manager->DeregisterConfigObserver(this);
	m_draw_search->Destroy();
}

DataMangler::DataMangler(DatabaseManager* db, const DrawInfoList& di, wxDateTime start_time, wxDateTime end_time, PeriodType period, XYDialog *dialog, bool average) :
		DBInquirer(db),
		m_di(di),
		m_period(period),
		m_start_time(m_period, start_time),
		m_current_time(m_period, start_time),
		m_end_time(m_period, end_time) {

	m_start_time.AdjustToPeriod();
	m_current_time.AdjustToPeriod();
	m_end_time.AdjustToPeriod();

	assert (m_start_time <= m_end_time);
	
	int vs = m_start_time.GetDistance(m_end_time) + 1;

	DTime tmpdate(PERIOD_T_OTHER, wxDateTime::Now());
	std::pair<std::vector<ValueInfo> , DTime> tmptuple(std::vector<ValueInfo>(di.size(), ValueInfo()), tmpdate);
	m_draw_vals.resize(vs, tmptuple);

	m_dialog = dialog;

	m_average = average;

}	

void DataMangler::Go() {
	m_tofetch = m_draw_vals.size() * m_di.size();
	m_fetched = 0;
	m_pending = 0;

	m_progress = new ProgressFrame(m_dialog);
	m_progress->Show(true);
	m_progress->Raise();

	ProgressFetch();
}

void DataMangler::ProgressFetch() {
	TimeIndex idx(m_period);

	std::vector<DatabaseQuery*> queries;
	for (size_t i = 0; i < m_di.size(); i++)
		queries.push_back(CreateDataQuery(m_di[i], m_period, i));

	while (m_pending < 200 && m_current_time <= m_end_time) {
		for (size_t i = 0; i < m_di.size(); i++)
			AddTimeToDataQuery(queries[i], m_current_time.GetTime().GetTicks());
		m_pending += m_di.size();
		m_current_time = m_current_time + idx.GetTimeRes() + idx.GetDateRes();
	}

	for (size_t i = 0; i < m_di.size(); i++)
		QueryDatabase(queries[i]);
}

void DataMangler::DatabaseResponse(DatabaseQuery *q) {

	for (std::vector<DatabaseQuery::ValueData::V>::iterator i = q->value_data.vv->begin();
			i != q->value_data.vv->end();
			i++) {
		DTime time = DTime(m_period, i->time);

		int index = m_start_time.GetDistance(time);

		m_draw_vals.at(index).second = time;

		ValueInfo *vi;
		vi = &m_draw_vals.at(index).first.at(q->draw_no);
		vi->state = ValueInfo::PRESENT;
		vi->val = i->response;
	}

	m_pending -= q->value_data.vv->size();
	m_fetched += q->value_data.vv->size();

	delete q;

	m_progress->SetProgress(m_fetched * 100 / m_tofetch);

	if (m_fetched == m_tofetch) {
		assert(m_pending == 0);

		XYGraph* graph = new XYGraph;

		graph->m_start = m_start_time.GetTime();
		graph->m_end = m_end_time.GetTime();
		graph->m_period = m_period;
		graph->m_di = m_di;
		graph->m_min.resize(m_di.size());
		graph->m_max.resize(m_di.size());
		graph->m_dmin.resize(m_di.size());
		graph->m_dmax.resize(m_di.size());
		graph->m_avg.resize(m_di.size());
		graph->m_standard_deviation.resize(m_di.size());
		graph->m_averaged = m_average;

		AssociateValues(graph);
		FindMaxMinRange(graph);

		if (m_average)
			AverageValues(graph);

		m_progress->Destroy();
		m_dialog->DataFromMangler(graph);
	} if (m_pending <= 100 && m_pending + m_fetched != m_tofetch) {
		ProgressFetch();
	}

}

void DataMangler::AverageValues(XYGraph *graph) {
	double pvx = pow(0.1, m_di[0]->GetPrec());

	int count;
	double slice;
	double dif = graph->m_dmax[0] - graph->m_dmin[0];
	if (dif < pvx) {
		count = 1;
		slice = pvx;
	} else {
		count = 0;
		while (dif / count > pvx && count < 20)
			++count;
		slice = dif / count;
	}

	std::deque<double> sums(count, 0);
	std::deque<int> counts(count, 0);
	std::deque< std::vector<DTime> > ptimes(count);

	for (size_t i = 0; i < graph->m_points_values.size(); ++i) {
		double& x = graph->m_points_values.at(i).first[0];
		double& y = graph->m_points_values.at(i).first[1];
		std::vector<DTime> &times = graph->m_points_values.at(i).second;

		int idx = int((x - graph->m_dmin[0] - slice / 2) * count / dif);
		if (idx < 0)
			idx = 0;

		if (idx >= count)
			idx = count - 1;

		counts[idx]++;
		sums[idx] += y;
 
		for(unsigned int j = 0; j < times.size(); j++)
			ptimes[idx].push_back(times[j]);
	}

	graph->m_points_values.clear();

	for (int i = 0; i < count; ++i) {
		int c = counts[i];
		if (c == 0)
			continue;

		std::vector<double> xy(2);
		xy[0] = graph->m_dmin[0] + dif * i / count;
		xy[1] = sums[i] / c;

		graph->m_points_values.push_back(XYPoint(xy, ptimes[i]));
	}
			
}


void DataMangler::ApplyPPAlgorithm(double &min, double &max, int prec) {

	long lmax = long(max * pow(10, prec));
	long lmin = long(min * pow(10, prec));

	long distance = lmax - lmin;

	long offset = (long)(distance * 0.1);

	long tmin = lmin - offset;
	long tmax = lmax + offset;


	if(tmin <= 0 && lmin >= 0){
		min = 0;
	} else {
		long current_alignment = 10;
		long best_alignment = 10;

		while(true) {
			if(tmin / current_alignment == lmin / current_alignment){
				break;
			} else {
				best_alignment = current_alignment;
				current_alignment *= 10;
			}
		}

		tmin = lmin / best_alignment * best_alignment;
		if(lmin < 0){
			tmin -= best_alignment;
		}
		min = double(tmin) / pow(10, prec);
	}

	if(lmax <= 0 && tmax >= 0){
		max = 0;
	} else {
		long current_alignment = 10;
		long best_alignment = 10;

		while(true) {
			if(tmax / current_alignment == lmax / current_alignment){
				break;
			} else {
				best_alignment = current_alignment;
				current_alignment *= 10;
			}
		}

		tmax = lmax / best_alignment * best_alignment;
		if(lmax > 0){
			tmax += best_alignment;
		}
		max = double(tmax) / pow(10, prec);
	}

}


void DataMangler::FindMaxMinRange(XYGraph *graph) {
	for (size_t i = 0; i < graph->m_di.size(); i++) {
		graph->m_dmin[i] = graph->m_min[i];
		graph->m_dmax[i] = graph->m_max[i];
		ApplyPPAlgorithm(graph->m_dmin[i], graph->m_dmax[i], graph->m_di[i]->GetPrec());
	}
}

void DataMangler::AssociateValues(XYGraph *graph) {

	int count = 0;
	
	std::map<std::vector<double>, std::vector<DTime> > points;

	for (size_t i = 0; i < m_draw_vals.size(); ++i) {
		std::vector<double> v;

		DTime time = m_draw_vals.at(i).second;
		
		for (size_t j = 0; j < graph->m_di.size(); j++) {
			ValueInfo &vi = m_draw_vals.at(i).first[j];
			if (!vi.IsData())
				goto next;
			v.push_back(vi.val);
		}
		
		points[v].push_back(time);

		if (count != 0)
			for (size_t j = 0; j < graph->m_di.size(); j++) {
				graph->m_min[j] = wxMin(graph->m_min[j], v[j]);
				graph->m_max[j] = wxMax(graph->m_max[j], v[j]);
			}
		else
			for (size_t j = 0; j < graph->m_di.size(); j++)
				graph->m_min[j] = graph->m_max[j] = v[j];

		count++;
next:;
	}

	std::map<std::vector<double>, std::vector<DTime> >::iterator i;
	std::vector<DTime>::iterator j;

	for (i = points.begin(); i != points.end(); i++)
			graph->m_points_values.push_back(XYPoint(i->first, i->second));
}

void XYDialog::OnCancel(wxCommandEvent &event) {
	EndModal(wxID_CANCEL);
}

void XYDialog::OnShow(wxShowEvent &event) {
	wxWindow *w = FindWindowById(XY_XAXIS_BUTTON);
	w->SetFocus();
}

void XYDialog::OnHelpButton(wxCommandEvent &event) {
	wxHelpProvider::Get()->ShowHelp(this);
}

DrawInfo* DataMangler::GetCurrentDrawInfo() {
	return m_di[0];
}

time_t DataMangler::GetCurrentTime() {
	return -1;
}


BEGIN_EVENT_TABLE(XYDialog, wxDialog) 
	LOG_EVT_BUTTON(XY_START_TIME, XYDialog , OnDateChange, "xydiag:start_date" )
	LOG_EVT_BUTTON(wxID_OK, XYDialog , OnOK, "xydiag:ok" )
	LOG_EVT_BUTTON(wxID_CANCEL, XYDialog , OnCancel, "xydiag:cancel" )
	LOG_EVT_BUTTON(wxID_HELP, XYDialog , OnHelpButton, "xydiag:help" )
	LOG_EVT_BUTTON(XY_END_TIME, XYDialog , OnDateChange, "xydiag:end_date" )
	LOG_EVT_BUTTON(XY_SAVE_TIME, XYDialog , OnSaveTimeButton, "xydiag:save_time" )
	LOG_EVT_BUTTON(XY_LOAD_TIME, XYDialog , OnLoadTimeButton, "xydiag:load_time" )
	LOG_EVT_CHOICE(XY_CHOICE_PERIOD, XYDialog , OnPeriodChange, "xydiag:period_change" )
	LOG_EVT_BUTTON(XY_XAXIS_BUTTON, XYDialog , OnDrawChange, "xydiag:xaxis" )
	LOG_EVT_BUTTON(XY_YAXIS_BUTTON, XYDialog , OnDrawChange, "xydiag:yaxis" )
	LOG_EVT_BUTTON(XY_ZAXIS_BUTTON, XYDialog , OnDrawChange, "xydiag:zaxis" )
END_EVENT_TABLE()
