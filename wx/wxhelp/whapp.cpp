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
 * wxhelp - simple wxWindows HTML help viewer
 * SZARP
 
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 *
 * $Id$
 */

#include <wx/image.h>
#include <string.h>

#include "whapp.h"
#include "cconv.h"


bool WHApp::OnInit()
{
	szApp::OnInit();

	// SET LOCALE 
#ifndef __WXMSW__
        locale.Init();
#else
	locale.Init(wxLANGUAGE_POLISH);
#endif

        locale.AddCatalogLookupPathPrefix(GetSzarpDir() + _T("resources/locales"));

	m_help = NULL;
	frame = NULL;

	locale.AddCatalog(_T("common"));
	locale.AddCatalog(_T("wx"));

        if (!locale.IsOk()) {
                wxLogError(_T("Could not initialize locale."));
                exit(1);
        }
	
	wxInitAllImageHandlers();

	wxString file;
	wxFileName fname;
	if (argc > 1) {
		file = argv[1];
		fname = file;
		if (!fname.IsOk() || !fname.FileExists()) {
			wxMessageBox(wxString(_("Cannot open file ")) + file,
					_("Error opening file"),
					wxOK | wxICON_ERROR);
			return FALSE;
		}
		
		if (file.AfterLast('.') != _T("hhp")) {
			frame = new HtmlViewerFrame(file,
					(wxWindow*)NULL, _("SZARP Help"),
					wxDefaultPosition, wxSize(600,600));
			
			SetAppName(_T("SZARP help viewer"));
			SetTopWindow( frame );
			frame->Show( TRUE );
			return TRUE;
		}
	}	
	
	m_help = new szHelpController;
	
	if (argc > 1) {
		m_help->AddBook(wxFileSystem::FileNameToURL(fname));
		m_help -> DisplayContents();
	} else {
		m_help->AddBook(GetSzarpDir() + _T("resources/documentation/new/eksploatacja/html/eksploatacja.hhp"));
#ifndef __WXMSW__
		m_help->AddBook(GetSzarpDir() + _T("resources/documentation/new/ekstraktor3/html/ekstraktor3.hhp"));
		m_help->AddBook(GetSzarpDir() + _T("resources/documentation/new/draw3/html/draw3.hhp"));
		m_help->AddBook(GetSzarpDir() + _T("resources/documentation/new/raporter/html/raporter.hhp"));
		m_help->AddBook(GetSzarpDir() + _T("resources/documentation/new/kontroler/html/kontroler.hhp"));
#endif
		m_help->AddBook(GetSzarpDir() + _T("resources/documentation/new/scc/html/scc.hhp"));
		m_help->AddBook(GetSzarpDir() + _T("resources/documentation/new/ssc/html/ssc.hhp"));
		m_help->AddBook(GetSzarpDir() + _T("resources/documentation/new/howto/html/howto.hhp"));
		m_help->AddBook(GetSzarpDir() + _T("resources/documentation/new/ipk/html/ipk.hhp"));
#ifndef __WXMSW__
		m_help->AddBook(GetSzarpDir() + _T("resources/documentation/new/ipk/html/ipk.hhp"));
#endif		
		m_help -> DisplayContents();
	}
	
	return TRUE;
}

int WHApp::OnExit()
{
	delete m_help;
	return 0;
}
