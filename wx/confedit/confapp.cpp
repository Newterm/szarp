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
 * confedit - wxWindows SZARP configuration editor
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
	szApp::OnInit();

	this->InitializeLocale(_T("ipkedit"), locale);

        /* PNG handler - needed for help displaying */
#if wxUSE_LIBPNG
	wxImage::AddHandler( new wxPNGHandler );
#endif
        
        /* Initalize XML parser */
        xmlInitParser();
        
        /* Parse command line. */
	if (ParseCommandLine())
		return 0;
        
	ConfFrame *frame = new ConfFrame(filename, wxPoint(x, y), 
                        wxSize(width, height) );
	frame->Show( TRUE );
	
	SetAppName(_("IPK Editor"));
	
	SetTopWindow( frame );
	return TRUE;
}


int ConfApp::ParseCommandLine()
{
	wxCmdLineParser parser;
	wxString geometry;

	/* Set command line info. */
        parser.SetLogo(_("IPK Editor version 1.0."));
	parser.AddOption(_T("geometry"), wxEmptyString, 
		_("X windows geometry specification"), wxCMD_LINE_VAL_STRING);
	parser.AddSwitch(_T("d<name>=<str>"), wxEmptyString,
		_("libparnt value initialization"));
	parser.AddSwitch(_T("h"), _T("help"), _("print usage info"));
        parser.AddParam(_("config file"), wxCMD_LINE_VAL_STRING, 
                wxCMD_LINE_PARAM_OPTIONAL);
	parser.SetCmdLine(argc, argv);
	if (parser.Parse(false) || parser.Found(_T("h"))) {
		parser.Usage();
		return 1;
	}
        filename = parser.GetParam();

        /* Reads program geometry (X style) */

	x = 100;
	y = 100;
	width = 600;
	height = 400;

        /* Read 'geometry' option. */
	if (parser.Found(_T("geometry"), &geometry)) 
		get_geometry(geometry, &x, &y, &width, &height);
		
	return 0;
}


