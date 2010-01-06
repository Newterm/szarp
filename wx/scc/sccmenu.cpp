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

#include <wx/config.h>
#include <wx/image.h>
#include "cconv.h"
#include "sccmenu.h"
#include "sccapp.h"
#include "szarp_config.h"
#include "szarpconfigs.h"
#include "libpar.h"
#include <vector>
#include <algorithm>

#define DRAW_ICON_PATH	_T("resources/wx/icons/draw16.xpm")
#define RAP_ICON_PATH	_T("resources/wx/icons/rap16.xpm")
#define EKS_ICON_PATH	_T("resources/wx/icons/extr16.xpm")

SCCMenu::SCCMenu(MenuHierarchyData *mhd, wxString _name, wxString _exec) :
	hierarchy_data(mhd), id(mhd->id), name(_name), exec(_exec), children(NULL),
	menu_type(SCCMenu::ORDINARY_ITEM), prefix(NULL)
{
	children = new ArrayOfMenu();
	wxLog *logger=new wxLogStderr();
	wxLog::SetActiveTarget(logger);

	mhd->id++;
}

SCCMenu::~SCCMenu()
{
	if (children) {
		for (unsigned int i = 0; i < children->GetCount(); i++)
			delete children->Item(i);
		delete children;
	}
	delete prefix;
}

SCCMenu* SCCMenu::AddMenu(wxString name)
{
	SCCMenu* m = new SCCMenu(hierarchy_data, name);
	children->Add(m);
	return m;
}

SCCMenu* SCCMenu::AddDraws(wxString expr)
{
	SCCMenu* m = new SCCMenu(hierarchy_data, expr);
	m->menu_type = DRAWS_ITEM_STUB;
	children->Add(m);
	return m;
}

SCCMenu* SCCMenu::InsertMenu(wxString name, int pos)
{
	SCCMenu* m = new SCCMenu(hierarchy_data, name);
	children->Insert(m, pos);
	return m;
}

void SCCMenu::AddMenu(SCCMenu* menu)
{
	children->Add(menu);
}

void SCCMenu::AddCommand(wxString name, wxString exec, wxBitmap* bmp)
{
	SCCMenu* m = new SCCMenu(hierarchy_data, name, exec);
	if (bmp)
		m->bmp = *bmp;
	children->Add(m);
}

void SCCMenu::AddSeparator()
{
	children->Add(NULL);
}

void SCCMenu::AddConfig(wxString prefix)
{
	if (prefix != wxEmptyString) {
		AddRaporter3(prefix);
	} else {
		wxFileName filename = wxFileName(wxGetApp().GetSzarpDir(), wxEmptyString);
		filename.AppendDir(_T("bin"));
		filename.SetName(_T("raporter3"));
#ifdef __WXMSW__
		filename.SetExt(_T("exe"));
#endif
		wxImage img;
		img.LoadFile(wxGetApp().GetSzarpDir() + RAP_ICON_PATH);
		wxBitmap b(img);
		AddCommand(_T("Raporter"), filename.GetFullPath(), &b);
	}

	bool ekstraktor3 = false;

	char *use_ekstraktor3 = libpar_getpar("scc", "use_ekstraktor3", 0);

	if (use_ekstraktor3 != NULL)
		ekstraktor3 = !strcmp(use_ekstraktor3, "yes");
	free(use_ekstraktor3);

	if (ekstraktor3)
		AddEkstraktor3();

	SzarpConfigs* sconfs = SzarpConfigs::GetInstance();
	TSzarpConfig* sc = sconfs->GetConfig(prefix);
	if (sc != NULL)
		AddDraw(_("Draw program"), prefix);
}

void SCCMenu::AddRaporter3(wxString prefix) {
	wxString path;
	int is_any = 0;
	wxString s;
	std::vector<std::wstring> raports;
	std::wstring c;
	int first_pos;

	if (children)
		first_pos = children->GetCount();
	else
		first_pos = 0;
	SzarpConfigs* sconfs = SzarpConfigs::GetInstance();
	TSzarpConfig* sc = sconfs->GetConfig(prefix);
	if (sc == NULL)
	{
		wxLogError(_T("Could not load configuration for prefix '%s'"),
			prefix.c_str());
		return;
	}
	c = sc->GetFirstRaportTitle();

	for (c = sc->GetFirstRaportTitle(); !c.empty(); c =
			sc->GetNextRaportTitle(c))
		raports.push_back(c);
	std::sort(raports.begin(), raports.end());

	for (unsigned int i = 0; i < raports.size(); i++) {
		std::wstring c = raports[i];
		TRaport* r = sc->GetFirstRaportItem(c);
		if (r == NULL)
			continue;
		TParam *p = r->GetParam();
		AddRaport3Item(wxString(p->GetName()), wxString(r->GetTitle()),
				prefix, first_pos);
		is_any = 1;
	}
	if (is_any)
		AddSeparator();

}

void SCCMenu::AddEkstraktor3() {
	wxFileName filename = wxFileName(wxGetApp().GetSzarpDir(), wxEmptyString);
	filename.AppendDir(_T("bin"));
	filename.SetName(_T("ekstraktor3"));
#ifdef __WXMSW__
	filename.SetExt(_T("exe"));
#endif
	wxString s = filename.GetFullPath();

	wxImage img;
	img.LoadFile(wxGetApp().GetSzarpDir() + EKS_ICON_PATH);
	wxBitmap b(img);

	AddCommand(_T("Ekstraktor"), s, &b);
}

void SCCMenu::AddDraw(const wxString& title, const wxString &prefix, bool removable)
{
	children->Add(CreateDrawItem(title, prefix, hierarchy_data, removable));
}

void SCCMenu::AddRaportItem(wxString param, wxString title,
		wxString file, wxString prefix, int pos)
{
	wxString group = param.BeforeFirst(':');
	int found = 0;
	unsigned int insert = 0;
	SCCMenu* item = NULL;

	if (children) for (unsigned int i = pos; i < children->GetCount();
			i++) {
		item = children->Item(i);
		if (item == NULL)
			continue;
		if (item->name.Cmp(group) == 0) {
			found = 1;
			break;
		}
	}
	if (found == 0) {
		if (children)
			for (insert = pos; ((insert < children->GetCount())
				&& children->Item(insert)
				&& (children->Item(insert)->name.Cmp(group) <
				0)); insert++);
		item = InsertMenu(group, insert);
	}
	group = wxGetApp().GetSzarpDir() + _T("/bin/rap '") + wxGetApp().GetSzarpDataDir() + _T("/")
		+ prefix + _T("/config/") +
		file + _T("' -p -Dprefix=") + prefix;
	wxImage img;
	img.LoadFile(wxGetApp().GetSzarpDir() + RAP_ICON_PATH);
	wxBitmap b(img);
	item->AddCommand(title, group, &b);
}

void SCCMenu::AddRaport3Item(wxString param, wxString title,
		wxString prefix, int pos)
{
	wxString group = param.BeforeFirst(':');
	int found = 0;
	unsigned int insert = 0;
	SCCMenu* item = NULL;

	if (children) for (unsigned int i = pos; i < children->GetCount();
			i++) {
		item = children->Item(i);
		if (item == NULL)
			continue;
		if (item->name.Cmp(group) == 0) {
			found = 1;
			break;
		}
	}
	if (found == 0) {
		if (children)
			for (insert = pos; ((insert < children->GetCount())
				&& children->Item(insert)
				&& (children->Item(insert)->name.Cmp(group) <
				0)); insert++);
		item = InsertMenu(group, insert);
	}

	char* port_c = libpar_getpar("paramd_for_raporter", "port", 0);
	if (port_c == NULL)
	{
		wxLogError(_T("Could not load paramd port number for raporter. Expected 'port' configuration key in section 'paramd_for_raporter'."));
		return;
	}
	wxString port = wxString(SC::A2S(port_c));

	wxFileName filename(wxGetApp().GetSzarpDir(), wxEmptyString);
	filename.AppendDir(_T("bin"));
	filename.SetName(_T("raporter3"));

#ifdef __WXMSW__
	filename.SetExt(_T("exe"));
#endif

	group = filename.GetFullPath() + _T(" ") + prefix + _T(":") + port + _T(" '") + title + _T("'");

	wxImage img;
	img.LoadFile(wxGetApp().GetSzarpDir() + RAP_ICON_PATH);
	wxBitmap b(img);
	item->AddCommand(title, group, &b);
}

void SCCMenu::BuildMenu() {
	for (unsigned int i = 0; i < children->GetCount(); i++) {
		SCCMenu* s = children->Item(i);
		if (s == NULL)
			continue;
        if (s->menu_type == DRAWS_ITEM_STUB)
			ExplodeDraws(s, i+1);
		else
			s->BuildMenu();
	}
}

void SCCMenu::RemoveSuperseded() {
	std::vector<unsigned> removed;

	for (unsigned int i = 0; i < children->GetCount(); i++) {
		SCCMenu* s = children->Item(i);
		if (s == NULL)
			continue;
		if (s->menu_type == REMOVEABLE) {
			if (hierarchy_data->superseded.find(*s->prefix) != hierarchy_data->superseded.end())
				removed.push_back(i); /*remove if SCCMenu if removeable
							and its prefix is superseded*/
			continue;
		}
		unsigned children_count = s->children->GetCount();
		if (children_count) {
			s->RemoveSuperseded();
			if (children_count != s->children->GetCount()) {
				switch (s->children->GetCount()) {
					case 0:	/*if all children of an item
						  are removed remove the item as well*/
						removed.push_back(i);
						break;
					case 1: /*if there is only one child left after
						  removal substitute the submenu
						  item with this child*/
						SCCMenu* t1 = s->children->Item(0);
						s->children->Item(0) = NULL;
						children->Item(i) = t1;
						delete s;
						break;
				}
			}
		}
	}
	/*actual removal operation*/
	for (unsigned i = 0 ; i < removed.size() ; ++i) {
		unsigned index = removed[i] - i;
		SCCMenu* menu = children->Item(index);
		delete menu;
		children->RemoveAt(index);
	}

}

wxMenu* SCCMenu::CreateWxMenu(wxArrayString unhidden_databases)
{
	wxMenu* m = new wxMenu();
	bool hidden = FALSE;
	for (unsigned int i = 0; i < children->GetCount(); i++) {
		SCCMenu* s = children->Item(i);
		if (s == NULL) {
			m->AppendSeparator();
			continue;
		}
		if (s->menu_type == DRAWS_ITEM_STUB)
			continue;
		wxMenuItem* mi;
		if(s->prefix != NULL) 
		{
		    hidden = FALSE;
        	    for(unsigned int j = 0; j < unhidden_databases.GetCount(); j++)
		    {
	                if(unhidden_databases[j] == (*(s->prefix))){
                        hidden=TRUE;
                        break;
			}
            	    }
		}

	        if(hidden == TRUE) 
		{
    		    hidden = FALSE;
	            continue;
	        }

		if (s->children->GetCount() > 0) 
		{
			mi = new wxMenuItem(m, s->id, s->name, wxEmptyString, wxITEM_NORMAL, s->CreateWxMenu(unhidden_databases));
			wxMenu *subMenu = mi->GetSubMenu();
			if(subMenu->GetMenuItemCount() == 0) continue;
		}
		else
			mi = new wxMenuItem(m, s->id, s->name);

		if (s->bmp.Ok())
			mi->SetBitmap(s->bmp);

		m->Append(mi);
	}
	return m;
}

void SCCMenu::ExplodeDraws(SCCMenu* draws_item, int pos)
{
	typedef SzarpConfigs::ConfigList CL;
	SzarpConfigs* szarp_configs = SzarpConfigs::GetInstance();

	CL* config_groups = szarp_configs->GetConfigs(draws_item->name, hierarchy_data->exclude_expr);

	if (!config_groups)
		return;

	for (CL::iterator i = config_groups->begin();i != config_groups->end(); ++i) {

		ArrayOfMenu* submenu = new ArrayOfMenu;
		ConfigGroup* group = *i;
		std::vector<wxString>& draws = hierarchy_data->draws;

		ConfigGroup::ItemsList::iterator d_iter = group->items.begin();
		for (; d_iter != group->items.end() ; ++d_iter) {
			//check if given draw is not already contained within menu hierarchy
			if (find(draws.begin(),draws.end(),d_iter->prefix) != draws.end())
				continue;
			submenu->Add(CreateDrawItem(
						d_iter->title,
						d_iter->prefix,
						hierarchy_data,
						true
						)
				);
		}
		if (submenu->Count() == 0) { //all draws are already in menu hiereachy or no configs match
			delete submenu;
			continue;
		}
		SCCMenu *item;
		if ( submenu->Count() > 1 ) {
			item = new SCCMenu(hierarchy_data,
					group->group_name);
			delete item->children;
			item->children = submenu;
			item->bmp = draws_item->bmp;
		} else { //do not create separate submenu if there is only one item to add
			item = (*submenu)[0];
			delete submenu;
		}
		children->Insert(item, pos++);
	}
	for (CL::iterator i = config_groups->begin();i != config_groups->end();++i)
		delete *i;
	delete config_groups;

}


SCCMenu* SCCMenu::ParseMenu(Tokenizer* t, wxString name,MenuHierarchyData *hierarchy_data)
{
	int token;
	wxString s, s1, s2;

#define HAVE_TOKEN(i)	\
	if (token != i) { \
		wxLogError(_T("Expected token %d, got %d ('%s')\n"), i, token, \
			s.c_str()); \
		return m; \
	}

	s = t->GetNext(&token);
	SCCMenu* m = new SCCMenu(hierarchy_data, name);
	do {
		if (token != TOK_ID) {
			wxLogError(_T("Expected indentifier, got %d (%s)"),
				token, s.c_str());
			return m;
		}
		if (s.CmpNoCase(_T("EXEC")) == 0) {
			s = t->GetNext(&token);
			HAVE_TOKEN(TOK_LEFT);
			s = t->GetNext(&token);
			HAVE_TOKEN(TOK_STRING);
			s1 = s;
			s = t->GetNext(&token);
			HAVE_TOKEN(TOK_COMMA);
			s = t->GetNext(&token);
			HAVE_TOKEN(TOK_STRING);
			s2 = s;
			s = t->GetNext(&token);
			HAVE_TOKEN(TOK_RIGHT);
			s = t->GetNext(&token);
			m->AddCommand(s1, s2);
		} else if (s.CmpNoCase(_T("DRAW")) == 0) {
			s = t->GetNext(&token);
			HAVE_TOKEN(TOK_LEFT);
			s = t->GetNext(&token);
			HAVE_TOKEN(TOK_STRING);
			s1 = s;
			s = t->GetNext(&token);
			HAVE_TOKEN(TOK_COMMA);
			s = t->GetNext(&token);
			HAVE_TOKEN(TOK_STRING);
			s2 = s;
			s = t->GetNext(&token);
			HAVE_TOKEN(TOK_RIGHT);
			s = t->GetNext(&token);
			m->AddDraw(s1,s2);
		} else if (s.CmpNoCase(_T("DRAWS")) == 0) {
			s = t->GetNext(&token);
			HAVE_TOKEN(TOK_LEFT);
			s = t->GetNext(&token);
			HAVE_TOKEN(TOK_STRING);
			s1 = s;
			s = t->GetNext(&token);
			HAVE_TOKEN(TOK_RIGHT);
			s = t->GetNext(&token);
			m->AddDraws(s1);
		} else if (s.CmpNoCase(_T("CONFIG")) == 0) {
			s = t->GetNext(&token);
			HAVE_TOKEN(TOK_LEFT);
			s = t->GetNext(&token);
			HAVE_TOKEN(TOK_STRING);
			s1 = s;
			s = t->GetNext(&token);
			HAVE_TOKEN(TOK_RIGHT);
			s = t->GetNext(&token);
			m->AddConfig(s1);
		} else if (s.CmpNoCase(_T("SEPARATOR")) == 0) {
			s = t->GetNext(&token);
			m->AddSeparator();
		} else if (s.CmpNoCase(_T("MENU")) == 0) {
			s = t->GetNext(&token);
			HAVE_TOKEN(TOK_LEFT);
			s = t->GetNext(&token);
			HAVE_TOKEN(TOK_STRING);
			s1 = s;
			s = t->GetNext(&token);
			HAVE_TOKEN(TOK_COMMA);
			m->AddMenu(ParseMenu(t, s1, hierarchy_data));
			s = t->GetNext(&token);
		} else if (s.CmpNoCase(_T("ICON")) == 0) {
			s = t->GetNext(&token);
			HAVE_TOKEN(TOK_LEFT);
			s = t->GetNext(&token);
			HAVE_TOKEN(TOK_STRING);
			s1 = s;
			s = t->GetNext(&token);
			HAVE_TOKEN(TOK_RIGHT);
			s = t->GetNext(&token);
			int count = m->children->GetCount();
			if (count) {
				SCCMenu* item = m->children->Item(count - 1);
				if (item != NULL) {
					wxImage img;
					img.LoadFile(s1);
					if (img.Ok())
						item->bmp = wxBitmap(img);
					else
						wxLogError(_T("Could not load icon %s"),
							s1.c_str());
				}
			}
		} else if (s.CmpNoCase(_T("DOC")) == 0) {
			s = t->GetNext(&token);
			wxFileName filename(wxGetApp().GetSzarpDir(), wxEmptyString);
			filename.AppendDir(_T("bin"));
			filename.SetName(_T("wxhelp"));
#ifdef __WXMSW__
			filename.SetExt(_T("exe"));
#endif

			m->AddCommand(_T("Dokumentacja SZARP"),
				filename.GetFullPath());
		} else if (s.CmpNoCase(_T("DONTGROUPBY")) == 0) {
			s = t->GetNext(&token);
			HAVE_TOKEN(TOK_LEFT);
			do {
				s = t->GetNext(&token);
				HAVE_TOKEN(TOK_STRING);
				wxRegEx* reg_ex = new wxRegEx(s, wxRE_ICASE);
				if (reg_ex->IsValid())
					hierarchy_data->exclude_expr.push_back(reg_ex);
				else {
					wxLogError(_T("Regular expression '%s' in not valid, skipping"),s.c_str());
					delete reg_ex;
				}
				s = t->GetNext(&token);
			} while ( token == TOK_COMMA );
			HAVE_TOKEN(TOK_RIGHT);
			s = t->GetNext(&token);
		} else {

			wxLogError(_T("Unexpected identifier '%s'"),
				s.c_str());
			s = t->GetNext(&token);
		}

		if (token == TOK_RIGHT)
			return m;
		if (token == TOK_COMMA)
			s = t->GetNext(&token);
		else if (token != TOK_END) {
			wxLogError(_T("Expected comma or ')' or 'TOK_END', got %d (%s)"),
				token, s.c_str());
			return m;
		}
	} while ((token != TOK_END) && (token != TOK_ERROR));
	if (token == TOK_ERROR)
		wxLogError(_T("Tokenizer error: %s"), s.c_str());

#undef HAVE_TOKEN
	return m;

}

SCCMenu* SCCMenu::ParseMenu(wxString menu)
{
	MenuHierarchyData *mhd = new MenuHierarchyData;
	int id = SCC_MENU_FIRST_ID;
	mhd->id = id;
	Tokenizer t(menu);
	SCCMenu* res =  ParseMenu(&t, _T("SZARP"), mhd);
	res->BuildMenu();
	res->RemoveSuperseded();
	return res;

}

wxString SCCMenu::GetCommand(int id)
{
	SCCMenu* item = FindItem(id);

	if (item != NULL)
		return item->exec;
	return wxEmptyString;
}

SCCMenu* SCCMenu::FindItem(int id)
{
	if (this->id == id)
		return this;
	if (children)
		for (unsigned int i = 0; i < children->GetCount(); i++) {
			SCCMenu* item = children->Item(i);
			if (item == NULL)
				continue;
			SCCMenu* f = item->FindItem(id);
			if (f != NULL)
				return f;
		}
	return NULL;
}

SCCMenu::Tokenizer::Tokenizer(wxString str) :
	s(str), pos(0), state(ST_WS), length(str.Len())
{
}

wxString SCCMenu::Tokenizer::GetNext(int* type)
{
#ifdef __UNICODE__
#define sz_isalpha(a)	iswalpha(a)
#define sz_isalnum(a)	iswalnum(a)
#else
#define sz_isalpha(a)	isalpha(a)
#define sz_isalnum(a)	isalnum(a)
#endif
	wxString ret = _T("");
	wxChar c;
	if (pos == length) {
		*type = TOK_END;
		return wxEmptyString;
	}
	while (1) {
		if (pos < length)
			c = s[pos++];
		else
			c = 0;
		switch (state) {
			case ST_WS:
				if (c == 0) {
					*type = TOK_END;
					return wxEmptyString;
				}
				if (c == '(') {
					ret += c;
					*type = TOK_LEFT;
					return ret;

				}
				if (c == ')') {
					ret += c;
					*type = TOK_RIGHT;
					return ret;
				}
				if (c == ',') {
					ret += c;
					*type = TOK_COMMA;
					return ret;
				}
				if (c == '"') {
					state = ST_STRING;
					break;
				}
				if (sz_isalpha(c)) {
					ret += c;
					state = ST_NORMAL;
				}
				break;
			case ST_NORMAL:
				if (c == 0) {
					*type = TOK_ID;
					return ret;
				}
				if (sz_isalnum(c)) {
					ret += c;
					break;
				}
				if ((c == ')') || (c == '(')
					|| (c == '"') ||
					(c == ',')) {
					pos--;
				}
				*type = TOK_ID;
				state = ST_WS;
				return ret;
			case ST_STRING:
				if (c == 0) {
					ret = \
				_T("Error: string not closed (missing \")");
					*type = TOK_ERROR;
					return ret;

				}
				if (c == '"') {
					*type = TOK_STRING;
					state = ST_WS;
					return ret;
				}
				if (c == '\\') {
					state = ST_ESCAPE;
					break;
				}
				ret += c;
				break;
			case ST_ESCAPE:
				if (c == 0) {
					ret = \
				_T("Error: string not closed (missing \")");
					*type = TOK_ERROR;
					return ret;
				}
				ret += c;
				state = ST_STRING;
				break;
			default:
				break;
		}
	}
}

SCCMenu* SCCMenu::CreateDrawItem(const wxString& title, const wxString& prefix, MenuHierarchyData *hierarchy_data, bool removable) {

	wxFileName command_fn = wxFileName(wxGetApp().GetSzarpDir(), wxEmptyString);
	command_fn.AppendDir(_T("bin"));

	command_fn.SetName(_T("draw3"));

#ifdef __WXMSW__
	command_fn.SetExt(_T("exe"));
#endif

	wxString s = command_fn.GetFullPath() +
#ifdef MINGW32
		_T(" /base:")
#else
		_T(" -Dprefix=")
#endif
		+ prefix;

	SCCMenu* m = new SCCMenu(hierarchy_data, title, s);

	wxImage img(wxGetApp().GetSzarpDir() + DRAW_ICON_PATH);
	if (img.Ok()) {
		wxBitmap bmp(img);
		m->bmp = bmp;
	}

	m->menu_type = removable ? REMOVEABLE : ORDINARY_ITEM;

	hierarchy_data->draws.push_back(prefix);

	SzarpConfigs* szarpconfigs = SzarpConfigs::GetInstance();
	std::set<wxString> supersedes = szarpconfigs->GetAggregatedPrefixes(prefix);
	hierarchy_data->superseded.insert(supersedes.begin(), supersedes.end());

	m->prefix = new wxString(prefix);

	return m;

}

void SCCMenu::Destroy(SCCMenu* m) {
	assert(m != NULL);
	delete m->hierarchy_data;
	delete m;
}
