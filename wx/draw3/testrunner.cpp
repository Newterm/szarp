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
#include <iostream>

#include <wx/wx.h>

#include "ids.h"
#include "classes.h"
#include "coobs.h"
#include "dbinquirer.h"
#include "database.h"
#include "drawtime.h"
#include "draw.h"
#include "drawsctrl.h"
#include "cfgmgr.h"
#include "testrunner.h"

bool Test::SetupController(wxString prefix, wxString set, wxString graph, PeriodType pt, const wxDateTime& time) {
	DrawsSets* draws_sets = m_cfg_mgr->GetConfigByPrefix(prefix);
	if (draws_sets == NULL)
		return false;

	DrawSet* draw_set;
	DrawSetsHash& sets = draws_sets->GetDrawsSets();
	DrawSetsHash::iterator i = sets.find(set);
	if (i != sets.end())
		draw_set = i->second;
	else
		return false;
	
	int draw_no = -1;
	DrawInfoArray* draws_array = draw_set->GetDraws();
	for (size_t i = 0; i < draws_array->size(); i++)
		if ((*draws_array)[i]->GetName() == graph) {
			draw_no = i;
			break;
		}
	if (draw_no == -1)
		return false;

	m_dc->Set(draw_set, pt, time, draw_no);
	return true;
}

void Test::NewData(Draw* , int idx) {
	for (size_t i = 0; i < m_dc->GetDrawsCount(); i++) {
		Draw* draw = m_dc->GetDraw(i);	
		const Draw::VT& vt = draw->GetValuesTable();
		for (size_t j = 0; j < vt.size(); j++) {
			const ValueInfo& vi = vt[j];
			if (vi.state != ValueInfo::PRESENT)
				return;
		}
	}

	DataReady();
}

void Test::Init(TestRunner *runner, DrawsController* dc, ConfigManager* cfg) {
	 m_runner = runner;
	m_cfg_mgr = cfg;
	m_dc = dc;
	dc->AttachObserver(this);
}

void Test::Teardown(DrawsController* dc) { dc->AttachObserver(this); }

class Test1 : public Test {
public:
	void Start();
};

void Test1::Start() {
	bool ret = SetupController(L"leg1", L"Temperatury sieciowe", L"Temp. odniesienia", PERIOD_T_DAY, wxDateTime::Now());
	assert(ret);
}

Test* tests[] = {new Test1,};

TestRunner:: TestRunner(DatabaseManager* database_manager, ConfigManager* config_manager) : m_database_manager(database_manager), m_config_manager(config_manager), m_started(false), m_current_test(0)  {}

void TestRunner::SetupEnv() {
	m_controller = new DrawsController(m_config_manager, m_database_manager);	
}

void TestRunner::Run() {
	SetupEnv();
	RunNextTest();
}

void TestRunner::RunNextTest() {
	if (m_current_test >= sizeof(tests)/sizeof(tests[0]))
		return;

	tests[m_current_test]->Init(this, m_controller, m_config_manager);	
	tests[m_current_test]->Start();
}

void TestRunner::TestDone() {
	m_current_test += 1;
	RunNextTest();
}

void TestRunner::TearDown() {
	delete m_controller;
}

void TestRunner::OnIdle(wxIdleEvent& event) {
	if (m_started)
		return;
	std::cerr << "Tests triggered" << std::endl;
	m_started = true;
}

BEGIN_EVENT_TABLE(TestRunner, wxEvtHandler) 
	EVT_IDLE(TestRunner::OnIdle)
END_EVENT_TABLE()
