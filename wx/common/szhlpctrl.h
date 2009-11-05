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

#ifndef __SZHLPCTRL_H__
#define __SZHLPCTRL_H__

#include <wx/wxprec.h>
#include <wx/html/helpctrl.h>
#include <wx/cshelp.h>
#include <wx/textfile.h>
#include <wx/string.h>


#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "helpctrlex/helpctrlex.h"

/**
 * Sub-class of wxHtmlHelpController, needed for Szarp Context Help System, and 
 * new Szarp Help resting on HTML books.
 */
class szHelpController: public wxHtmlHelpControllerEx
{
	public:
		/**
		 * It's the same constructor as orginal, but there is one different. Title of 
		 * help window is "Szarp help".
		 * @param style is combination of flags as: wxHF_TOOLBAR, wxHF_CONTENTS etc,
		 * default style contain: wxHF_TOOLBAR | wxHF_CONTENTS | wxHF_INDEX |
		 * wxHF_SEARCH | wxHF_BOOKMARKS | wxHF_PRINT
		 */
		szHelpController(int style = wxHF_DEFAULT_STYLE | wxHF_FRAME);
		/**
		 * Display book page from hhc file if section is number or
		 * use GetId to get to right number of page to display it.
		 * @see GetId
		 */ 
		virtual bool DisplaySection(const wxString& section);
		/**
		 * Almost the same function as orginal, only InitializeContext is added.
		 */
		bool AddBook(const wxString& book);
	protected:
		/**
		 * Read map file and add the number of page and name of section 
		 * to map_id structure.
		 * @params is path to the map file.
		 */
		bool InitializeContext(const wxString& filepath);
	private:
		/** 
		 * Return the number of page of given section. If section not find
		 * always return "1" (the first page of book). 
		 */
		int GetId(const wxString& text);
	
		/**
		 * Structure contain number of page and name of section, also
		 * pointer to next element. One-way list.
		 */ 
		struct map_id {
			int id;
			wxString section;
			struct map_id *next;
		};
		struct map_id *m_begin_id; /**< pointer on the begining of list. */
};

/** Simple sub-class needed by context help to showing section
 *  from szHelpController
 */ 
class szHelpControllerHelpProvider: public wxHelpControllerHelpProvider 
{
	public:
		/**
	 	* Method used to show context help.
	 	* @param - text is the number of section in hhc file
	 	*/
		virtual bool ShowHelp(wxWindowBase *window);
};

#endif
