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

#ifndef __TESTRUNNER_H__
#define __TESTRUNNER_H__

#include "drawobs.h"

class TestRunner;

class Test : public DrawObserver {
protected:
	DrawsController *m_dc;
	ConfigManager *m_cfg_mgr;
	TestRunner *m_runner;

	virtual void DataReady() {};
public:
	bool SetupController(wxString prefix, wxString set, wxString graph, PeriodType pt, const wxDateTime& time);

	virtual void NewData(Draw* draw, int idx);
	virtual void Init(TestRunner *runner, DrawsController* dc, ConfigManager* cfg);
	virtual void Teardown(DrawsController* dc);
	virtual void Start() = 0;
	virtual const char* TestName() const = 0;

	static int s_assertions_tested;
	static int s_assertions_failed;
};

class TestRunner : public wxEvtHandler {
	DatabaseManager *m_database_manager;
	ConfigManager *m_config_manager;

	bool m_started;
	size_t m_current_test;

	DrawsController *m_controller;

	void SetupEnv();
	void TearDown();
	void RunNextTest();
public:
	TestRunner(DatabaseManager* database_manager, ConfigManager* config_manager);

	void TestDone();
	void Run();
	void OnIdle(wxIdleEvent& e);

	DECLARE_EVENT_TABLE();
};

#endif
