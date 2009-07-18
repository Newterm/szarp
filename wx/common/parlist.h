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
 * Parameters list implementation.
 * 

 * pawel@praterm.com.pl
 * 
 * $Id$
 */

#ifndef _PARLIST_H
#define _PARLIST_H

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <libxml/tree.h>
#include <libxml/relaxng.h>
#include "szarp_config.h"

WX_DEFINE_ARRAY(TSzarpConfig *, ArrayOfIPK);

#define SZPARLIST_NS_HREF "http://www.praterm.com.pl/SZARP/list"

class szParList {
public:
	/**
	 * Creates empty list.
	 */
	szParList();
	/** Copy constructor. */
	szParList(const szParList& list);
	/** Assigment operator */
	szParList& operator=(const szParList& list);
	/** Constructor from XML document.
	 * @param xml not-NULL pointer to XML document, no copy is made so 
	 * do not delete or modify passed document! If passed document doesn't
	 * conform with list schema, document is freed and empty list is
	 * created.
	 */
	szParList(const xmlDocPtr xml);
	/** Destructor. Doesnt delete IPK objects. */
	~szParList();
	/** Makes list empty. */
	void Clear();
	/**
	 * Load parameters from file to list. 
	 * @param path path to file with parameters; if empty dialog to select
	 * name of file is displayed.
	 * @param showErrors if TRUE message is displayed on error
	 * @return number of parameters loaded, 0 on error
	 */
	int LoadFile(wxString path = wxEmptyString, bool showErrors = TRUE);
	/**
	 * Clears list and loads parameters from XML document.
	 * @param xml not-NULL pointer to XML document, no copy is made so 
	 * do not delete or modify passed document! If passed document doesn't
	 * conform with list schema, document is freed and empty list is
	 * created.
	 * @return number of parameters loaded, 0 on error
	 */
	int LoadXML(const xmlDocPtr xml);
	/**
	 * Saves list to file.
	 * @param path path to file; if empty last name used with load or save is
	 * used or dialog to select name of file is displayed.
	 * @param showErrors if TRUE message is displayed on error
	 * @return TRUE on success or when user dismissed file selecting dialog, 
	 * FALSE on error
	 */
	bool SaveFile(wxString path = wxEmptyString, bool showErrors = TRUE);
	/**
	 * @return TRUE if list should be saved to preserve modifications
	 */
	bool IsModified();

	/** Register IPK configuration and binds parameter list with this
	 * configuration. All parameters are linked to corresponding TParam objects.
	 * Sets name of default configuration if it's the first one registered.
	 * Should be called before loading or adding any parameters.
	 * @param ipk pointer to IPK object
	 */
	void RegisterIPK(TSzarpConfig *ipk);

	/** Checks if IPK configuration is already registered.
	 * @param ipk configuration to check
	 * @return TRUE if configuration with the same name as given is registered,
	 * false otherwise
	 */
	bool IsRegisteredIPK(TSzarpConfig *ipk);
	/** Returns number of parameters not bound to any configuration. */
	size_t Unbinded();
	/** 
	 * Removes from configuration all parameters not bound to any configuration.
	 */
	void RemoveUnbinded();
	/**
	 * Returns default name of configuration or wxEmptyString.
	 */
	wxString GetConfigName();
	
	/** @return TRUE if list is empty */
	bool IsEmpty();
	/** @return number of parameters on list */
	size_t Count() const;
	/**
	 * Returns name of parameter with given index.
	 * @param index index of param on list
	 * @return name of parameter, wxEmptyString if not found
	 */ 
	wxString GetName(size_t index);
	/**
	 * Returns pointer to XML node.
	 * @param index index of param on list
	 * @return pointer to XML node in tree corresponding to element with
	 * given index; NULL if not found
	 */
	xmlNodePtr GetXMLNode(size_t index) const;
	/**
	 * Returns pointer to root XML node.
	 * @return pointer to root XML node in tree
   */
	xmlNodePtr GetXMLRootNode();
	/**
	 * Returns pointer to parameter object.
	 * @param index index of param on list
	 * @return pointer to TParam object corresponding to element with
	 * given index; NULL if not found
	 */
	TParam* GetParam(size_t index);

	/**
	 * @return pointer to XML document holding params list */
	xmlDocPtr GetXML() const
	{
		return xml;
	}
	
	/**  Returns value of aditional property (from extra namespace). 
	 * @param index index of parameter in list
	 * @param ns namespace URI
	 * @param attrib name of property
	 * @return value of property or wxEmptyString if not found
	 */
	wxString GetExtraProp(size_t index, const wxString &ns, 
			const wxString &attrib);
	/**  Sets value of aditional property (from extra namespace). 
	 * @param index index of parameter in list
	 * @param ns namespace URI
	 * @param attrib name of property
	 * @param value value of property or wxEmptyString to remove property
	 */
	void SetExtraProp(size_t index, const wxString &ns, 
			const wxString &attrib, const wxString &value);
	/**  Returns value of aditional property (from extra namespace) from
	 * root list element.
	 * @param ns namespace URI
	 * @param attrib name of property
	 * @return value of property or wxEmptyString if not found
	 */
	wxString GetExtraRootProp(const wxString &ns, const wxString &attrib);
	/**  Sets value of aditional property (from extra namespace) for root
	 * list element.
	 * @param index index of parameter in list
	 * @param ns namespace URI
	 * @param attrib name of property
	 * @param value value of property or wxEmptyString to remove property
	 */
	void SetExtraRootProp(const wxString &ns, const wxString &attrib, 
			const wxString &value);
	/** Add new namespace declaration to list.
	 * @param prefix namespace prefix
	 * @param href namespace URI
	 */
	void AddNs(const wxString &prefix, const wxString &href);

	/**
	 * Appends parameter to list. Registers new IPK if necessary. Creates 
	 * default XML node.
	 * @param param pointer to parameter to register (pointer is not copied)
	 * @return index of appended parameter
	 */
	size_t Append(TParam *param);
	/**
	 * Inserts parameter to list. Registers new IPK if necessary. Creates 
	 * default XML node.
	 * @param param pointer to parameter to register (pointer is not copied)
	 * @param index parameter is inserted before position given position or at 
	 * the end if index is bigger then Count() + 1.
	 * @return index of inserted parameter
	 */
	size_t Insert(TParam *param, size_t index);
	/**
	 * Removes parameter with given index. Does nothing if item doesn't exist.
	 */
	void Remove(size_t index);
	/**
	 * Creates new wxListBox widget from parameters list.
	 * Sets client data to pointer to TParam objects.
	 */
	wxListBox* AsListBox(wxWindow* parent, wxWindowID id, 
			const wxPoint& pos = wxDefaultPosition, 
			const wxSize& size = wxDefaultSize,
			long style = 0);
	/**
	 * Clears given listbox object and fills with current parameters list.
	 * @param lbox wxListBox object to fill
	 */
	void FillListBox(wxListBox* lbox);

	/** 
	 * Print list of parameters. Currently under Linux kde print tool 'kprinter' 
	 * is launched.
	 * @param parent parent window for printer setup dialog
	 */
	void Print(wxWindow *parent);
	/** 
	 * Show printing preview.
	 * @param parent parent window for preview widget
	 */
	void PrintPreview(wxFrame *parent);
	/** Frees memory used by RelaxNG schema. */
	static void Cleanup();
	/** Binds parameters to configuration. */
	void Bind(TSzarpConfig *ipk);
	/** Tries to bind parameters to all configs. */
	void BindAll();
	/** Creates empty xml, does nothing if XML tree exists */
	void CreateEmpty();
	/** If this flag is set, list will be constructed from xml provided by @see LoadXML method*/
	bool& ConstructFromXML() { return construct_from_xml; }
protected:
	/** Loads RelaxNG schema. Does nothing if schemat is already loaded.
	 * Exits on failure. */
	static void LoadSchema();

	/** Performs construction of list from current xml file*/
	void DoConstructFromXML();

	xmlDocPtr xml;	/**< XML tree for holding list */
	
	/** Class-wide RelaxNG schema, used for validating loaded documents. */
	static xmlRelaxNGPtr list_rng;

	wxString last_path;	/**< Last path used in load or save. */

	bool is_modified;	/**< Modify flag */

	ArrayOfIPK ipks;	/**< Array of registered configurations. */

	bool construct_from_xml;
};
class szProbe {
public:
  wxString m_parname;
  TParam *m_param;
  bool m_formula;
  bool m_value_check;
  double m_value_min;
  double m_value_max;
  int m_data_period;
  int m_precision;
  int m_alarm_type;
  int m_alarm_group;
  wxString m_alt_name;
  /** */
  szProbe() { Set(_T(""), NULL, false, false, 0, 1, 0, 4, 0, 0, _T("")); }
  /** */
  szProbe(const szProbe &s) { Set(s); }
  /** */
  szProbe(wxString _parname, TParam *_param,
      bool _formula, bool _value_check,
      double _value_min, double _value_max,
      int _data_period, int _precision,
      int _alarm_type, int _alarm_group,
      wxString _alt_name) {
    Set(_parname, _param, _formula, _value_check,
      _value_min, _value_max, _data_period, _precision,
      _alarm_type, _alarm_group, _alt_name);
  }
  /** */
  inline void Set(wxString _parname, TParam *_param,
      bool _formula, bool _value_check,
      double _value_min, double _value_max,
      int _data_period, int _precision,
      int _alarm_type, int _alarm_group,
      wxString _alt_name) {
    m_parname = _parname; m_param = _param;
    m_formula = _formula; m_value_check = _value_check;
    m_value_min = _value_min; m_value_max = _value_max;
    m_data_period = _data_period; m_precision = _precision;
    m_alarm_type = _alarm_type; m_alarm_group = _alarm_group;
    m_alt_name = _alt_name;
  }
  /** */
  inline void Set(const szProbe &s) {
    Set(s.m_parname, s.m_param, s.m_formula, s.m_value_check,
        s.m_value_min, s.m_value_max, s.m_data_period, s.m_precision,
        s.m_alarm_type, s.m_alarm_group, s.m_alt_name);
  }
  /** */
  //inline void Set(const szProbe *s) { Set(*s); }
};

WX_DECLARE_LIST(szProbe, szProbeList);


#endif //_PARLIST_H
