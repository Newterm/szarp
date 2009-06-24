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
 * Simple WX 'about' dialog with HTML content, based on about.cpp from
 * wxWindows samples.
 *
 * $Id$
 *
 * Pawe³ Pa³ucha <pawel@praterm.com.pl>

 */

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif
// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

/* Remember to load image handlers before displaying HTML ! */

#include "wx/image.h"
#include "wx/imagpng.h"
#include "wx/wxhtml.h"
#include "wx/statline.h"

#include "htmlabout.h"

bool HtmlAbout(wxWindow* parent, wxString file,wxString version)
   {
        wxBoxSizer *topsizer;
        wxHtmlWindow *html;
        wxDialog dlg(parent, -1, wxString(_("About")));
	wxStaticText * ver = new wxStaticText(&dlg,-1,version);

        topsizer = new wxBoxSizer(wxVERTICAL);
	
	
	
        html = new wxHtmlWindow(&dlg, -1, wxDefaultPosition, wxSize(380, 160), 
		wxHW_SCROLLBAR_NEVER);
        html->SetBorders(0);
        if (html->LoadPage(file) == FALSE)
		return FALSE;
	
        html->SetSize(html->GetInternalRepresentation()->GetWidth(), 
                        html->GetInternalRepresentation()->GetHeight());
	topsizer->Add(ver, 0,wxCENTER,1);
        topsizer->Add(html, 1, wxALL, 10);

        topsizer->Add(new wxStaticLine(&dlg, -1), 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
                        
        wxButton *b1 = new wxButton(&dlg, wxID_OK, _("OK"));
        b1->SetDefault();

        topsizer->Add(b1, 0, wxALL | wxALIGN_RIGHT, 15);

        dlg.SetAutoLayout(TRUE);
        dlg.SetSizer(topsizer);
        topsizer->Fit(&dlg);

        dlg.ShowModal();
	return TRUE;
    }

