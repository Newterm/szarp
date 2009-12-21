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
#include "dbinquirer.h"
#include "draw.h"
#include "datechooser.h"
#include "progfrm.h"
#include "xydiag.h"
#include "cfgmgr.h"
#include "xygraph.h"
#include "incsearch.h"

XYDialog::XYDialog(wxWindow *parent, wxString prefix, ConfigManager *cfg, DatabaseManager *db, XYFrame *frame) : 
		wxDialog(parent, 
			wxID_ANY, 
			_("X/Y graph parameters"),
			wxDefaultPosition,
			wxDefaultSize,
			wxDEFAULT_DIALOG_STYLE | wxTAB_TRAVERSAL),
		m_period(PERIOD_T_YEAR),
		m_start_time(m_period, wxDateTime::Now()),
		m_end_time(m_period, wxDateTime::Now()) {

	SetHelpText(_T("draw3-ext-chartxy"));

	wxString period_choices[PERIOD_T_SEASON] = { _("YEAR"), _("MONTH"), _("WEEK"), _("DAY") };

	m_xdraw = NULL;
	m_ydraw = NULL;

	m_start_time.AdjustToPeriod();
	m_end_time.AdjustToPeriod();

	wxStaticText *label; 
	wxStaticLine *line;
	wxButton *button;

	m_config_manager = cfg;
	
	m_database_manager = db;

	m_mangler = NULL;

	m_frame = frame;

	m_draw_search = new IncSearch(m_config_manager, prefix, this, incsearch_DIALOG, _("Choose draw"), false, true, true, m_database_manager);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	label = new wxStaticText(this, wxID_ANY, _("Choose graph parameters:"));
	line  = new wxStaticLine(this);

	sizer->Add(label, 0, wxALIGN_CENTER | wxALL, 5);
	sizer->Add(line, 0, wxEXPAND, 10);

	wxBoxSizer* sizer_1 = new wxBoxSizer(wxHORIZONTAL);
	label = new wxStaticText(this, wxID_ANY, _("Axis X:"));
	sizer_1->Add(label, 0, wxALIGN_CENTER | wxALL, 5);
	button = new wxButton(this, XY_XAXIS_BUTTON, _("Choose draw"));
	sizer_1->Add(button, 1, wxEXPAND);
	sizer->Add(sizer_1, 0, wxEXPAND | wxALL, 5);

	wxBoxSizer* sizer_2 = new wxBoxSizer(wxHORIZONTAL);
	label = new wxStaticText(this, wxID_ANY, _("Axis Y:"));
	sizer_2->Add(label, 0, wxALIGN_CENTER | wxALL, 5);
	button = new wxButton(this, XY_YAXIS_BUTTON, _("Choose draw"));
	sizer_2->Add(button, 1, wxEXPAND);
	sizer->Add(sizer_2, 0, wxEXPAND | wxALL, 5);

	line = new wxStaticLine(this);
	sizer->Add(line, 0, wxEXPAND, 10);

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

	line = new wxStaticLine(this);
	sizer->Add(line, 0, wxEXPAND, 5);

	wxBoxSizer* sizer_4 = new wxBoxSizer(wxHORIZONTAL);
	label = new wxStaticText(this, wxID_ANY, _("Choose period:"));
	sizer_4->Add(label, 0, wxALIGN_CENTER | wxALL, 5);
	line = new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL);
	m_period_choice = new wxChoice(this, XY_CHOICE_PERIOD, wxDefaultPosition, wxDefaultSize, PERIOD_T_SEASON, period_choices);
	sizer_4->Add(m_period_choice, 0, wxALIGN_CENTER, 5);
	sizer->Add(sizer_4, 0, wxALIGN_CENTER | wxALL, 5);

	line = new wxStaticLine(this);
	sizer->Add(line, 0, wxEXPAND | wxALL, 5);

	m_avg_check = new wxCheckBox(this, wxID_ANY, _("Average values"));
	sizer->Add(m_avg_check, 0, wxALIGN_CENTER | wxALL, 5);

	line = new wxStaticLine(this);
	sizer->Add(line, 0, wxEXPAND | wxALL, 5);

	wxBoxSizer *button_sizer = new wxBoxSizer(wxHORIZONTAL);
	button = new wxButton(this, wxID_OK, _("OK"));
	button_sizer->Add(button, 0, wxALL, 10);
	button = new wxButton(this, wxID_CANCEL, _("Cancel"));
	button_sizer->Add(button, 0, wxALL, 10);
	button = new wxButton(this, wxID_HELP, _("Help"));
	button_sizer->Add(button, 0, wxALL, 10);

	sizer->Add(button_sizer, 1, wxALIGN_CENTER | wxALL, 10);

	SetSize(500, 350);

	SetSizer(sizer);

	m_period_choice->SetSelection(0);
	m_period_choice->SetFocus();

	//DrawInfoDropTarget* dt = new DrawInfoDropTarget(this);
	//SetDropTarget(dt);

}

void XYDialog::OnOK(wxCommandEvent &event) {
	if (m_xdraw == NULL || m_ydraw == NULL) {
		wxMessageBox(_("You have to specify both draws"), _("Draw missing"), wxICON_INFORMATION | wxOK, this);
		return;
	}

	if (m_start_time > m_end_time) {
		wxMessageBox(_("Start date must be less than or equal to end date"), _("Invalid date"), wxICON_INFORMATION | wxOK, this);
		return;
	}

	delete m_mangler;

	m_mangler = new DataMangler(m_database_manager,
			m_xdraw,
			m_ydraw,
			m_start_time.GetTime(),
			m_end_time.GetTime(),
			m_period,
			this,
			m_avg_check->IsChecked());

	m_mangler->Go();
}

void XYDialog::DataFromMangler(XYGraph *graph) {
	if (graph->m_points_values.size() == 0) {
		wxMessageBox(_("There is no common data in selected draws from given time span"), _("No common data"), wxICON_INFORMATION | wxOK, this);
		delete graph;
		return;
	}

	m_frame->SetGraph(graph);

	Show(false);
}

void XYDialog::ConfigurationIsAboutToReload(wxString prefix) {
	wxButton *button;

	if (m_xdraw && m_xdraw->GetBasePrefix() == prefix) {
		delete m_mangler;
		m_mangler = NULL;

		m_xdraw = NULL;

		button = wxDynamicCast(FindWindow(XY_XAXIS_BUTTON), wxButton);
		button->SetLabel(_T(""));
	}

	if (m_ydraw && m_ydraw->GetBasePrefix() == prefix) {
		delete m_mangler;
		m_mangler = NULL;

		m_ydraw = NULL;

		button = wxDynamicCast(FindWindow(XY_YAXIS_BUTTON), wxButton);
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
				10,
				-1,
				-1
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

void XYDialog::OnDrawChange(wxCommandEvent &event) {
	DrawInfo **draw;
	wxButton *button;

	if (event.GetId() == XY_XAXIS_BUTTON) {
		button = wxDynamicCast(FindWindow(XY_XAXIS_BUTTON), wxButton);
		draw = &m_xdraw;
	} else {
		button = wxDynamicCast(FindWindow(XY_YAXIS_BUTTON), wxButton);
		draw = &m_ydraw;
	}

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

DataMangler::DataMangler(DatabaseManager* db, DrawInfo *dx, DrawInfo *dy, wxDateTime start_time, wxDateTime end_time, PeriodType period, XYDialog *dialog, bool average) :
		DBInquirer(db),
		m_dx(dx),
		m_dy(dy),
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
	boost::tuple<ValueInfo, ValueInfo, DTime> tmptuple(ValueInfo(), ValueInfo(), tmpdate);
	m_draw_vals.resize(vs, tmptuple);

	m_dialog = dialog;

	m_average = average;

}	

void DataMangler::Go() {
	m_tofetch = m_draw_vals.size() * 2;
	m_fetched = 0;
	m_pending = 0;

	m_progress = new ProgressFrame(m_dialog);
	m_progress->Show(true);
	m_progress->Raise();

	ProgressFetch();
}

void DataMangler::ProgressFetch() {
	TimeIndex idx(m_period);

	DatabaseQuery *q_x = CreateDataQuery(m_dx, m_period, 0);
	DatabaseQuery *q_y = CreateDataQuery(m_dy, m_period, 1);

	while (m_pending < 200 && m_current_time <= m_end_time) {
		AddTimeToDataQuery(q_x, m_current_time.GetTime().GetTicks());
		AddTimeToDataQuery(q_y, m_current_time.GetTime().GetTicks());

		m_pending += 2;
		m_current_time = m_current_time + idx.GetTimeRes() + idx.GetDateRes();
	}

	QueryDatabase(q_x);
	QueryDatabase(q_y);
}

void DataMangler::DatabaseResponse(DatabaseQuery *q) {

	for (std::vector<DatabaseQuery::ValueData::V>::iterator i = q->value_data.vv->begin();
			i != q->value_data.vv->end();
			i++) {
		DTime time = DTime(m_period, i->time);

		int index = m_start_time.GetDistance(time);

		ValueInfo *vi;
		if (q->draw_no == 0) 
			vi = &m_draw_vals.at(index).get<0>();
		else 
			vi = &m_draw_vals.at(index).get<1>();

		m_draw_vals.at(index).get<2>() = time;

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

		AssociateValues(graph);
		FindMaxMinRange(graph);

		graph->m_start = m_start_time.GetTime();
		graph->m_end = m_end_time.GetTime();
		graph->m_period = m_period;
		graph->m_dx = m_dx;
		graph->m_dy = m_dy;
		graph->m_averaged = m_average;

		if (m_average)
			AverageValues(graph);

		m_progress->Destroy();
		m_dialog->DataFromMangler(graph);
	} if (m_pending <= 100 && m_pending + m_fetched != m_tofetch) {
		ProgressFetch();
	}

}

void DataMangler::AverageValues(XYGraph *graph) {
	double pvx = pow(0.1, m_dx->GetPrec());

	int count;
	double slice;
	double dif = graph->m_dxmax - graph->m_dxmin;
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
		double& x = graph->m_points_values.at(i).get<0>();
		double& y = graph->m_points_values.at(i).get<1>();
		std::vector<DTime> &times = graph->m_points_values.at(i).get<2>();

		int idx = int((x - graph->m_dxmin - slice / 2) * count / dif);
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

		double x = graph->m_dxmin + dif * i / count;
		double y = sums[i] / c;

		graph->m_points_values.push_back(XYPoint(x, y, ptimes[i]));
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

	graph->m_dxmin = graph->m_xmin;
	graph->m_dxmax = graph->m_xmax;

	graph->m_dymin = graph->m_ymin;
	graph->m_dymax = graph->m_ymax;

	ApplyPPAlgorithm(graph->m_dxmin, graph->m_dxmax, m_dx->GetPrec());
	ApplyPPAlgorithm(graph->m_dymin, graph->m_dymax, m_dy->GetPrec());

}

void DataMangler::AssociateValues(XYGraph *graph) {

	int count = 0;

	std::map< double , std::map< double , std::vector<DTime> > > points;

	for (size_t i = 0; i < m_draw_vals.size(); ++i) {
		ValueInfo& vx = m_draw_vals.at(i).get<0>();
		ValueInfo& vy = m_draw_vals.at(i).get<1>();
		DTime time = m_draw_vals.at(i).get<2>();
		
		if (!vx.IsData() || !vy.IsData())
			continue;

		points[vx.val][vy.val].push_back(time);

		if (count != 0) {
			graph->m_xmin = wxMin(graph->m_xmin, vx.val);
			graph->m_xmax = wxMax(graph->m_xmax, vx.val);

			graph->m_ymin = wxMin(graph->m_ymin, vy.val);
			graph->m_ymax = wxMax(graph->m_ymax, vy.val);
		} else {
			graph->m_xmin = 
			graph->m_xmax = vx.val;

			graph->m_ymin = 
			graph->m_ymax = vy.val;
		}

		count++;

	}

	std::map< double , std::map< double , std::vector<DTime> > >::iterator i;
	std::map< double , std::vector<DTime> >::iterator j;

	for(i = points.begin(); i != points.end(); i++) {
		for(j = i->second.begin(); j != i->second.end(); j++) {

			XYPoint tmp (i->first, j->first);
			tmp.get<2>() = j->second;
			graph->m_points_values.push_back(tmp);

		}
	}

}

void XYDialog::OnCancel(wxCommandEvent &event) {
	Show(false);
	if (m_frame == NULL)
		Destroy();	
}

void XYDialog::OnShow(wxShowEvent &event) {
	wxWindow *w = FindWindowById(XY_XAXIS_BUTTON);
	w->SetFocus();
}

void XYDialog::OnHelpButton(wxCommandEvent &event) {
	wxHelpProvider::Get()->ShowHelp(this);
}

DrawInfo* DataMangler::GetCurrentDrawInfo() {
	return m_dx;
}

time_t DataMangler::GetCurrentTime() {
	return -1;
}


BEGIN_EVENT_TABLE(XYDialog, wxDialog) 
	EVT_BUTTON(XY_START_TIME, XYDialog::OnDateChange)
	EVT_BUTTON(wxID_OK, XYDialog::OnOK)
	EVT_BUTTON(wxID_CANCEL, XYDialog::OnCancel)
	EVT_BUTTON(wxID_HELP, XYDialog::OnHelpButton)
	EVT_BUTTON(XY_END_TIME, XYDialog::OnDateChange)
	EVT_CHOICE(XY_CHOICE_PERIOD, XYDialog::OnPeriodChange)
	EVT_BUTTON(XY_XAXIS_BUTTON, XYDialog::OnDrawChange)
	EVT_BUTTON(XY_YAXIS_BUTTON, XYDialog::OnDrawChange)
END_EVENT_TABLE()
