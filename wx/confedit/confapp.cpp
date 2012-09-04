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
 * confedit - SZARP configuration editor
 * SZARP, 2002 Pawe³ Pa³ucha
 *
 * $Id$
 */

#include <wx/image.h>
#include <wx/cmdline.h>

#include <libxml/tree.h>

#include "confapp.h"
#include "cframe.h"
#include "geometry.h"
#include "cconv.h"

bool ConfApp::OnInit()
{
	if (!szApp<>::OnInit()) {
		return false;
	}

	this->InitializeLocale(_T("ipkedit"), locale);

        /* PNG handler - needed for help displaying */
#if wxUSE_LIBPNG
	wxImage::AddHandler( new wxPNGHandler );
#endif
        
        /* Initalize XML parser */
        xmlInitParser();
        
        /* Parse command line. */
	//if (ParseCommandLine())
	//	return 0;
        
	ConfFrame *frame = new ConfFrame(filename, wxPoint(x, y), 
                        wxSize(width, height) );
	frame->Show( TRUE );
	
	SetProgName(_("IPK Editor"));
	
	SetTopWindow( frame );
	return true;
}

bool ConfApp::OnCmdLineHelp(wxCmdLineParser &parser) 
{
	parser.Usage();
	return false;
}


bool ConfApp::OnCmdLineParsed(wxCmdLineParser &parser) 
{
	if (parser.GetParamCount() > 0)
		filename = parser.GetParam(0);
	return true;
}


void ConfApp::OnInitCmdLine(wxCmdLineParser &parser) 
{
        parser.SetLogo(_("IPK Editor version 2.0."));
	parser.AddSwitch(_T("h"), _T("help"), _("print usage info"), wxCMD_LINE_OPTION_HELP);
        parser.AddParam(_("config file"), wxCMD_LINE_VAL_STRING, 
                wxCMD_LINE_PARAM_OPTIONAL);
}


