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
 * szhlpctrl - SzarpHelpController
 * SZARP

 * Michal Blajerski nameless@praterm.com.pl
 *
 * $Id$
 */


#include <config.h>
#include "szhlpctrl.h"
#include "cconv.h"
#include "szapp.h"

szHelpController::szHelpController(int style) : wxHtmlHelpControllerEx(style)
{
    m_begin_id = NULL;
    SetTitleFormat(wxString(_("SZARP Help")) + _T(": %s"));
}

bool szHelpController::DisplaySection(const wxString& section) {
	int id;

	if (section.IsNumber()) 
		id = wxAtoi(section);
	else 
		id = GetId(section);

	return wxHtmlHelpControllerEx::DisplaySection(id);

}

int szHelpController::GetId(const wxString& section)
{
	map_id *tmp=m_begin_id;
	while (tmp!=NULL)
	{ 
		if(section.CmpNoCase(tmp->section) == 0)
		{
			return tmp->id;
		}
		tmp=tmp->next;
	}
	return 1;
}


bool szHelpController::InitializeContext(const wxString& filepath)
{
	if (wxFileName::FileExists(filepath))
	{   
		m_begin_id = new map_id;
		m_begin_id->next=NULL;
		map_id *tmp_id = m_begin_id;
		wxTextFile *map_file = new wxTextFile;
		wxString tmp_str;
		
		map_file->Open(filepath);
		for (tmp_str = map_file->GetFirstLine(); !map_file->Eof(); tmp_str = map_file->GetNextLine())
		{
	    		tmp_id->id = wxAtoi(tmp_str);
			tmp_id->section += tmp_str;
			tmp_id->section.Remove(0,(tmp_id->section.Find(_T(" ")))+1);
			tmp_id->next = new map_id;
			tmp_id = tmp_id->next;
			tmp_id->next=NULL;
		}
   		tmp_id->id=wxAtoi(tmp_str);
		tmp_id->section += tmp_str;
		tmp_id->section.Remove(0,(tmp_id->section.Find(_T(" ")))+1);
		map_file->Close();
		return TRUE;
	}
	return FALSE;
}


bool szHelpController::AddBook(const wxString& book)
{
    bool retval = wxHtmlHelpControllerEx::AddBook(book); 

    if (retval) 
	return InitializeContext(book.BeforeLast('.') + _T(".map"));
    return FALSE;
}
	
bool szHelpControllerHelpProvider::ShowHelp(wxWindowBase *window)
{
    wxString text = GetHelp(window);
    if (!text.empty() && m_helpController)
    {
         return m_helpController->DisplaySection(text);
    }

    return false;
}

