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

 * Darek Marcinkiewicz reksio@parterm.com.pl
 *
 * $Id$
 */

#include <wx/tokenzr.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/file.h>

#include "cconv.h"
#include "szarpconfigs.h"
#include "libpar.h"
#include "sccapp.h"


using std::vector;
using std::map;
using std::find_if;
using std::set;

SzarpConfigs* SzarpConfigs::instance = NULL;

SzarpConfigs::SzarpConfigs()
{
	xmlInitParser();
	char* aggregated = libpar_getpar("scc", "hide_aggregated", 0);
	if (aggregated) {
		handle_aggregated = !(strcmp(aggregated, "yes"));
		free(aggregated);
	}
	else 
		handle_aggregated = true;

	if (handle_aggregated) {
		char* da = libpar_getpar("scc", "dont_aggregate", 0);

		if (da != NULL) {
			wxStringTokenizer tknz(wxString(SC::A2S(da)), 
					_T(","), 
					wxTOKEN_STRTOK);
			while (tknz.HasMoreTokens()) 
				dont_aggregate[tknz.GetNextToken()] = true;

			free(da);
		}

	}


};

TSzarpConfig* SzarpConfigs::GetConfig(const wxString& prefix)
{
	int i = 1;
	wxString path = 
		wxGetApp().GetSzarpDataDir() + prefix + _T("/config/params.xml");
	TSzarpConfig *sc = new TSzarpConfig();
	if (wxFile::Exists(path)) {
		i = sc->loadXML(std::wstring(path.wc_str()));
	}
	if (i != 0) {
		delete sc;
		return NULL;
	}
	return sc;
}
	
SzarpConfigs* SzarpConfigs::GetInstance()
{
	if (instance == NULL)
		instance = new SzarpConfigs();
	return instance;
}

void SzarpConfigs::PrepareTitles(ConfigNameHash& titles, 
		const std::vector<wxRegEx*>& ignored_expr, 
		map<wxString,set<wxString, NoCaseCmp> >& titles_words)
{
	for (ConfigNameHash::iterator i = titles.begin(); i != titles.end(); ++i) {
		set<wxString, NoCaseCmp> words;
		wxStringTokenizer tknz(i->second, _T(" \t\r\n"), wxTOKEN_STRTOK);
		while (tknz.HasMoreTokens()) {
			wxString token = tknz.GetNextToken();
			if (token.Length() < 2)
				continue; //we are not interested in one letter words
			if (find_if(ignored_expr.begin(), ignored_expr.end(), RegExMatch(token)) != ignored_expr.end() )
				continue; //this word we should ignore
			words.insert(token);
		}
		titles_words[i->first] = words;
	}
}

SzarpConfigs::ConfigList* SzarpConfigs::GetConfigs(const wxString& expr, const vector<wxRegEx*>& ignored_words)
{
	ConfigNameHash _titles = GetConfigTitles(wxGetApp().GetSzarpDataDir());

	ConfigNameHash titles;

	wxRegEx prex(expr);

	if (prex.IsValid() == false) {
		wxLogError(_T("Invalid regular expression %s"), expr.c_str());
		return NULL;
	}

	for (ConfigNameHash::iterator i = _titles.begin(); i != _titles.end() ; ++i) {
		if (!prex.Matches(i->first))
			continue;

		titles[i->first] = i->second;
	}
	

	if (titles.size() == 0 )
		return NULL;

	map<wxString, set<wxString, NoCaseCmp> > titles_words;

	PrepareTitles(titles, ignored_words, titles_words);

	MPW prefixes_map = AssignWordsToPrefixes(titles_words);

	ConfigList* result = new ConfigList;
	CreateGroups(result, prefixes_map, titles);
	/* all those titles which left were not assigned to any group.
	 * add each to separate group */
	CreateOneElementGroups(result, titles);

	SortConfigList(result, ignored_words);

	return result;

}

void SzarpConfigs::CreateGroups(ConfigList* result, MPW& prefix_map, ConfigNameHash &titles) {
	for (MPW::iterator i = prefix_map.begin(); i != prefix_map.end(); ++i) {
		const set<wxString>& prefixes = i->first;
		set<wxString, NoCaseCmp>& words = i->second;
		if (prefixes.size() <= 1) 
			continue;

		set<wxString>::iterator j;
		wxString title;
		for (j = prefixes.begin(); j != prefixes.end(); j++)  {
			ConfigNameHash::iterator k  = titles.find(*j);
			if (k != titles.end()) {
				title = k->second;
				break;
			}
		}

		if (j == prefixes.end())
			continue;

		ConfigGroup* confgroup = new ConfigGroup;
		wxString& group_name = confgroup->group_name;
			
		wxStringTokenizer tknz(title, 
					_T(" \t\r\n"), 
					wxTOKEN_STRTOK);
		while (tknz.HasMoreTokens()) {
			wxString token = tknz.GetNextToken();
			if (words.find(token) != words.end()) 
				group_name += token + _T(" ");
		}

		int prefix_count = 0;
		for (set<wxString>::iterator k = prefixes.begin(); k != prefixes.end(); k++) {
			ConfigNameHash::iterator l = titles.find(*k);
			if (l == titles.end())
				continue;
			prefix_count++;
			confgroup->AppendConfig(*k, l->second);
		}

		if (prefix_count <= 1) {
			delete confgroup;
			continue;
		}

		result->push_back(confgroup);
		for (set<wxString>::iterator k = prefixes.begin(); k != prefixes.end(); k++)  {
			titles.erase(*k);
		}
	}
}


SzarpConfigs::MPW SzarpConfigs::AssignWordsToPrefixes(map<wxString, set<wxString, NoCaseCmp> > titles_words) {
	typedef map<wxString, set<wxString> > MWP;
	MWP words_map;
	for (map<wxString, set<wxString, NoCaseCmp> >::iterator i = titles_words.begin(); i != titles_words.end(); ++i) {
		const wxString& prefix = i->first;
		set<wxString,NoCaseCmp>& words = i->second;
		for (set<wxString, NoCaseCmp>::iterator j = words.begin(); j != words.end(); ++j) {
			set<wxString>& prefixes = words_map[*j];
			prefixes.insert(prefix);
		}
	}

	MPW prefix_map;

	for (MWP::iterator i = words_map.begin(); i != words_map.end(); ++i) {
		set<wxString, NoCaseCmp>& words = prefix_map[i->second];
		words.insert(i->first);
	}

	return prefix_map;

}

void SzarpConfigs::CreateOneElementGroups(ConfigList* config_groups, ConfigNameHash& titles) {
	for (ConfigNameHash::iterator i = titles.begin(); i != titles.end() ; ++i) {
		const wxString& prefix = i->first;
		ConfigGroup* confgroup = new ConfigGroup;
		confgroup->AppendConfig(prefix, titles[prefix]);
		confgroup->group_name = titles[prefix];
		config_groups->push_back(confgroup);
	}
}


void ConfigGroup::AppendConfig(const wxString& prefix,const wxString& title)
{
	GroupItem item;
	item.prefix = prefix;
	item.title = title;
	items.insert(item);
}

namespace {
	struct GroupSortEntry {
		GroupSortEntry(ConfigGroup *group, const std::vector<wxRegEx*>& ignored) {
			this->group = group;
			wxStringTokenizer tknz(group->group_name, _T(" \t\r\n"), wxTOKEN_STRTOK);
			while (tknz.HasMoreTokens()) {
				wxString token = tknz.GetNextToken();
				if (token.Length() <= 3)
					continue; //we are not interested in 3(or less) letter words
				if (find_if(ignored.begin(), ignored.end(), RegExMatch(token)) != ignored.end() )
					continue; //this word we should ignore
				unique += token + _T(" ");
			}
		}
		wxString unique;
		ConfigGroup *group;
		bool operator<(const GroupSortEntry &entry) const {
			return NoCaseCmp()(unique, entry.unique);
		}
	};
}

void SzarpConfigs::SortConfigList(ConfigList *config_list, const std::vector<wxRegEx*> &ignored) {
		std::multiset<GroupSortEntry> entries;

		for (ConfigList::iterator i = config_list->begin(); i != config_list->end(); ++i) 
			entries.insert(GroupSortEntry(*i, ignored));

		config_list->clear();
		for (set<GroupSortEntry>::iterator i = entries.begin(); i != entries.end(); ++i) 
			config_list->push_back(i->group);
}

set<wxString> SzarpConfigs::GetAggregatedPrefixes(const wxString& prefix) {

	if (!handle_aggregated || dont_aggregate.find(prefix) != dont_aggregate.end()) 
		return set<wxString>();	

	wxString path = wxGetApp().GetSzarpDataDir() + prefix + _T("/config/aggr.xml");

	if (!wxFile::Exists(path)) 
		return set<wxString>();	

	AggregatedConfig aggr_config;

	if (!aggr_config.ParseXml(SC::S2A(path).c_str())) 
		return set<wxString>();	
		
	return aggr_config.GetAggregatedPrefixes();

}
	
RegExMatch::RegExMatch(const wxString& _str) : str(_str)
{ };

bool RegExMatch::operator() (const wxRegEx* expr) const
{
	return expr->Matches(str);
}

bool SzarpConfigs::AggregatedConfig::ParseXml(const char* file_name) 
{
	if (doc)
		xmlFree(doc);

	doc = xmlParseFile(file_name);

	return doc != NULL;
}

set<wxString> SzarpConfigs::AggregatedConfig::GetAggregatedPrefixes() 
{
	set<wxString> result;

	if (!doc)
		return result;

	xmlNodePtr node = doc->children;

	if (!node || strcmp((const char*) node->name, "aggregate"))  
		return result;

	for (node = node->children ; node; node = node->next) {
		if (strcmp((const char*) node->name, "config"))
			continue;

		xmlChar* config_prefix = xmlGetProp(node, (const xmlChar*) "prefix");

		if (config_prefix) {
			result.insert(wxString(SC::U2S(config_prefix)));
			xmlFree(config_prefix);
		}
	}

	return result;
}

SzarpConfigs::AggregatedConfig::~AggregatedConfig() {
	if (doc)
		xmlFreeDoc(doc);
}

