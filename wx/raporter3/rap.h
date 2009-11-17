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
 * raporter3 program
 * SZARP

 * pawel@praterm.com.pl
 */

#ifndef __RAP_H__
#define __RAP_H__

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "szapp.h"

/**
 * Main app class
 */
class rapApp: public szApp {
	wxLocale locale;

	virtual bool OnCmdLineError(wxCmdLineParser &parser);

	virtual bool OnCmdLineHelp(wxCmdLineParser &parser);

	virtual bool OnCmdLineParsed(wxCmdLineParser &parser);

	virtual void OnInitCmdLine(wxCmdLineParser &parser);
protected:
	wxString m_server;

	wxString m_title;

	virtual bool OnInit();
	int OnExit();
};


DECLARE_APP(rapApp);

#endif
