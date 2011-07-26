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
 * draw3
 * SZARP

 *
 * $Id$
 *
 * Contains set of draws
 */

#ifndef _DEFWIN_H
#define _DEFWIN_H

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/dynarray.h>

#include <vector>
#include <set>

class DefinedParam : public DrawParam {
	wxString m_formula;

	wxString m_param_name;

	wxString m_base_prefix;

	wxString m_unit;

	int m_prec;

	time_t m_start_time;

	TParam::FormulaType m_type;

public:
	DefinedParam() : m_start_time(-1) {
	}
	;

	DefinedParam(wxString base_prefix, wxString name, wxString unit, wxString formula, int prec,
			TParam::FormulaType type, time_t start_time);

	void SetParamName(wxString pn);

	void ParseXML(xmlNodePtr node);

	void GenerateXML(xmlNodePtr parent);

	void SetPrec(int prec);

	void SetFormula(wxString formula);

	void SetFormulaType(TParam::FormulaType type);

	virtual int GetPrec();

	virtual wxString GetUnit();

	void SetUnit(wxString unit);

	wxString GetFormula();

	virtual wxString GetParamName();

	virtual wxString GetBasePrefix();

	TParam::FormulaType GetFormulaType();

	void SetStartTime(time_t start_time);

	time_t GetStartTime();

	void CreateParam();

};

class DefinedDrawsSets : public DrawsSets, public ConfigObserver {
public:
	/** 
	 * Constructor, initializes 'c' attribute.
	 * @param c IPK configuration, not NULL, freeing by destructor
	 * @param basemgr database access manager, freeing by destructor
	 */
	DefinedDrawsSets(ConfigManager *mgr);

	/** Destructor */
	virtual ~DefinedDrawsSets();

	/** Return configuration ID (title) */
	virtual wxString GetID();

	/** Return configuration prefix*/
	virtual wxString GetPrefix();

	bool LoadSets(wxString path, std::vector<DefinedDrawSet*>& draw_sets, std::vector<DefinedParam*>& defined_params);

	void LoadSets(wxString path);

	bool IsModified();

	void SetModified(bool modfied = true);

	bool SaveSets(wxString path, const std::vector<DefinedDrawSet*>& ds, const std::vector<DefinedParam*>& dp);

	void SaveSets(wxString path);

	void AddSet(DefinedDrawSet *set);

	void SubstituteSet(wxString name, DefinedDrawSet *set);

	static const wxChar* const DEF_PREFIX;

	static const wxChar* const DEF_NAME;

	void RemoveSet(wxString name);

	void LoadDraw2File();

	void ConfigurationWasLoaded(wxString prefix, std::set<wxString>* configs_to_load = NULL);

	virtual DrawSetsHash& GetDrawsSets(bool all = true);

	virtual DrawSetsHash& GetRawDrawsSets();

	void RemoveAllRelatedToPrefix(wxString prefix);

	virtual void AttachDefined();

	wxString GetNameForParam(const wxString& prefix, const wxString& proposed_name);

	std::vector<DefinedParam*>& GetDefinedParams();

	void AddDefinedParam(DefinedParam *dp);

	void RemoveParam(DefinedParam *dp);

	void RemoveDrawFromSet(DefinedDrawInfo *di);
protected:
	void AddDrawsToConfig(wxString prefix, std::set<wxString> *configs_to_load);

	/**Priority of next draw added to defined daws sets*/
	double m_prior;

	/**Flags idicating is set has been modified*/
	bool m_modified;

	/** Set of draws. This is HashMap with set name keys. Contains sets present in this config as well as sets
	 * that relate to these params from this config and are in 'User defined sets'*/
	DrawSetsHash drawsSets;

	std::vector<DefinedParam*> definedParams;

	/** Defined draws info*/
	bool m_params_attached;

};

/**
 * DrawInfo with flags marking changes
 */
class DefinedDrawInfo : public DrawInfo {
private:
	double m_min;

	double m_max;
	
	time_t m_start_date;

	TDraw::SPECIAL_TYPES m_sp;

	wxString m_set_name;

	wxString m_name;

	wxString m_short_name;

	wxString m_unit;

	DefinedDrawsSets* m_ds;

	/** User changed long name (title in draw) */
	bool m_long_changed;

	/** User changed shrot name */
	bool m_short_changed;

	/** User changed min value */
	bool m_min_changed;
	
	/** User changed max value */
	bool m_max_changed;

	/** User changed color*/
	bool m_col_changed;

	/** User changed special*/
	bool m_special_changed;

	/** User chagned draw unit*/
	bool m_unit_changed;

	bool m_scale_changed;

	double m_scale;	

	bool m_min_scale_changed;

	double m_min_scale;	

	bool m_max_scale_changed;

	double m_max_scale;

	wxString m_param_name;

	wxString m_base_prefix;

	wxString m_base_draw;

	wxString m_title;

	void ParseDrawElement(xmlNodePtr d);

	void ParseDefinedElement(xmlNodePtr d);

public:
	DefinedDrawInfo(DrawInfo *di, DefinedDrawsSets* ds);

	DefinedDrawInfo(wxString draw_name, wxString short_name, wxColour color, double min, double max,
			TDraw::SPECIAL_TYPES special, wxString unit, DefinedParam *dp, DefinedDrawsSets *cfg);

	void SetBasePrefix(wxString prefix);

	void SetParamName(wxString param_name);

	virtual wxString GetParamName();

	virtual wxString GetBasePrefix();

	virtual wxString GetBaseDraw();

	virtual wxString GetBaseParam();

	virtual wxString GetUnit();

	struct EkrnDefDraw {
		DrawInfo *di;
		wxColour color;
	};

	DefinedDrawInfo(EkrnDefDraw& edd, DefinedDrawsSets* ds);

	DefinedDrawInfo(DefinedDrawsSets* ds);

	virtual DefinedDrawsSets* GetDrawsSets();

	/** @return name of draw */
	virtual wxString GetName();

	virtual wxString GetShortName();

	virtual TDraw::SPECIAL_TYPES GetSpecial();

	virtual void SetDrawColor(wxColour color);

	void SetDraw(TDraw *d);

	void SetColorFromBaseDrawInfo(const wxColour &color);

	void SetParam(DrawParam *d);

	/** @return declared minimal value for draw */
	virtual double GetMin();

	/** @return declared maximum value for draw */
	virtual double GetMax();

	/** @return name of the window this DrawInfo is part of*/
	virtual wxString GetSetName();

	void SetSpecial(TDraw::SPECIAL_TYPES sp);

	void SetMax(double max);

	void SetMin(double min);

	void SetDrawName(wxString name);

	void SetShortName(wxString name);

	void SetSetName(wxString window);

	void SetScale(int scale);

	int GetScale();

	void SetScaleMin(double scale);

	double GetScaleMin();

	void SetScaleMax(double scale);

	double GetScaleMax();

	/** User changed long name (title in draw) */
	void SetUnit(wxString unit);

	void ParseXML(xmlNodePtr node);

	void GenerateXML(xmlNodePtr parent);

};

/** Hash map connecting configuration prefixes with number ocurrances of
 * its draw in current draw set (DefinedDrawsSet) */
WX_DECLARE_STRING_HASH_MAP(int, SetsNrHash)
;

/**
 * DrawSet with XML serialising and modyfing methods.
 */
class DefinedDrawSet : public DrawSet {
private:
	DefinedDrawSet(DrawsSets* parent, DefinedDrawSet *set);
public:
	/** Parse XML tree to construct object 
	 * @param cur xml node with window element
	 * @param config ConfigManager */
	DefinedDrawSet(DefinedDrawsSets *cfg, wxString title);

	DefinedDrawSet(DefinedDrawsSets* cfg);

	DefinedDrawSet(DefinedDrawsSets *cfg, wxString title, std::vector<DefinedDrawInfo::EkrnDefDraw>& v);

	virtual ~DefinedDrawSet();

	/** Swaps two params given by its positions */
	void Swap(int a, int b);

	/** Removes draw
	 * @param row draw id - index in list
	 */
	void Remove(int idx);
	/** Returns given draw
	 * @param row draw id - index in list
	 * @return draw
	 */
	DefinedDrawInfo *GetDraw(int idx);

	/** Replaces draw info at given index with the one provieded as argument*/
	void Replace(int idx, DefinedDrawInfo * ndi);

	void Add(const std::vector<DrawInfo*>& draws, bool make_color_uniqe = false);

	void Add(DefinedDrawInfo *i);

	/** Sets new name to DefinedDrawsSet and new window names to
	 * all its draws
	 * @param title new title
	 */
	virtual void SetName(const wxString& name);

	/** Removes all elemets */
	void Clear();
	/** Adds data to XML tree */
	void GenerateXML(xmlNodePtr root);

	/**
	 * Parses XML tree to get DrawInfo data*/
	void ParseXML(xmlNodePtr cur);

	/** @return array holding prefixes of configraions that 
	 * param of this window originate from*/
	SetsNrHash& GetPrefixes();

	DefinedDrawSet *MakeDeepCopy();

	DefinedDrawSet *MakeShallowCopy(DrawsSets* parent);

	virtual double GetPrior();

	void SetPrior(double prior);

	void ConfigurationWasLoaded(wxString prefix, std::set<wxString> *configs_to_load = NULL);

	void GetColor(DefinedDrawInfo *ddi);

	DefinedParam* LookupDefinedParam(wxString prefix, wxString param_name, const std::vector<DefinedParam*>& defined_params);

	bool SyncWithPrefix(wxString prefix);

	void SyncWithAllPrefixes();

	bool SyncWithPrefix(wxString prefix, std::vector<wxString>& removed, const std::vector<DefinedParam*>& defined_params);

	bool RefersToPrefix(wxString prefix);

	std::set<wxString> GetUnresolvedPrefixes();

	bool IsTemporary() {
		return m_temporary;
	}

	bool SetTemporary(bool val) {
		return m_temporary = val;
	}

	DefinedDrawsSets * GetDefinedDrawsSets() {
		return m_ds;
	}

	std::vector<DefinedDrawSet*>* GetCopies();
protected:
	/**Object this sets belongs to*/
	DefinedDrawsSets *m_ds;

	/** Names of configurations' prefixes that params of this window come from*/
	SetsNrHash m_sets_nr;

	/**priority of this set*/
	double m_prior;

	/** If this draw should be saved */
	bool m_temporary;

	bool m_copy;

	std::vector<DefinedDrawSet*> *m_copies;

};

#endif				// _DEFWIN_H
