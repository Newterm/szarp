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
/* * draw3
 * SZARP
 
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 *
 * $Id$
 * Configuration manager.
 */

#ifndef __CFGMGR_H__
#define __CFGMGR_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <vector>
#include <limits>

#include <wx/hashmap.h>
#include "szarp_config.h"
#include "cfgnames.h"


class DrawParam {
protected:
	TParam *m_param;	
public:
	DrawParam() : m_param(NULL) {}

	DrawParam(TParam *p) : m_param(p) {}

	virtual wxString GetBasePrefix();
	
	virtual wxString GetParamName();

	TParam* GetIPKParam();
	
	virtual wxString GetUnit();

	virtual wxString GetDrawName();

	virtual wxString GetShortName();

	virtual wxString GetSumUnit();

	const double& GetSumDivisor();

	virtual int GetPrec();

	virtual ~DrawParam() {};
};

/** Single draw info */
class DrawInfo
{
    public:
	DrawInfo();
	
	DrawInfo(TDraw *d, DrawParam *p);
	
	/** @return name of draw */
	virtual wxString GetName();
	
	/** @return name of parameter */
	virtual wxString GetParamName();

	virtual wxString GetShortName();
	
	/** @return pointer to parameter object */
	DrawParam *GetParam();
	
	/** @return pointer to draw object */
	TDraw *GetDraw();

	/** @return declared minimal value for draw */
	virtual double GetMin();
	
	/** @return declared maximum value for draw */
	virtual double GetMax();

	virtual int GetScale();

	virtual int GetPrec();

	virtual double GetScaleMin();

	virtual wxString GetUnit();

	virtual wxString GetSumUnit();

	const double& GetSumDivisor();

	virtual double GetScaleMax();

	virtual TDraw::SPECIAL_TYPES GetSpecial();
	
	/** @return color of draw */
	virtual wxColour GetDrawColor();
	
	/** set color of draw
	 * @param color new color, no checking */
	virtual void SetDrawColor(wxColour color);
	
	/** comparison function used to sort arrays */
	static bool CompareDraws(DrawInfo *first, DrawInfo *second);
	
	/** @return priority of draws set (window) */
	virtual double GetPrior();

	/** @return draw number (position in params.xml) */
	int GetNumber();
	
	/** @return prefix of base configuration */
	virtual wxString GetBasePrefix();

	/** @return name of the window this DrawInfo is part of*/
	virtual wxString GetSetName();

	virtual DrawsSets* GetDrawsSets() = 0;

	virtual wxString GetValueStr(const double &val, const wxString &no_data_str = L"-");

	virtual bool IsValid() const;

	virtual ~DrawInfo() { }
    protected:
	TDraw *d;	/**< Pointer to draw. */
	
	DrawParam *p;	/**< Pointer to parameter. */
	
	wxColour c;	/**< Color of draw. */

};

class DrawInfoList : public std::vector<DrawInfo*> {
public:
	void SetSelectedDraw( DrawInfo* di )
	{	selected = di; }

	DrawInfo* GetSelectedDraw()
	{	return selected; }
private:
	DrawInfo* selected;
};

class IPKDrawInfo : public DrawInfo {
protected:
	DrawsSets *ds;
public:
	IPKDrawInfo(TDraw *d, TParam *p, DrawsSets *ds);

	virtual DrawsSets *GetDrawsSets();

	virtual ~IPKDrawInfo() { delete p; }
};


/** Array of draws from one set. */
typedef std::vector<DrawInfo*> DrawInfoArray;

/** Draw Set (window in draw2) */
class DrawSet
{
    public:
	/** Default constructor */
	DrawSet(DrawsSets *cfg) : m_name(wxString()), m_prior(-1.0),  m_number(-1), m_cfg(cfg)
	    { m_draws = new DrawInfoArray(); };
    
	/**
	 * Constructor setting name of DrawSet
	 * @param DrawSet name
	 */
	DrawSet(wxString & name, DrawsSets *cfg) : m_name(name), m_prior(-1.0), m_number(-1), m_cfg(cfg)
	    { m_draws = new DrawInfoArray(); };

	/**
	 * Constructor setting name and number of DrawSet in params.xml
	 * @param name
	 * @param number
	 * @param parent ConfigManager
	 */
	DrawSet(wxString & name, int number, DrawsSets * cfg)
	    : m_name(name), m_prior(-1.0), m_number(number), m_cfg(cfg) 
	    { m_draws = new DrawInfoArray(); };

	/** Copy constructor */
	DrawSet(const DrawSet& drawSet);

	virtual ~DrawSet();

	/** @return name of DrawSet */
	wxString& GetName();

	/** @param name of DrawSet to set */
	virtual void SetName(const wxString&);

	/** @return number of DrawSet in params.xml */
	int GetNumber() { return m_number; };

	/** @param number of DrawSet in params.xml (for sorting) */
	void SetNumber(int number) { m_number = number; };

	/** @return prior from param.xml (lowest prior from draws in DrawSet) */
	virtual double GetPrior();

	/** Compares DrawSets - for sorting (based on prior and number) */
	static int CompareSets(DrawSet * d1, DrawSet * d2);

	/** Add draw to DrawSet */
	void Add(DrawInfo* drawInfo);

	/** Sort Draws */
	void SortDraws();

	/** @return Draws in DrawSet */
	DrawInfoArray* GetDraws() { return m_draws; }

	/** @return Draw with given index */
	DrawInfo* GetDraw(int index);
	
	/** @return Name of draw with given index */
	wxString GetDrawName(int index);
	
	/** @return ParamName of draw with given index */
	wxString GetParamName(int index);

	/** @return sets this set is part of*/
	DrawsSets* GetDrawsSets();

	/**
	 * @param index Index of draw
	 * @return Color of draw with given index
	 */
	wxColour GetDrawColor(int index);

	/** initializes all NULL colors in set */
	void InitNullColors();

	static const double unassigned_prior_value;

	static const double defined_draws_prior_start;

    protected:
	wxString m_name;    /**< name of DrawSet */
	double m_prior;	    /**< draw set prior (minimum of priors of draws */
	int m_number;	    /**< number of draw set (position in params.xml */

	DrawsSets* m_cfg;	/**< parent config */

	DrawInfoArray* m_draws; /**< Array of DrawInfos */
};
    

/** Hash map, sets names are keys, draw array are values. */
WX_DECLARE_STRING_HASH_MAP(DrawSet *, DrawSetsHash);

WX_DEFINE_SORTED_ARRAY(DrawSet *, SortedSetsArray);

class DrawTreeRoot;

class DrawTreeNode {
public:
	enum ELEMENT_TYPE { LEAF, NODE };
protected:
	ELEMENT_TYPE m_elemet_type;
	DrawSet* m_child_set;	
	std::vector<DrawTreeNode*> m_child_nodev;
	double m_prior;
	wxString m_name;
public:	
	DrawTreeNode();
	void Sort();
	wxString GetName() const;
	double GetPrior() const;
	const std::vector<DrawTreeNode*>& GetChildren() const;
	DrawSet* GetDrawSet() const;
	ELEMENT_TYPE GetElementType() const;
	virtual ~DrawTreeNode();

	friend class DrawTreeRoot;
};

class DrawTreeRoot : public DrawTreeNode {
	DrawTreeNode* m_user_subtree;
	DrawTreeNode* AddTNode(TTreeNode *node);
public:
	DrawTreeRoot();
	void AddSet(TDraw* d, DrawSet *node);
	void AddUserSet(DrawSet* darwset);
	void RemoveUserSet(wxString name, DrawSet *set);
	void RenameUserSet(wxString oname, DrawSet *set);
	void SubstituteUserSet(wxString name, DrawSet *set);
};

class DrawsSets
{
    public:
	DrawsSets(ConfigManager *cfg);
	/** Destructor */
	virtual ~DrawsSets();
	
	/** Return configuration ID (title) */
	virtual wxString GetID() = 0;

	/** Return configuration prefix*/
	virtual wxString GetPrefix() = 0;
	
	/** Set of draws. This is HashMap with set name keys. Contains sets present in this config as well as sets
	 * that relate to these params from this config and are in 'User defined sets'*/
	virtual DrawSetsHash& GetDrawsSets(bool all = true) = 0;

	/** @return sorted drawsets names */
	virtual SortedSetsArray GetSortedDrawSetsNames(bool all = true);

	/** @return @see ConfigManager*/
	ConfigManager* GetParentManager();

	virtual DrawSetsHash& GetRawDrawsSets() = 0;

	virtual void AttachDefined() = 0;
	
	void AddUserSet(DefinedDrawSet *s);

	void RemoveUserSet(wxString name);

	void RenameUserSet(wxString oname, DefinedDrawSet *set);

	void SubstituteUserSet(wxString name, DefinedDrawSet *set);

	DrawTreeNode& GetDrawTree();

    protected:
	ConfigManager*  m_cfgmgr;    /**< parent ConfigManager */

	DrawTreeRoot m_tree_root;		
};

class IPKConfig : public DrawsSets 
{
    public:
	/** 
	 * Constructor, initializes 'c' attribute.
	 * @param c IPK configuration, not NULL, freeing by destructor
	 * @param basemgr database access manager, freeing by destructor
	 */
	IPKConfig(TSzarpConfig *c, ConfigManager *mgr);
	
	/** Destructor */
	virtual ~IPKConfig();
	
	/** Return configuration ID (title) */
	virtual wxString GetID();

	/** Return configuration prefix*/
	virtual wxString GetPrefix();
	
	/** @return @see TSzarpConfig associated with this draw*/
	TSzarpConfig* GetTSzarpConfig() { return m_sc; }

	void AttachDefinedSet(DefinedDrawSet *set);

	virtual void AttachDefined();

	virtual DrawSetsHash& GetDrawsSets(bool all = true);

	virtual DrawSetsHash& GetRawDrawsSets();
protected:
	wxString m_id;	/**< Configuration ID (title) */

	TSzarpConfig* m_sc;    /**< Configuration data. */

	/** Set of draws. This is HashMap with set name keys. Contains sets present in this config as well as sets
	 * that relate to these params from this config and are in 'User defined sets'*/
	DrawSetsHash drawSets;

	/** Set of draws. This is HashMap with set name keys, these are sets only present in this config.*/
	DrawSetsHash baseDrawSets;
	
	bool defined_attached;

};

WX_DECLARE_STRING_HASH_MAP(DrawsSets*, DrawsSetsHash);

class ConfigManager
{
    public:
	/** Default constructor, does nothing. */
	ConfigManager(DrawApp* app, IPKContainer *ipk_container);
	
	/** Loads configuration for given base name 
	 * @param prefix prefix of configration to load
	 * @param config_path optional path to a file with a configuration to load
	 * @return loaded configuration*/
	DrawsSets *LoadConfig(const wxString& prefix, const wxString& config_path = wxEmptyString, bool logparams = false );
	
	/** @return configuration object for given prefix, NULL if configuration cannot be laoded*/
	DrawsSets *GetConfigByPrefix(const wxString& prefix, bool load = true);

	/** @return configuration object for given id(configuration name), NULL if configuration cannot be laoded*/
	DrawsSets *GetConfig(const wxString& id);

	/** @return sets containing defined sets*/
	DefinedDrawsSets* GetDefinedDrawsSets();

	/** Sets the splashscreen for displaying the progress of loading configuration files.
	 * @param statusBar the statusbar */
	void SetSplashScreen(SplashScreen *splash) { this->splashscreen = splash; };

	/** Resets the statusbar. */
	void ResetSplashScreen() { this->splashscreen = NULL; };

	/**
	 * @param id configuration identifier
	 * @param set name of draws set
	 * @param index index of draw in set
	 * @return pointer to draw info */
	DrawInfo *GetDraw(const wxString id, const wxString set, int index);
	
	bool RemoveDefinedParam(DefinedParam *p);

	bool RemoveDefinedParam(wxString prefix, wxString name);

	/**@return all available onfigurations*/
	DrawsSetsHash& GetConfigurations();

	/** Appends new configuration to list.
	 * @param ipk initialized IPK configuration object
	 * @return configuration object
	 */
	DrawsSets* AddConfig(TSzarpConfig *sc);

	/** @return prefixes of names of all available configurations*/
	ConfigNameHash& GetConfigTitles();

	/** @retrun path to main SZARP directory*/
	wxString GetSzarpDir();

	/** Add object to a list of configrations' observers*/
	void RegisterConfigObserver(ConfigObserver *obsrver);

	/** Removes object from a list of configrations' observers*/
	void DeregisterConfigObserver(ConfigObserver *observer);

	/** Notifies observers that a set has been removed from the configuration
	 * @param prefix - prefix of the configuration
	 * @param name - name of the set*/
	void NotifySetRemoved(wxString prefix, wxString name);

	void NotifyStartConfigurationReload(wxString prefix);

	void NotifyEndConfigurationReload(wxString prefix);

	/** Notifies observers that a name of the set has changed from the configuration
	 * @param prefix - prefix of the configuration
	 * @param from - previous name of the set
	 * @param name - new name of the set*/
	void NotifySetRenamed(wxString prefix, wxString from, wxString to, DrawSet *set);

	/** Notifies observers that a set has been modified 
	 * @param prefix - prefix of the configuration
	 * @param from - name of the set
	 * @param set - pointer to actual set*/
	void NotifySetModified(wxString prefix, wxString name, DrawSet *set);

	/** Notifies observers that a new set has been added 
	 * @param prefix - prefix of the configuration
	 * @param from - name of the set
	 * @param set - pointer to new set*/
	void NotifySetAdded(wxString prefix, wxString name,DrawSet *set);

	void NotifyParamSubsitute(DefinedParam *d, DefinedParam *n);

	void NotifyParamDestroy(DefinedParam *d);

	/**Method called when @see DatabaseManager is ready for configuration reload*/
	void ConfigurationReadyForLoad(wxString prefix);

	/**sets @see DatabaseManager object*/
	void SetDatabaseManager(DatabaseManager *mgr);

	/**Reloads configuration with given prefix*/
	bool ReloadConfiguration(wxString prefix);

	/**Loads defined draws set from file*/
	void LoadDefinedDrawsSets();

	/**Loads defined draws set from file*/
	bool SaveDefinedDrawsSets();

	bool IsPSC(wxString prefix);

	void EditPSC(wxString prefix, wxString param = wxString());

	void ImportSet();

	void ExportSet(DefinedDrawSet *set, wxString our_name);

	void RemoveDefinedParams(std::vector<DefinedParam*>& dp);

	bool SubstituteOrAddDefinedParams(const std::vector<DefinedParam*>& dp);

	void SubstiuteDefinedParams(const std::vector<DefinedParam*>& to_rem, const std::vector<DefinedParam*>& to_add);
	
	~ConfigManager();
protected:
	void FinishConfigurationLoad(wxString prefix);
	/**Vector of observers*/
	std::vector<ConfigObserver*> m_observers;

	/** Hash of configurations. */
	DrawsSetsHash config_hash;

	/**@return all configus prefixes*/
	wxArrayString GetPrefixes();

	/** Pointer to szApp which use this manager */
	DrawApp* m_app;
	
	/** Container for a IPKs */
	IPKContainer *m_ipks;

	/** Containers mapping configuration prefixes to their names*/
	ConfigNameHash m_config_names;

	/**@see DatabaseManager*/
	DatabaseManager *m_db_mgr;
	
	/** @see SplashScreen*/
	SplashScreen *splashscreen;

	DefinedDrawsSets* m_defined_sets;

	wxCriticalSection m_reload_config_CS;

	DrawPsc *psc;

	bool m_logparams;
};

#endif

