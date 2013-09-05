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

#include "conversion.h"

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


#define TEST_ASSERT(x) \
	{ \
		Test::s_assertions_tested++; \
		if (!(x)) { \
			Test::s_assertions_failed++; \
			std::cerr << "Test " << TestName() << " failed (" << __PRETTY_FUNCTION__ << "), assertion " #x " is false" << std::endl; \
		} \
	}

int Test::s_assertions_tested = 0;
int Test::s_assertions_failed = 0;

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
protected:
	void DataReady();
public:
	void Start();
	virtual const char *TestName() const { return "Basic data fetch test"; }
};

void Test1::Start() {
	bool ret = SetupController(L"testconfig", L"Temperatury sieciowe", SC::U2S(BAD_CAST "Temp. odniesienia"),
			PERIOD_T_DAY, wxDateTime(1, wxDateTime::Jan, 2000));
	TEST_ASSERT(ret);
}

void Test1::DataReady() {
	std::cout << "Running test 1" << std::endl;

	double expected[][24 * 6] = {
{ 50.9, 51, 51, 51.1, 51.1, 51.2, 51.4, 51.5, 51.6, 51.5, 51.6, 51.6, 51.6, 51.5, 51.3, 51.3, 51.2, 51.2, 51.3, 51.4, 51.5, 51.6, 51.8, 52, 52.1, 52.3, 52.4, 52.4, 52.3, 52.4, 52.5, 52.5, 52.6, 52.7, 52.7, 52.7, 52.7, 52.8, 52.9, 53, 52.9, 53, 53, 53, 53, 53.1, 53.1, 53.1, 53.1, 53, 53.1, 53.1, 53, 53, 53, 53, 52.9, 52.9, 52.8, 52.8, 52.8, 52.7, 52.7, 52.5, 52.5, 52.3, 52.2, 52.2, 52, 51.9, 51.9, 51.7, 51.6, 51.5, 51.6, 51.5, 51.3, 51.2, 51.2, 51.2, 51.1, 51.1, 51.1, 50.9, 50.9, 51, 50.9, 50.9, 50.8, 50.8, 50.6, 50.7, 50.6, 50.5, 50.5, 50.6, 50.5, 50.5, 50.6, 50.6, 50.6, 50.7, 50.7, 50.8, 50.8, 50.7, 50.9, 50.8, 51, 50.9, 50.9, 51, 51.1, 51.2, 51.2, 51.4, 51.5, 51.6, 51.6, 51.6, 51.7, 51.8, 51.8, 51.7, 51.6, 51.7, 51.6, 51.7, 51.8, 51.8, 51.7, 51.6, 51.7, 51.7, 51.8, 51.8, 51.5, 51.7, 51.8, 51.7, 51.6, 51.6, 51.7, 51.7 },
{ 81.2, 81.7, 81.8, 82.4, 82.6, 83.4, 83.2, 83.7, 85.1, 85.2, 85.3, 86, 85.4, 85.1, 85.2, 84.6, 84.6, 84.9, 85.1, 85.1, 85, 85.2, 85.3, 85.5, 85.6, 85.9, 86, 85.9, 86.2, 86, 85.7, 85.3, 85.4, 85.7, 85.7, 85.7, 85.8, 85.9, 86.4, 86.4, 86.3, 85.8, 85.4, 85.6, 85.8, 85.7, 86.2, 86.1, 86.3, 86.2, 86, 87, 86.2, 85.9, 86.2, 86.4, 86, 85.9, 85.6, 85.6, 85.8, 85.5, 85.8, 85.2, 85, 84.5, 84.8, 85.1, 85.2, 85, 85.3, 85.1, 84.9, 84.9, 84.1, 84.1, 84.1, 84.2, 84, 83.4, 83.4, 83.4, 83.4, 82.5, 82.2, 82.4, 83, 84.1, 84.4, 84.4, 83.9, 83.7, 83.5, 83.4, 83, 83.3, 83.8, 84.5, 84.4, 84.7, 84.8, 84.9, 86.7, 87.5, 87, 86.5, 86, 85.7, 85.2, 85.1, 85.4, 85.4, 85.5, 85.5, 85.8, 85.5, 85.7, 85.9, 85.9, 85.8, 85.8, 85.7, 86, 85.8, 85.6, 85.5, 85.6, 85.4, 86.1, 86.1, 85.4, 85.7, 86, 85.9, 85.8, 85.8, 85.9, 85.6, 85.7, 86, 86, 86.2, 86.9, 88 },
{ -0.3, -0.2, -0.1, -0.2, -0.2, -0.2, -0.2, -0.3, -0.2, -0.2, -0.2, -0.2, -0.2, -0.3, -0.2, -0.2, -0.2, -0.1, -0.1, -0.1, -0.2, -0.2, -0.3, -0.3, -0.3, -0.2, -0.3, -0.4, -0.4, -0.3, -0.2, -0.3, -0.1, -0.1, -0.2, -0.2, -0.1, -0.1, -0.2, -0.2, -0.1, -0.1, -0.2, -0.3, -0.2, -0.3, -0.3, -0.2, -0.2, -0.2, -0.2, -0.4, -0.3, -0.4, -0.5, -0.6, -0.5, -0.4, -0.3, -0.3, -0.1, 0, -0.3, -0.3, -0.2, -0.1, -0.2, -0.3, -0.4, -0.3, -0.2, -0.2, -0.2, -0.1, -0.3, -0.3, -0.4, -0.3, -0.5, -0.4, -0.4, -0.3, -0.5, -0.6, -0.5, -0.4, -0.5, -0.6, -0.4, -0.5, -0.6, -0.6, -0.8, -0.9, -0.9, -1, -1.1, -1.1, -1.1, -1.2, -1.3, -1.3, -1.2, -1.3, -1.3, -1.4, -1.4, -1.5, -1.6, -1.7, -1.7, -1.7, -1.7, -2, -2.1, -2.2, -2.3, -2.4, -2.5, -2.6, -2.7, -3, -3, -2.9, -3, -2.9, -2.9, -2.9, -3, -3.2, -3.3, -3.5, -3.5, -3.8, -3.7, -3.8, -3.6, -3.7, -3.6, -3.8, -4, -4.1, -4.2, -4.3 },
{ -0.9, -0.9, -0.9, -0.8, -0.7, -0.7, -0.7, -0.7, -0.7, -0.6, -0.6, -0.6, -0.6, -0.6, -0.6, -0.5, -0.4, -0.4, -0.4, -0.4, -0.4, -0.3, -0.3, -0.3, -0.3, -0.3, -0.3, -0.2, -0.2, -0.2, -0.2, -0.2, -0.2, -0.1, -0.1, -0.1, -0.1, -0.1, -0.1, -0.1, -0.1, -0.1, -0.1, -0.1, -0.1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -0.1, -0.1, -0.1, -0.1, -0.1, -0.1, -0.1, -0.1, -0.1, -0.1, -0.1, -0.1, -0.1, -0.1, -0.1, -0.1, -0.1, -0.1, -0.2, -0.2, -0.2, -0.2, -0.2, -0.2, -0.2, -0.2, -0.2, -0.2, -0.2, -0.2, -0.3, -0.3, -0.3, -0.3, -0.3, -0.3, -0.4, -0.4, -0.4, -0.4, -0.4, -0.4, -0.5, -0.5, -0.5, -0.5, -0.5, -0.5, -0.6, -0.6, -0.6, -0.6, -0.6, -0.6, -0.8, -0.8, -0.8, -0.8, -0.8, -0.8, -0.9, -0.9 },
{ -0.3, -0.3, -0.3, -0.2, 0, 0, 0, 0, 0, 0, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.7, 0.7, 0.7, 0.7, 0.7, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.4, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.1, 0, 0, 0, 0, 0, 0, -0.1, -0.1, -0.1, -0.1, -0.1, -0.1, -0.1, -0.1, -0.1, -0.1, -0.1, -0.1, -0.2, -0.2, -0.2, -0.2, -0.2, -0.2, -0.3, -0.3, -0.3, -0.3, -0.3, -0.3, -0.4, -0.4, -0.4, -0.4, -0.4, -0.4, -0.7, -0.7, -0.7, -0.7, -0.7, -0.7, -0.9, -0.9, -0.9, -0.9, -0.9, -0.9, -1.1, -1.1, -1.1, -1.1, -1.1, -1.1, -1.4, -1.4, -1.4, -1.4, -1.4, -1.4, -1.7, -1.7 },
{ 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0, 0, 0, 0, 0, 0, 0, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.2, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0, 0.1, 0, 0, 0, 0, 0, 0.1, 0, 0, 0, 0, 0, 0, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.2, 0.2, 0.2, 0.2, 0.3, 0.2, 0.1, 0.1, 0.1, 0.1, 0.2, 0.2, 0.2, 0.2, 0.3, 0.5, 0.3, 0.1, 0.1, 0.1, 0.2, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0, 0, -0.1, -0.2, -0.3, -0.4, -0.4, -0.5, -0.6, -0.6, -0.7, -0.7, -0.7, -0.7, -0.7, -0.7, -0.7, -0.9, -1.1, -1.1, -1.2, -1.3, -1.3, -1.3, -1.4, -1.5, -1.7, -1.7, -1.8, -2, -2, -2.2, -2.2, -2.3, -2.4, -2.6, -2.6, -2.6, -2.5, -2.4, -2.4, -2.6, -2.6, -2.9, -3.2, -3.1, -3.2, -3.3, -3.4, -3.4, -3.4, -3.3, -3.3, -3.4, -3.6, -3.6, -3.7, -3.8 },
{ 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62 },
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.1, 0.1, 0.1, 0.1, 0.1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.4, 0.4, 0.4, 0.4, 0.4, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3 },
};
	
	TEST_ASSERT(sizeof(expected)/sizeof(expected[0]) == m_dc->GetDrawsCount());
	for (size_t i = 0; i < m_dc->GetDrawsCount(); i++) {
		Draw* draw = m_dc->GetDraw(i);	
		const Draw::VT& vt = draw->GetValuesTable();
		TEST_ASSERT(vt.size() == sizeof(expected[i]) / sizeof(expected[i][0]));
		for (size_t j = 0; j < vt.size(); j++) {
			const ValueInfo& vi = vt[j];
			TEST_ASSERT(vi.val == expected[i][j]);
		}
	}
	m_runner->TestDone();
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
	if (m_current_test < sizeof(tests)/sizeof(tests[0])) {
		std::cerr << "Executing test " << tests[m_current_test]->TestName() << std::endl;
		tests[m_current_test]->Init(this, m_controller, m_config_manager);	
		tests[m_current_test]->Start();
	} else {
		std::cerr << std::endl << std::endl;
		std::cerr << "Tests execution completed, executed " << m_current_test << " tests" << std::endl;
		std::cerr << Test::s_assertions_tested << " assertions tested of which " << Test::s_assertions_failed
			<< " failed" << std::endl;
		wxExit();
	}
}

void TestRunner::TestDone() {
	std::cerr << "Done executing test " << tests[m_current_test]->TestName() << std::endl;
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
