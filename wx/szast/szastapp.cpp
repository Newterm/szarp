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

#include "config.h"
#include "conversion.h"

#ifndef MINGW32
#include <sys/types.h>
#include <grp.h>
#endif

#include <wx/image.h>
#include <wx/cmdline.h>
#include <wx/xrc/xmlres.h>

#include <libxml/tree.h>

#include "szastapp.h"
#include "szastframe.h"
#include "geometry.h"
#include "cconv.h"
#include "libpar.h"

extern void InitXmlResource();

bool SzastApp::OnInit()
{
	szApp::OnInit();
	/* Set locale. */
	this->InitializeLocale(_T("szast"), locale);

	/* Set logging to stderr. */
	wxLog *logger = new wxLogStderr();
	//wxLog *logger = new wxLogGui();
	wxLog::SetActiveTarget(logger);


        /* PNG handler - needed for help displaying */
#if wxUSE_LIBPNG
	wxImage::AddHandler( new wxPNGHandler );
#endif

        /* Initalize XML parser */
        xmlInitParser();
        
        /* Parse command line. */
	if (ParseCommandLine())
		return 0;

	InitXmlResource(); 

#ifndef MINGW32
	if (CheckGroup() == true)
		return FALSE;
#endif
        
	SzastFrame *frame = new SzastFrame(NULL, wxID_ANY);
	frame->Show( TRUE );
	
	SetAppName(_("SZAST"));
	
	SetTopWindow( frame );

	return TRUE;
}

#ifndef MINGW32
bool SzastApp::CheckGroup() {
	wxString user = wxGetUserId();
	if (user == _T("root"))
		return false;


	struct group* dialout_group = getgrnam("dialout");
	if (dialout_group == NULL)
		return false;

	for (char **gn = dialout_group->gr_mem; *gn; gn++)
		if (SC::A2S(*gn) == user)
			return false;
	int ret = wxMessageBox(wxString::Format(_("User %s is not a member of 'dialout' group. This probably means that you won't have access to a serial port.\n"
				"Would you like to add to user %s to a a 'dialout' group? (You'll have to type administrator password)"), user.c_str(), user.c_str()),
				_("Question."),
				wxYES_NO | wxICON_QUESTION);

	if (ret != wxYES)
		return false;

	libpar_init();
	libpar_init_with_filename(const_cast<char*>(SC::S2A((GetSzarpDir() + _T("resources/szarp.cfg"))).c_str()), 0);
	char* _sucommand = libpar_getpar("szast", "su_command", 0);
	wxString sucommand = SC::A2S(_sucommand);
	free(_sucommand);
	libpar_done();

	ret = wxExecute(sucommand + _T(" -- /usr/sbin/usermod -a -G dialout ") + user + _T(""), wxEXEC_SYNC);
	if (ret == 0) {
		wxMessageBox(wxString::Format(_("User %s was added to dialout group. You must run application again for this change to take effect"), user.c_str()), 
				_("Information."), wxICON_INFORMATION); 
		return true;
	} else {
		wxMessageBox(wxString::Format(_("User %s was not added to dialout group"), user.c_str()), _("Error."), wxICON_ERROR); 
		return false;
	}

}
#endif


int SzastApp::ParseCommandLine()
{
	wxCmdLineParser parser;
	wxString geometry;

	/* Set command line info. */
        parser.SetLogo(_("SZAST version 1.0."));
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

IMPLEMENT_APP(SzastApp)
