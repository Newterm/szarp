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
/*
 * draw3
 * SZARP
 
 * Paweł Pałucha pawel@praterm.com.pl
 *
 * $Id$
 */

#ifndef __TESTAPP_H__
#define __TESTAPP_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#include <wx/glcanvas.h>
#endif

#include <map>

#include "szapp.h"
#include "singleinstance.h"
#include "szhlpctrl.h"

#include "wxlogging.h"

class TestRunner;

class TestApp : public szApp<wxApp>
{
			/**<Object representing thread that executes queries*/
	QueryExecutor* m_executor;

			/**Database manager object*/
	DatabaseManager* m_dbmgr;

			/**Queries queue*/
	DatabaseQueryQueue* m_db_queue;

			/**Remarks handling object*/
	RemarksHandler* m_remarks_handler;

	ConfigManager *m_cfg_mgr;

	TestRunner *m_runner;

	void StopThreads();
public:
#if 0
	virtual int OnRun();
#endif

	virtual bool OnInit();

	virtual int OnExit();

	virtual bool OnCmdLineError(wxCmdLineParser &parser);

	virtual bool OnCmdLineHelp(wxCmdLineParser &parser);

	virtual bool OnCmdLineParsed(wxCmdLineParser &parser);

	virtual void OnInitCmdLine(wxCmdLineParser &parser);

	TestApp();

	virtual ~TestApp();
};

DECLARE_APP(TestApp)

#endif
