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

#ifndef _SZARPCONFIGS_H__
#define _SZARPCONFIGS_H__

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
#include <wx/hashmap.h>
#include <map>
#include <algorithm>
#include <set>

#include "cfgnames.h"
#include "szarp_config.h"

/** Function object which compares wxStrings case insensitively */
struct NoCaseCmp : public std::binary_function<const wxString&,const wxString&,bool> {
	/** Actual comparison fucntion 
	 * @param s1 string
	 * @param s2 string */
	bool operator() (const wxString& s1, const wxString& s2) const {
#ifdef __WXGTK__
		const wchar_t* ss1 = s1.c_str();
		const wchar_t* ss2 = s2.c_str();
		return wcscoll(ss1, ss2) < 0;
#else
		return s1.CmpNoCase(s2) < 0;
#endif
	}
};

/** Function object which preforms regular expression matching */
class RegExMatch {
	public:
	/** Object's constructor 
	 * @param string to be matched */
	RegExMatch(const wxString& _str);
	/** Matching operator 
	 * @param expr regular expression to be matched against the string*/
	bool operator() (const wxRegEx* expr) const;
	private:
	const wxString& str;	/**< holds reference to string on which match if performed */
};


	/** holds data on groups of configs returned by @see GetConfigs*/
struct ConfigGroup 
{
	/** Single item of a group list*/
	struct GroupItem 
	{
		wxString prefix ; 	/**< config's prefix */
		wxString title; 	/**< title of config */

		/** Comapre @see GroupItems alphabetically by item's title */
		struct ItemComparer : public std::binary_function<const GroupItem&,const GroupItem&,bool> {
		/** Actual comparison fucntion 
		 * @param i1 GroupItem
		 * @param i2 GroupItem */
			bool operator() (const GroupItem& i1, const GroupItem& i2) const {
				return NoCaseCmp()(i1.title, i2.title);
			}
		};
		
	}; 

	typedef	std::multiset<GroupItem,GroupItem::ItemComparer> ItemsList ; /**< list of sorted items */

	ItemsList items; 		/**< list of items */
	wxString group_name; 		/**< name of a group */
	/** convenience function: adds GroupItem to group items list
	 * @param prefix - configuration prefix 
	 * @param title - configuration title */
	void AppendConfig(const wxString& prefix,const wxString& title);

	/** Comapre @see ConfigGroup alphabetically by group's name */
	struct ConfigGroupComparer : public std::binary_function<const ConfigGroup*,const ConfigGroup*,bool> {
	/** Actual comparison function 
	 * @param c1 ConfigGroup 
	 * @param c2 ConfigGroup */
		bool operator() (const ConfigGroup* c1, const ConfigGroup* c2) const {
			return NoCaseCmp()(c1->group_name, c2->group_name);
		}
	};
}; 

/** SzarpConfigs class provides access to SZARP's configurations */
class SzarpConfigs
{
public:
	//typedef std::multiset<ConfigGroup*, ConfigGroup::ConfigGroupComparer> ConfigList;
	typedef std::vector<ConfigGroup*> ConfigList;
private:
	typedef std::map<std::set<wxString>, std::set<wxString, NoCaseCmp> > MPW;

	/** object's constructor - only one instance of an object exists */
	SzarpConfigs();
	/**  class instance */
	static SzarpConfigs* instance;
	/** tokenizes szarp configs titles skipping words which match given reulars expressions
	 * @param titles assoc table in form key->prefix,value->title 
	 * @param ignored list of regular expressions - a word matching any of these expressions is ommitted 
	 * @param titles_words method outputs tokenized titles into this containter, key is a config's prefix, 
	 * value a list of words */
	void PrepareTitles(ConfigNameHash& titles,
			const std::vector<wxRegEx*>& ignored,
			std::map<wxString, std::set<wxString, NoCaseCmp> >& titles_words 
			);

	WX_DECLARE_STRING_HASH_MAP(bool, StringHash);

	/**Maps prefixes and titles together
	 * @param titles_word - array holding titles' words for a configuration prefixes
	 * @return generated mapping */
	MPW AssignWordsToPrefixes(std::map<wxString, std::set<wxString, NoCaseCmp> > titles_words);


	/**Creates configuration groups. Only groups having more than one configuration
	 * are created. Prefixes contained in generated groups are removed from parameter 'titles'
	 * @param result - arrys of configuration's groups - result of the operaion
	 * @param prefix_map - mapping of titles words and configurations
	 * @param titles - assoc array mapping prefixes to titles*/
	void CreateGroups(ConfigList* result, MPW& prefix_map, ConfigNameHash &titles);

	/**Creates one element groups.
	 * @param config_groups - created groups are stored in this conatiner
	 * @param titles - assoc array mapping prefixes to titles*/
	void CreateOneElementGroups(ConfigList* config_groups, ConfigNameHash& titles);
	/**Flag denoting if aggrated configurations should be recognized*/
	bool handle_aggregated;

	/**Configurations located in this hash will not be treated as aggregating configurations*/
	StringHash dont_aggregate;

	/**Represents aggregated configuration*/
	class AggregatedConfig {

		xmlDocPtr doc;	/**<Pointer to xml document descirbing configuration*/

		public:
		AggregatedConfig() : doc(NULL) {};
		/**Parses xml document with aggregated configuration describtion
		 * @param file_name name of file with configuration
		 * @return bool if document was parsed successfully*/
		bool ParseXml(const char* file_name);

		/*@return set of configuration prefixes aggregated by 
		 * the configuration*/
		std::set<wxString> GetAggregatedPrefixes();

		~AggregatedConfig();

	};

	void SortConfigList(ConfigList *config_list, const std::vector<wxRegEx*> &ignored);

public:
	/** gets config for a prefix */
	TSzarpConfig* GetConfig(const wxString& prefix);

	/** retrieves SzarpConfigs instance */
	static SzarpConfigs* GetInstance();
	/* List of pointers to @see ConfigGroups, ConfigGroups are sorted alpahabetically
	 * by group name */
	/** Method returs a list of config groups which match given regular expression.
	* Configs are regarded to belong to the same group if their titles share a word which
	* does not match any regular expression listed in @see ignored parameter.
	* A name of a group is a common word in the group configs titles.
	* If a ConfigGroup contains only one item @see group_name is set to the item's title.
	* @param expr regular expression against which config prefixes should be matched 
	* @param ignored list of regular expressions 
	* @return list of @see ConfigGroup */
	ConfigList* GetConfigs(const wxString& expr, 
				const std::vector<wxRegEx*>& ignored
				);
									  

	/* Returns set of configuration prefixes which are aggregated
	 * by configuration pointed by 
	 * @param prefix*/
	std::set<wxString> GetAggregatedPrefixes(const wxString& prefix);

};

#endif
