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
#ifndef __PANELMGR_H__
#define __PANELMGR_H__

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
#include <wx/event.h>
#endif

#include <tr1/unordered_map>
#include <set>

#include <vector>

#include "sz4/param_observer.h"

/**Event encapsulating response from database*/
class DatabaseResponse : public wxCommandEvent {
	/**@see DatabaseQuery*/
	DatabaseQuery *data;
public:
	DatabaseResponse(DatabaseQuery *_data);

	/**Clones object*/
	virtual wxEvent* Clone() const;

	/**@return encapsulated query*/
	DatabaseQuery *GetQuery();

	virtual ~DatabaseResponse();
};

DECLARE_EVENT_TYPE(DATABASE_RESP, -1) 

typedef void (wxEvtHandler::*DatabaseResponseEventFunction)(DatabaseResponse&);

#define EVT_DATABASE_RESP(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY( DATABASE_RESP, id, -1, \
    (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction) \
    wxStaticCastEvent( DatabaseResponseEventFunction, & fn ), (wxObject *) NULL ),


class ConfigurationChangedEvent : public wxCommandEvent {
	std::wstring m_prefix;
public:
	ConfigurationChangedEvent(std::wstring prefix);
	std::wstring GetPrefix();
	/**Clones object*/
	virtual wxEvent* Clone() const;
	virtual ~ConfigurationChangedEvent();

};

DECLARE_EVENT_TYPE(CONFIGURATION_CHANGE, -1) 

typedef void (wxEvtHandler::*ConfigurationChangedEventFunction)(ConfigurationChangedEvent&);

#define EVT_CONFIGUARION_CHANGE(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY( CONFIGURATION_CHANGE, id, -1, \
    (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction) \
    wxStaticCastEvent( ConfigurationChangedEventFunction, & fn ), (wxObject *) NULL ),

DECLARE_EVENT_TYPE(PARAM_DATA_CHANGED, -1)

class ParamDataChangedEvent : public wxCommandEvent {
	TParam* m_param;
public:
	ParamDataChangedEvent(TParam *param);
	TParam* GetParam();
	/**Clones object*/
	virtual wxEvent* Clone() const;
	virtual ~ParamDataChangedEvent();
};

typedef void (wxEvtHandler::*ParamDataChangedEventFunction)(ParamDataChangedEvent&);
#define EVT_PARAM_DATA_CHANGED(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY( PARAM_DATA_CHANGED, id, -1, \
    (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction) \
    wxStaticCastEvent( ParamDataChangedEventFunction, & fn ), (wxObject *) NULL ),


class SyncedPrefixSet {
	private:
		std::set<wxString> prefixes;
		wxMutex mutex;
	public:
		bool Contains(wxString prefix);
		bool TryAdd(wxString prefix);
		bool Remove(wxString prefix);
};


/**Class managing draw inquirers*/
class DatabaseManager : public wxEvtHandler, public sz4::param_observer {
	typedef std::tr1::unordered_map<InquirerId, DBInquirer*> IH;
	typedef IH::iterator IHI;

	/**@see DatabaseQueryQueue*/
	DatabaseQueryQueue *query_queue;
	/**@see ConfigManager*/
	ConfigManager *config_manager;
	/**current inquirer*/
	InquirerId current_inquirer;
	/**next free inquirer identifier*/
	InquirerId free_inquirer_id;
	/**Panels hash*/
	IH inquirers;

	SyncedPrefixSet reloadingDatabases;

public:
	DatabaseManager(DatabaseQueryQueue *_query_queue, ConfigManager *_config_manager);
	/**Sets current inquirer identifier*/
	void SetCurrentInquirer(InquirerId id);
	/**Forces recalulcation of priorities in @see DatabaseQueryQueue*/
	void InquirerStateChanged();
	/**@return identifier of the current inquirer*/
	InquirerId GetCurrentInquirerId();
	/**@return current param of given inquirer id*/
	DrawInfo *GetCurrentDrawInfoForInquirer(InquirerId id);
	/**@return current date of current inquirer id*/
	time_t GetCurrentDateForInquirer(InquirerId id);
	/**Destorys inquirer of given id*/
	void RemoveInquirer(InquirerId id);

	void CheckAndNotifyAboutError(DatabaseResponse &response);

	/**Returns next free Id for inquirer*/
	InquirerId GetNextId();

	/**Sends clean old request to @see DatabaseQueryQueue*/
	void CleanDatabase(DatabaseQuery *query);
	
	/**Inserts query into @see DatabaseQueryQueue*/
	void QueryDatabase(DatabaseQuery *query);

	void QueryDatabase(std::list<DatabaseQuery*> &qlist);

	/**Handles response from database, the response is directed to proper inquirer
	 * or if such inquirer does not exists anymore - deleted*/
	void OnDatabaseResponse(DatabaseResponse& query);

	void OnConfigurationChange(ConfigurationChangedEvent &e);

	void OnParamDataChanged(ParamDataChangedEvent &e);

	/**Adds object to list of inquirers*/
	void AddInquirer(DBInquirer *inquirer);

	void AddParams(const std::vector<DefinedParam*>& ddi);

	void RemoveParams(const std::vector<DefinedParam*>& ddi);

	void SetProbersAddresses(const std::map<wxString, std::pair<wxString, wxString> > &addresses);

	void InitiateConfigurationReload(wxString prefix);

	void ConfigurationReloadFinished(wxString prefix);

	void ChangeObservedParamsRegistration(const std::vector<TParam*>& to_deregister, const std::vector<TParam*>& to_register);

	void param_data_changed(TParam *param);
	
        DECLARE_EVENT_TABLE()
};
#endif
