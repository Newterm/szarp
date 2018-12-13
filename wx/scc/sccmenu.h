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
 * scc - Szarp Control Center
 * SZARP

 * Pawe³ Pa³ucha pawel@praterm.com.pl
 *
 * $Id$
 */

#ifndef __SCCMENU_H__
#define __SCCMENU_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#include <wx/regex.h>
#endif

//following is needed before standard library headers inclusion
//due to wxWidgets' messing with new operator
#ifdef new
#undef new
#endif
#include <vector>
#include <set>
#include <algorithm>

class SCCMenu;

WX_DEFINE_ARRAY(SCCMenu*, ArrayOfMenu);

#define SCC_MENU_FIRST_ID	(wxID_HIGHEST + 100)
#define SCC_MENU_MAX_ID		(SCC_MENU_FIRST_ID + 1000)

/**
 * Class is an internal representation of SZARP menu. It holds information
 * about menu structure, menu items descriptions and corresponding
 * commands to execute.
 * Usually menu is created by parsing config string with ParseMenu() method.
 * wxMenu object may be created from this information, using CreateWxMenu()
 * method.
 */
class SCCMenu
{
public:
	/**
	 * Builds and returns pointer to newly created SCCMenu object, based on
	 * info from parsed config string.
	 * @param menu menu config string
	 */
	static SCCMenu* ParseMenu(wxString menu);

	/**
	 * Builds and returns pointer to newly created wxMenu object,
	 * corresponding to this object. This wxMenu object is not deleted on
	 * SCCMenu object deleting.
     * @param array string with names (prefixes) of databases to be hidden
	 */
	wxMenu* CreateWxMenu(wxArrayString);

	/**Destroys provided hierachy of menus, note that SCCMenu destructor
	 * is private so this is only possible way to destroy SCCMenu object*/
	static void Destroy(SCCMenu *);
	/**
	 * Appends empty submenu with given name to menu.
	 */
	SCCMenu* AddMenu(wxString name);
	/**
	 * Inserts empty submenu with given MenuHierarchyData and name to menu before given
	 * position.
	 */
	SCCMenu* InsertMenu(wxString name, int pos);
	/**
	 * Appends menu item with given name and command to execute.
	 * @param name menu item title
	 * @param exec command to execute
	 * @param bmp bitmap for item
	 * @return pointer to newly created menu
	 */
	void AddCommand(wxString name, wxString exec = _T(""),
			wxBitmap* bmp = NULL);
	/**
	 * Appends separator to the menu.
	 */
	void AddSeparator();
	/**
	 * Append raports and SZARP draw program for configuration with given
	 * prefix. Configuration must be IPK compatible.
	 * @param prefix configuration prefix
	 */
	void AddConfig(wxString prefix);
	/**
	 * Returns command string attached to menu item with given ID
	 */
	wxString GetCommand(int id);
	/**
	 * Searches for item with given id.
	 * @return item found or NULL
	 */
	SCCMenu* FindItem(int id);


private:
	/** Expands DRAWS_ITEM_STUB using provided regular expression.
	 * This method will not insert draws which are already contained within
	 * menu hierarchy.
	 * @param draws_item SCCMenu item of type DRAWS_ITEM_STUB
	 * @param pos the resulting items are inserterd at position pos in children list
	 */
	void ExplodeDraws(SCCMenu *draws_item,int pos);
	/** Appends given submenu */
	void AddMenu(SCCMenu* menu);

	/**Expands of items of type @see DRAWS_ITEM_STUB*/
	void BuildMenu();

	/**Removes draws which are superseded by another drwaws
	 *and are of type @see REMOVEABLE*/
	void RemoveSuperseded();

	/** MenuHierarchyData contains global data collected during menu hierarchy parsing */
	class MenuHierarchyData {
		public:
		std::vector<wxRegEx*> exclude_expr;	/**< list of expressions which should not be used for draws grouping*/
		std::vector<wxString> draws;		/**< table of draws contained within menu hiererchy */
		std::set<wxString> superseded;		/**< set of draws superseded by another draws*/
		int id;					/**< max menu item id */
	};

	/**
	 * @param hierarchy_data pointer to @see MenuHierarchyData
	 * @param name name of menu (title, description)
	 * @param exec command to execute on item selected
	 */
	SCCMenu(MenuHierarchyData *hierarchy_data, wxString name = _T(""), wxString exec = _T(""));

	/** All submenus are deleted. This destructor is private if you want
	 * to destroy SCCMenu object use @see Destroy method*/
	~SCCMenu();

	MenuHierarchyData* hierarchy_data;

	/** Enumeration types for tokens types.
	 * TOK_STRING - string (in double collons)
	 * TOK_ID - alpha starting identifier
	 * TOK_COMMA - comma,
	 * TOK_LEFT - left paranthesis
	 * TOK_RIGHT - right paranthesis
	 * TOK_END - end of string
	 * TOK_ERROR - error
	 */
	enum { TOK_STRING = 0, TOK_ID, TOK_COMMA, TOK_LEFT, TOK_RIGHT, TOK_END,
		TOK_ERROR };
	/** Enumeration types for tokenizer states.
	 * ST_WS - white space or other non significant
	 * ST_NORMAL - identifier (alpha starting)
	 * ST_STRING - string constant (in double collons)
	 * ST_ESCAPE - escaped charachter within string */
	enum { ST_WS, ST_NORMAL, ST_STRING, ST_ESCAPE };
	/** Menu string tokenizer, divides string into tokens. */
	class Tokenizer {
		public:
		/**
		 * @param str string to tokenize
		 */
		Tokenizer(wxString str);
		/**
		 * Returns next token from string.
		 * @param type retursns type of token (see token enumeration type)
		 * @return token's string, if type is TOK_ERROR returns error
		 * description
		 */
		wxString GetNext(int* type);
		private:
		/** Copy of string to tokenize */
		wxString s;
		size_t pos;	/**< Current tokenizer position */
		int state;	/**< Internal tokenizer state */
		size_t length;	/**< Length of string s */
	};

	/** Method used internally to recursivly parse menus.
	 * @param t pointer to tokenizer with current status
	 * @param name name of menu to create
	 * method
	 * @return pointer to newly created submenu
	 */

	static SCCMenu* ParseMenu(Tokenizer* t, wxString name,MenuHierarchyData *hierarchy_data);

	/**
	 * Appends draw menu item
	 * @param title name of menu item
	 * @param prefix configuration prefix
	 * @param expanded denotes should be of type @see REMOVEABLE
	 */
	void AddDraw(const wxString& title,const wxString& prefix, bool removable = false);

	/**
	 * Creates SCCMenu object describing draw
	 * @param title name of menu item
	 * @param prefix configuration prefix
	 * @param hierarchy_data pointer to a @see MenuHierarchyData object
	 * @param removeable if true menu_type is set to @see REMOVEABLE,
	 * if false menu_type is set to @see ORDINARY_ITEM
	 * @return pointer to created  SCCMenu object
	 */
	static SCCMenu* CreateDrawItem(const wxString& title, const wxString& prefix, MenuHierarchyData *hierarchy_data, bool removable);

	/**
	 * Appends draws menu item stub
	 * @param expr regular expression utlized for expanding stub
	 */
	SCCMenu* AddDraws(wxString expr);
	/**
	 * Appends raport item to menu. Submenus for raport groups are also
	 * created.
	 * @param name name of param, needed for create submenu
	 * @param title title of raport
	 * @param file raport file name
	 * @param prefix configuration prefix
	 * @param pos position where to start searching for existing submenus,
	 * also used for sorting submenus.
	 */
	void AddRaportItem(wxString param, wxString title, wxString file,
			wxString prefix, int pos);
	void AddRaport3Item(wxString param, wxString title, wxString prefix, int pos);
	/**
	 * Adds raporter3 item to the menu. Does it only if
	 * address of xlstd daemon is configured*/
	void AddRaporter3(wxString prefix);
	/**
	 * Adds ekstraktor3 item to the menu.*/
	void AddEkstraktor3(const wxString &prefix);

	int id;			/**< Uniq menu item ID */
	wxString name;		/**< Menu item name (title) */
	wxString exec;		/**< Command to execute, empty for submenus. */
	ArrayOfMenu* children;	/**< List of pointers to children
				  (for submenu), NULL pointers are used for
				  separators. */
	wxBitmap bmp;		/**< Menu item icon. */

	/** Menu type
	 * ORDINARY_ITEM - an item from which wxMenuItem can directly be created
	 * DRAWS_ITEM_STUB - an item which does not map directly to wxMenuItem  - requiers
	 * further processing i.e. should be expanded to draws items by @see ExplodeDraws
	 * REMOVEABLE - a draw item which can be superseded by other draws*/
	enum { ORDINARY_ITEM, DRAWS_ITEM_STUB, REMOVEABLE } menu_type;

	/**Configuration prefix which draw menu item referes to
	 * NULL if none or SCCMenu is not a draw item*/
	wxString* prefix;
};


#endif

