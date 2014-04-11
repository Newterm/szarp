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

#include <algorithm>
#include <utility>


#include "ids.h"
#include "classes.h"
#include "drawtime.h"
#include "drawobs.h"
#include "database.h"
#include "dbinquirer.h"
#include "draw.h" 
#include "coobs.h"
#include "drawsctrl.h"
#include "database.h"
#include "dbmgr.h"
#include "cfgmgr.h"
#include "cconv.h"

void print_time(const wxDateTime &s) {
	wxLogWarning(_T("%s"), s.Format().c_str());
}

void Draw::PrintTime(const DTime& s) {
	wxLogWarning(_T("%s"), s.Format().c_str());
}

ValueInfo::ValueInfo() {
	state = EMPTY;
	val = SZB_NODATA;
	sum = SZB_NODATA;
	db_val = SZB_NODATA;
	av_val = SZB_NODATA;
	first_val = SZB_NODATA;
	last_val = SZB_NODATA;
	max_probes = -1;
	count_probes = 0;
	n_count = 0;
	m_remark = false;
}

bool ValueInfo::IsData() const {
	return state == PRESENT && !IS_SZB_NODATA(val);
}

bool ValueInfo::MayHaveData() const {
	return (state == PRESENT && !IS_SZB_NODATA(val))
		|| state == PENDING
		|| state == NEIGHBOUR_WAIT;
}


Draw::Draw(DrawsController* draws_controller, DrawsObservers *observers, int draw_no) : 
	m_draw_no(draw_no),
	m_initial_draw_no(draw_no),
	m_draws_controller(draws_controller), 
	m_draw_info(NULL),
	m_blocked(false),
	m_index(draws_controller->GetPeriod(), draws_controller->GetNumberOfValues(draws_controller->GetPeriod())),
	m_stats_start_time(draws_controller->GetPeriod()),
	m_values(this),
	m_param_has_data(true),
	m_enabled(true),
	m_observers(observers)
{
}

void Draw::RefreshData(bool auto_refresh)
{
	if (auto_refresh && m_draws_controller->GetFilter() == 0) {
		for (size_t i = 0; i < m_values.len(); ++i) {
			ValueInfo &v = m_values.Get(i);

			if (v.state == ValueInfo::PRESENT && !v.IsData())
				m_values.ResetValue(i);
		}
		m_values.RecalculateStats();
	} else
		m_values.clean();

}

void Draw::PrintTimeOfIndex(int i) {
	print_time(GetTimeOfIndex(i));
}

void Draw::SetNumberOfValues(size_t values_number) {
	m_values.SetNumberOfValues(values_number);
	m_index.SetNumberOfValues(values_number);
}

int Draw::GetIndex(const DTime& time, int* dist) const {
	return m_index.GetIndex(time, dist);
}

void Draw::MoveToTime(const DTime& time) {

	const DTime& start_time = m_index.GetStartTime();

	assert(start_time.IsValid());

	if (start_time == time)
		return;

	int d = start_time.GetDistance(time);

	m_index.SetStartTime(time);

	m_values.MoveView(d);

}

void Draw::SetPeriod(const DTime& time, size_t number_of_values) {

	m_index.SetStartTime(time, number_of_values);

	m_values.SetNumberOfValues(number_of_values);

	m_param_has_data = true;

	for (size_t i = 0; i < m_values.len(); ++i)
		m_values.Get(i).state = ValueInfo::EMPTY;
}

void Draw::SetStartTimeAndNumberOfValues(const DTime& start_time, size_t number_of_values) {
	m_index.SetStartTime(start_time);

	m_values.SetNumberOfValues(number_of_values);

	m_param_has_data = true;

	for (size_t i = 0; i < m_values.len(); ++i)
		m_values.Get(i).state = ValueInfo::EMPTY;
}

DTime Draw::GetDTimeOfIndex(int i) const {
	return m_index.GetTimeOfIndex(i);
}

DTime Draw::GetTimeOfIndex(int i) const {
	return m_index.GetTimeOfIndex(i);
}

bool ShouldFetchNewValue(ValueInfo& vi, bool fetch_present_no_data) { 
	if (vi.state == ValueInfo::EMPTY)
		return true;

	if (!fetch_present_no_data)
		return false;

	return vi.state == ValueInfo::PENDING || (vi.state == ValueInfo::PRESENT && !vi.IsData());
}

DatabaseQuery* Draw::GetDataToFetch(bool fetch_present_no_data) {
	const DTime& start_time = m_index.GetStartTime();
	assert(start_time.IsValid());

	PeriodType period = start_time.GetPeriod();

	int d = m_values.m_view.Start();

	DatabaseQuery* q = NULL;

	if (m_draw_info == NULL)
		return q;

	if (GetEnable() == false)
		return q;

	bool no_max_probes = !m_draw_info->IsValid() || m_draw_info->GetParam()->GetIPKParam()->GetFormulaType() == TParam::LUA_AV;
	for (size_t i = 0; i < m_values.len(); ++i) {
		ValueInfo &v = m_values.Get(i);

		if (!ShouldFetchNewValue(v, fetch_present_no_data))
			continue;
		
		DTime pt = GetDTimeOfIndex(i - d);
		if (period == PERIOD_T_DAY)
			v.max_probes = 1;
		else if (no_max_probes)
			v.max_probes = 0;
		else {
			DTime ptn = pt + m_index.GetDateRes() + m_index.GetTimeRes();
			v.max_probes = (ptn.GetTime() - pt.GetTime()).GetMinutes() / 10;
		}

		v.state = ValueInfo::PENDING;
		
		if (q == NULL)
			q = CreateDataQuery(m_draw_info, period, m_draw_no);
		AddTimeToDataQuery(q, pt.GetTime());
	}

	return q;
}

void Draw::SetDraw(DrawInfo *info) {
	m_param_has_data = true;
	m_blocked = false;
	m_draw_info = info;
	for (size_t i = 0; i < m_values.len(); ++i) {
		m_values.Get(i).state = ValueInfo::EMPTY;
		m_values.Get(i).n_count = 0;
		m_values.Get(i).m_remark = false;
	}
	m_values.ClearStats();

}

DrawInfo* Draw::GetDrawInfo() const {
	return m_draw_info;
}

const Draw::VT& Draw::GetValuesTable() const {
	return m_values;
}

bool Draw::GetEnable() const {
	return m_enabled;
}

void Draw::SetEnable(bool enable) {
	if (enable == m_enabled)
		return;
	m_enabled = enable;

}

int Draw::GetDrawNo() const {
	return m_draw_no;
}

int Draw::GetInitialDrawNo() const {
	return m_initial_draw_no;
}

void Draw::SetDrawNo(int draw_no) {
	m_draw_no = draw_no;
}

void Draw::SetInitialDrawNo(int draw_no) {
	m_initial_draw_no = draw_no;
}

bool Draw::GetNoData() const {
	return !m_param_has_data;
}

void Draw::SetNoData(bool no_data) {
	m_param_has_data = !no_data;
}

bool Draw::GetBlocked() const {
	return m_blocked;
}

void Draw::SetBlocked(bool blocked) {
	m_blocked = blocked;
}

int Draw::GetStatsStartIndex() {
	int ret;
	m_index.GetIndex(m_stats_start_time, &ret);
	return ret;
}

void Draw::DragStats(int disp) {
	m_stats_start_time = m_stats_start_time 
		+ m_index.GetTimeRes() * disp 
		+ m_index.GetDateRes() * disp;

	wxLogInfo(_T("Draw no: %d, dragging stats by: %d"), GetDrawNo(), disp);
	m_values.DragStats(disp);
}


void Draw::StartDoubleCursor(int index) {

	m_values.EnableDoubleCursor(index);	
	m_stats_start_time = GetDTimeOfIndex(index);

}

void Draw::StopDoubleCursor() {
	m_values.DisableDoubleCursor();
}

void Draw::SetRemarksTimes(std::vector<wxDateTime>& times) {
	for (std::vector<wxDateTime>::iterator i = times.begin();
			i != times.end();
			i++) {

		DTime dt(m_index.GetStartTime().GetPeriod(), *i);

		int idx = GetIndex(dt);
		if (idx >= 0) {
			ValueInfo& vi = m_values.at(idx);
			if (vi.m_remark == false)
				vi.m_remark = true;
		}
	}
	m_observers->NotifyNewRemarks(this);
}

DrawsController* Draw::GetDrawsController() {
	return m_draws_controller;
}

void Draw::AddData(DatabaseQuery *query, bool &data_within_view) {
	bool stats_updated = false;

	m_values.AddData(query, data_within_view, stats_updated);

	m_param_has_data = true;

	if (stats_updated)
		m_observers->NotifyStatsChanged(this);

}

int Draw::GetCurrentIndex() const {
	if (GetSelected())
		return m_draws_controller->GetCurrentIndex();
	else
		return -1;
}

wxDateTime Draw::GetCurrentTime() {
	if (GetSelected())
		return m_draws_controller->GetCurrentTime();
	else
		return wxInvalidDateTime;
}

bool Draw::GetDoubleCursor() {
	return m_draws_controller->GetDoubleCursor();
}

const PeriodType& Draw::GetPeriod() const {
	return m_draws_controller->GetPeriod();
}

bool Draw::GetSelected() const {
	return m_draws_controller->GetSelectedDraw() == this;
}

DTime Draw::GetStartTime() const {
	return m_index.GetTimeOfIndex(0);
}

DTime Draw::GetLastTime() const {
	return m_index.GetLastTime();
}

TimeInfo Draw::GetTimeInfo() const {
	TimeInfo t;
	t.begin_time = GetStartTime();
	t.end_time = GetLastTime();
	t.period =  GetPeriod();

	return t;
}

void Draw::SwitchFilter(int f, int t)  {
	m_values.SwitchFilter(f, t);
}

const TimeIndex& Draw::GetTimeIndex() {
	return m_index;
}

void Draw::AverageValueCalculationMethodChanged() {
	for (size_t i = 0; i < m_values.len(); ++i) {
		ValueInfo &v = m_values.Get(i);
		if (v.state == ValueInfo::PRESENT || v.state == ValueInfo::NEIGHBOUR_WAIT) {
			switch (m_draw_info->GetAverageValueCalculationMethod()) {
				case AVERAGE_VALUE_CALCULATION_AVERAGE:
					v.db_val = v.av_val;
					break;
				case AVERAGE_VALUE_CALCULATION_LAST:
					v.db_val = v.last_val;
					break;
				case AVERAGE_VALUE_CALCULATION_LAST_FIRST:
					v.db_val = v.last_val - v.first_val;
					break;
			}
			if (v.state == ValueInfo::PRESENT)
				m_values.CalculateProbeValue(i);
		}
	}
	m_values.RecalculateStats();
}

ValuesTable::ValuesTable(Draw *draw) {
	m_draw = draw;
}

ValueInfo& ValuesTable::at(size_t i) {
	return m_values.at(i + m_view.Start());
}

const ValueInfo& ValuesTable::at(size_t i) const {
	return m_values.at(i + m_view.Start());
}

ValueInfo& ValuesTable::operator[](size_t i) {
	return m_values.at(i + m_view.Start());
}

const ValueInfo& ValuesTable::operator[](size_t i) const {
	return m_values.at(i + m_view.Start());
}

size_t ValuesTable::size() const {
	return m_number_of_values;
}

size_t ValuesTable::len() const {
	return m_values.size();
}

void ValuesTable::ClearStats() {
	m_count = 0;
	m_probes_count = 0;
	m_data_probes_count = 0;

	m_sum = 
	m_sum2 = 
	m_sdev = 
	m_max = 
	m_min = 
	m_hsum = 
	m_data_probes_ratio = 
		nan("");
}	

void ValuesTable::RecalculateStats() {
	ClearStats();

	size_t s,e;
	s = m_stats.Start(); 
	e = m_stats.End();

	for (size_t i = s; i <= e; ++i) 
		UpdateStats(i);
}

void ValuesTable::SetNumberOfValues(size_t number_of_values) {

	assert(m_draw->m_draws_controller->GetDoubleCursor() == false);
	m_values.clear();
	m_values.resize(number_of_values + 2 * m_draw->m_draws_controller->GetFilter());
	m_view.m_start = m_draw->m_draws_controller->GetFilter();
	m_view.m_end = number_of_values - 1 + m_draw->m_draws_controller->GetFilter();

	m_stats.m_start = m_draw->m_draws_controller->GetFilter();
	m_stats.m_end = number_of_values - 1 + m_draw->m_draws_controller->GetFilter();

	m_number_of_values = number_of_values;

	ClearStats();
}

Interval Interval::Union(const Interval &i) const {
	Interval ret;

	ret.m_start = std::min(Start(), i.Start());
	ret.m_end = std::max(End(), i.End());
	
	return ret;
}

void Interval::Drag(int d) {
	m_start += d;
}

void Interval::Move(int dr) {
	m_start += dr;
	m_end += dr;
}

int Interval::Start() const {
	return std::min(m_start, m_end);
}

int Interval::End() const {
	return std::max(m_start, m_end);
}

void ValuesTable::PopFront() {
	ValueInfo &vi = m_values.front();
	if (vi.state != ValueInfo::PRESENT && 
			vi.state != ValueInfo::NEIGHBOUR_WAIT) {
		m_values.pop_front();
		return;
	}

	for (int i = 1; i < m_draw->m_draws_controller->GetFilter() && i < (int)m_values.size(); ++i) {
		ValueInfo& vi = m_values.at(i);
		vi.n_count--;
	}

	m_values.pop_front();
}

void ValuesTable::PopBack() {
	ValueInfo &vi = m_values.back();
	if (vi.state != ValueInfo::PRESENT && 
			vi.state != ValueInfo::NEIGHBOUR_WAIT) {
		m_values.pop_back();
		return;
	}

	for (int i = m_values.size() - 1; i > (int)m_values.size() - m_draw->m_draws_controller->GetFilter() - 1 && i >= 0; --i) {
		ValueInfo & vi = m_values.at(i);
		vi.n_count--;
	}

	m_values.pop_back();
	
}

void ValuesTable::PushBack() {
	ValueInfo nvi;
	for (int i = m_values.size() - 1; i > (int)m_values.size() - m_draw->m_draws_controller->GetFilter() - 1 && i >= 0; --i) {
		ValueInfo & vi = m_values.at(i);
		if (vi.state == ValueInfo::PRESENT 
			|| vi.state == ValueInfo::NEIGHBOUR_WAIT)
			nvi.n_count++;
	}

	m_values.push_back(nvi);

}

void ValuesTable::PushFront() {
	ValueInfo nvi;
	for (int i = 0; i < m_draw->m_draws_controller->GetFilter() && i < (int)m_values.size(); ++i) {
		ValueInfo & vi = m_values.at(i);
		if (vi.state == ValueInfo::PRESENT 
			|| vi.state == ValueInfo::NEIGHBOUR_WAIT)
			nvi.n_count++;
	}

	m_values.push_front(nvi);
}

void ValuesTable::Move(const Interval &in) {
	if (in.End() < 0 || in.Start() >= (int)m_values.size()) {
		m_values.resize(0);
		m_values.resize(in.End() - in.Start() + 1);
		return;
	}

	while ((in.End() + 1) < (int)m_values.size())
		PopBack();

	for (int i = 0; i < in.Start(); ++i)
		PopFront();

	for (int i = 0; i > in.Start(); --i)
		PushFront();

	for (int i = m_values.size(); i <= in.End() - in.Start(); ++i) 
		PushBack();
}

void ValuesTable::UpdateStats(int idx) {
	const ValueInfo& v = m_values.at(idx);

	if (v.state != ValueInfo::PRESENT)
		return;

	m_probes_count += v.max_probes;
	m_data_probes_count += v.count_probes;
	if (m_probes_count)
		m_data_probes_ratio = double(m_data_probes_count) / m_probes_count;

	if (!v.IsData())
		return;

	const double& val = v.val;

	bool is_30min = m_draw->m_draws_controller->GetPeriod() == PERIOD_T_30MINUTE;
	if (m_count) {
		m_max = std::max(val, m_max);
		m_min = std::min(val, m_min);
		m_sum += val;
		m_sum2 += val * val;
		if (m_draw->GetDrawInfo() != NULL) {
			double hsum = v.sum / m_draw->GetDrawInfo()->GetSumDivisor();
			if (is_30min)
				hsum /= 60;
			m_hsum += hsum;
		}
	} else {
		m_max = m_min = val;
		m_sum = val;
		m_sum2 = val * val;
		if (m_draw->GetDrawInfo() != NULL)
			m_hsum = v.sum / m_draw->GetDrawInfo()->GetSumDivisor();
		if (is_30min)
			m_hsum /= 60;
	}
	m_count++;

	m_sdev = sqrt(m_sum2 / m_count - m_sum / m_count * m_sum / m_count);
}

void ValuesTable::CalculateProbeValue(int index) {
	double sum = 0.0;
	int count = 0;
	for (int i = - m_draw->m_draws_controller->GetFilter() + index; i <= m_draw->m_draws_controller->GetFilter() + index; ++i) {
		if (i < 0 || i >= (int)m_values.size())
			continue;
		ValueInfo &vi = m_values.at(i);

		if (!IS_SZB_NODATA(vi.db_val)) {
			sum += vi.db_val;
			count++;
		}
	}

	ValueInfo &vi = m_values.at(index);

	if (count) 
		vi.val = IS_SZB_NODATA(vi.db_val) ? SZB_NODATA : sum / count;
	else
		vi.val = SZB_NODATA;

#if 0
	wxLogInfo(_T("Calculated value of probe %d that is %f"), index, vi.val);
#endif
}


void ValuesTable::AddData(DatabaseQuery *q, bool &view_values_changed, bool &stats_updated) {

	DatabaseQuery::ValueData& vd = q->value_data;
	PeriodType pt = vd.period_type;
	if (pt != m_draw->GetPeriod() || q->draw_info != m_draw->GetDrawInfo()) {
		return;
	}

	for (std::vector<DatabaseQuery::ValueData::V>::iterator i = q->value_data.vv->begin();
			i != q->value_data.vv->end();
			i++)
		InsertValue(&(*i), view_values_changed, stats_updated);

}

void ValuesTable::InsertValue(DatabaseQuery::ValueData::V *v, bool &view_values_changed, bool& stats_updated) {
	DTime dt(m_draw->GetPeriod(), ToWxDateTime(v->time_second, v->time_nanosecond));

	int view_index = m_draw->m_index.GetStartTime().GetDistance(dt);
	int index = view_index + m_view.Start();

	if (index < 0 || index >= (int)m_values.size()) 
		return;

	if (m_values.at(index).IsData())
		return;

	ValueInfo& vi = m_values.at(index);

	vi.av_val = v->response;
	vi.sum = v->sum;
	vi.first_val = v->first_val;
	vi.last_val = v->last_val;
	vi.count_probes = v->count;

	switch (m_draw->m_draw_info->GetAverageValueCalculationMethod()) {
		case AVERAGE_VALUE_CALCULATION_AVERAGE:
			vi.db_val = v->response;
			break;
		case AVERAGE_VALUE_CALCULATION_LAST:
			vi.db_val = v->last_val;
			break;
		case AVERAGE_VALUE_CALCULATION_LAST_FIRST:
			vi.db_val = v->last_val - v->first_val;
			break;
	}

	UpdateProbesValues(index, stats_updated, view_values_changed);
}

void ValuesTable::UpdateProbesValues(int index, bool &stats_updated, bool &view_updated) {

	for (int i = index - m_draw->m_draws_controller->GetFilter() ; i <= m_draw->m_draws_controller->GetFilter() + index; ++i) {
		if (i < 0 || i >= (int)m_values.size())
			continue;

		ValueInfo& n = m_values.at(i);
		if (i == index) {
			if (n.n_count != 2 * m_draw->m_draws_controller->GetFilter()) {
				n.state = ValueInfo::NEIGHBOUR_WAIT;
				continue;
			}
		} else {
			++n.n_count;
			if (n.state != ValueInfo::NEIGHBOUR_WAIT || n.n_count < 2 * m_draw->m_draws_controller->GetFilter())
				continue;
		}
	
		n.state = ValueInfo::PRESENT;

		CalculateProbeValue(i);

		if (i >= m_view.Start() && i <= m_view.End()) {
			m_draw->m_observers->NotifyNewData(m_draw, i - m_view.Start());
			view_updated = true;
		}

		if (i >= m_stats.Start() && i <= m_stats.End()) {
			UpdateStats(i);
			stats_updated = true;
		}
	}

}

void ValuesTable::MoveView(int d) { 

	m_view.Move(d);
	
	if (!m_draw->m_draws_controller->GetDoubleCursor())
		m_stats.Move(d);

	Interval in = m_view.Union(m_stats);
	Interval fin = in;
	fin.m_start = fin.Start() - m_draw->m_draws_controller->GetFilter();
	fin.m_end = fin.End() + m_draw->m_draws_controller->GetFilter();

	Move(fin);

	m_view.Move(-in.Start() + m_draw->m_draws_controller->GetFilter());
	m_stats.Move(-in.Start() + m_draw->m_draws_controller->GetFilter());

	if (!m_draw->m_draws_controller->GetDoubleCursor()) {
		RecalculateStats();
		m_draw->m_observers->NotifyStatsChanged(m_draw);
	}

}

void ValuesTable::DragStats(int d) {
	if (!m_draw->m_draws_controller->GetDoubleCursor())
		return;

	if (m_draw->GetDrawInfo() == NULL)
		return;

	m_stats.Drag(d);

	Interval in = m_view.Union(m_stats);
	Interval fin = in;
	fin.m_start -= m_draw->m_draws_controller->GetFilter();
	fin.m_end += m_draw->m_draws_controller->GetFilter();
	Move(fin);

	m_view.Move(-in.Start() + m_draw->m_draws_controller->GetFilter());
	m_stats.Move(-in.Start() + m_draw->m_draws_controller->GetFilter());

	RecalculateStats();

	m_draw->m_observers->NotifyStatsChanged(m_draw);

}

void ValuesTable::EnableDoubleCursor(int idx) {
	m_stats.m_start = m_stats.m_end = idx + m_draw->m_draws_controller->GetFilter();

	if (m_draw->GetDrawInfo() == NULL)
		return;

	RecalculateStats();
}

void ValuesTable::DisableDoubleCursor() {
	for (int i = m_draw->m_draws_controller->GetFilter(); i < m_view.m_start; i++)
		m_values.pop_front();

	m_view.m_end = m_draw->m_draws_controller->GetFilter() + (m_view.m_end - m_view.m_start);
	m_view.m_start = m_draw->m_draws_controller->GetFilter();

	m_stats.m_start = m_view.m_start;
	m_stats.m_end = m_view.m_end;

	if (m_draw->GetDrawInfo() == NULL)
		return;

	RecalculateStats();
}

ValueInfo& ValuesTable::Get(size_t i) {
	return m_values.at(i);
}

const ValueInfo& ValuesTable::Get(size_t i) const {
	return m_values.at(i);
}

void ValuesTable::SwitchFilter(int f, int t) {
	int df = t - f;

	m_view.m_start += df;
	m_view.m_end += df;

	m_stats.m_start += df;
	m_stats.m_end += df;

	for (size_t i = 0; i < m_values.size(); ++i) 
		m_values[i].n_count = 0;

	if (df > 0) 
		while (df--) {
			m_values.push_back(ValueInfo());
			m_values.push_front(ValueInfo());
		}
	else 
		while (df++) {
			m_values.pop_back();
			m_values.pop_front();
		}

	if (m_draw->GetDrawInfo() == NULL)
		return;

	ClearStats();

	bool stats_updated, view_updated;
	for (size_t i = 0; i < m_values.size(); ++i) {
		ValueInfo& v = m_values.at(i);
		if (v.state != ValueInfo::EMPTY && v.state != ValueInfo::PENDING)
			UpdateProbesValues(i, stats_updated, view_updated);
	}

}

void ValuesTable::clean() {
	size_t size = m_values.size();
	m_values.resize(0);
	m_values.resize(size);
	RecalculateStats();
}

void ValuesTable::ResetValue(int i) {
	ValueInfo& v = m_values.at(i);
	m_values.at(i).state = ValueInfo::EMPTY;
	if (m_stats.Start() <= i && i <= m_stats.End()) {
		m_probes_count -= v.max_probes;
		if (m_probes_count)
			m_data_probes_ratio = double(m_data_probes_count) / m_probes_count;
		else
			m_data_probes_ratio = nan("");
	}
}
