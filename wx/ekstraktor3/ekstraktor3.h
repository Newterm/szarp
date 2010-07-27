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
 * ekstrakta3 program
 * SZARP

 * vooyeck@praterm.com.pl

 * $Id$
 */

#ifndef __EKSTRAKTOR3__H_
#define __EKSTRAKTOR3__H_

// For compilers that support precompilation, includes "wx/wx.h".
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

#include <wx/cmdline.h>
#include <wx/filename.h>
#include <wx/config.h>
#include <wx/tokenzr.h>
#include "libpar.h"

// the application widget
#include "EkstraktorWidget.h"
#include "geometry.h"
#include "cconv.h"
#include "szapp.h"
#include "cfgdlg.h"
#include "version.h"

/**
 * Main application class.
 */

class EkstrApp: public szApp
{
	wxLocale locale;

	wxString base;

	wxString geometry;
  protected:
	virtual bool OnInit();
  public:
	virtual bool OnCmdLineError(wxCmdLineParser &parser);

	virtual bool OnCmdLineHelp(wxCmdLineParser &parser);

	virtual bool OnCmdLineParsed(wxCmdLineParser &parser);

	virtual void OnInitCmdLine(wxCmdLineParser &parser);
}; 

DECLARE_APP(EkstrApp)

#endif //__EKSTRAKTOR3__H_
