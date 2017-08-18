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
/* $Id$
 *
 * reporter3 program
 * SZARP

 * pawel@praterm.com.pl
 */

#ifndef __RAP4_H__
#define __RAP4_H__

#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../common/szapp.h"

#include <memory>
#include "reporter_controller.h"
#include "config_handler.h"
#include "data_provider.h"

/**
 * Main app class
 */
class repApp: public szApp<> {
	wxLocale locale;
	using Reporter = ReportController<ReportsConfigHandler, IksReportDataProvider, ReporterWindow>;

private:
	void InitializeReporter();
	bool ParseCMDLineOptions();

	void SetImageHandler();
	void SetTitle();
	void SetIcon();
	void SetLocale();

	std::string m_server;
	std::string m_port;
	std::wstring m_base;
	std::wstring initial_report;

	std::shared_ptr<ReportControllerBase> reporter;

protected:
	virtual bool OnInit();
	int OnExit();
};


DECLARE_APP(repApp);

#endif
