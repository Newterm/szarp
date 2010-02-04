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
 * Remote Parametr Setter GUI
 * SZARP
 * 
 *
 * $Id$
 */

#ifndef _PSC_GUI_H
#define _PSC_GUI_H

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <list>
#include <map>
#include <set>
#include <algorithm>

#include <wx/gbsizer.h>
#include <wx/grid.h>
#include <wx/listctrl.h>

#include <wx/treectrl.h>
#include <wx/notebook.h>

#include <libxml/tree.h>

#include <wx/socket.h>

#include <libxml/tree.h>

#include <libxml/relaxng.h>
#include "szarp_config.h"

class PscGUI;
class PscPacksWidget;
class PscPackEdit;

enum PackType {
	WEEKS,
	DAYS
};
/** Information about parameter */
class PscParamInfo
{
public:
	/** Constructor
	 * @param is_pack flag set to true if parameter is pack,
	 * false if constant
	 * @param nr number of parameter in regulator
	 * @param min minimal value parameter can have
	 * @param max maximal value parameter can have
	 * @param prec precision of parameter
	 * @param unit unit of parameter
	 * @param desc description
	 * @param param_name name of IPK parameter connected to
	 * @param param TParam object describing IPK parameter
	 */ 
	PscParamInfo(bool is_pack, int number, int min, int max, int prec, wxString unit, wxString desc, wxString param_name);

	/**
	 * @return true if parameter is pack false if constant
	 */ 
	bool IsPack() { return m_is_pack; }

	/**
	 * @return number of parameter in regulator
	 */ 
	int GetNumber() { return m_number; }

	/**
	 * @return minimal value parameter can have
	 */ 
	int GetMin() { return m_min; }
		
	/**
	 * @return maximal value parameter can have
	 */ 
	int  GetMax() { return m_max; }

	/**
	 * @return precision of parameter
	 */ 
	int GetPrec() { return m_prec; }
	
	/**
	 * @return unit of parameter
	 */ 
	wxString GetUnit() { return m_unit; }

	/** 
	 * @return name of IPK parameter connected to
	 */
	wxString GetParamName() { return m_param_name; }
		
	/**
	 * @return description if non-empty param_name otherwise
	 */ 
	wxString GetDescription();

private:
	/** 
	 * flag set to true if parameter is pack, false if constant
	 */ 
	bool m_is_pack;
		
	/** 
	 * number of parameter in regulator
	 */ 
	int m_number;
		
	/** 
	 * minimal value parameter can have
	 */ 
	int m_min;

	/** 
	 * @param max maximal value parameter can have
	 */ 
	int m_max;

	/** 
	 * precision of parameter
	 */ 
	int m_prec;

	/** 
	 * unit of parameter
	 */ 
	wxString m_unit;
		
	/** 
	 * description
	 */ 
	wxString m_description;

	/** 
	 * name of IPK parameter connected to
	 */ 
	wxString m_param_name;
		
	/** Device unit */
	wxString m_dev_unit;
};

/**Structure describing strting time of pack*/
struct PackTime {
	/**starting day*/
	int day;
	/**starting hour*/
	int hour;
	/**starting minute*/
	int minute;
	bool operator<(const PackTime& pt) const;
};

/**Handles the 'logic' associated with packs setting*/
class PacksParamMapper {
	/**Available packs numbers*/
	std::set<int> m_params;
	/**Values of packs*/
	std::map<PackTime, std::map<int, int> > m_packs;
	/**Finds initial pack's value of given param
	 * @param param number of param to find value of
	 * @param value output param - found value
	 * @return true if first value for pack exsists*/
	bool FindFirstValue(int param, int *value);
public:
	PacksParamMapper();
	/**Reorganizes @see m_packs to simplest possible form*/
	void Canonicalize();
	/**@retrun true if there are more than zero params in the object*/
	bool HasPacks();
	/**Clears all packs values*/
	void Reset();
	/**Inserts new packs' value
	 * @param pt time of pack
	 * @param param number of param
	 * @param value value of param*/
	void Insert(const PackTime &pt, int param, int value);
	/**Removes value of param*/
	void Remove(const PackTime &pt, int param);
	/**Return values of given param
	 * @param param number of param*/
	std::map<PackTime, int> GetParamValues(int param);
	/**@return a 'mapping' - values of packs in a form where
	 * for each @see PackTime value of every param is specified.
	 * This is the form that can be directly fed to a regulator*/
	std::map<PackTime, std::map<int, int> > GetMapping();
};


class ParamHolder  {
protected:
	std::map<int, PscParamInfo*> m_packs_info;
	std::map<int, PscParamInfo*> m_consts_info;

	std::map<int, int> m_consts_values;
	PacksParamMapper m_packs_values;

	PackType m_pack_type;
	virtual ~ParamHolder() { }
public:
	virtual std::map<int, PscParamInfo*>& GetConstsInfo();
	virtual std::map<int, PscParamInfo*>& GetPacksInfo();
	virtual std::vector<PscParamInfo*> GetParamsInfo();

	virtual void SetConstsInfo(const std::map<int, PscParamInfo*>& pi);
	virtual void SetPacksInfo(const std::map<int, PscParamInfo*>& pi);

	virtual void SetPacksType(PackType pack_type);
	virtual PackType GetPacksType();

	virtual PacksParamMapper& GetPacksValues();
	virtual std::map<int, int>& GetConstsValues();

	virtual void SetConstsValues(std::map<int,int>& vals);
	virtual void SetPacksValues(PacksParamMapper &pm);

};

struct PscReport {
	std::vector<short> values;
	wxDateTime time;
	wxString nE, nL, nb, np;
	bool ParseResponse(xmlDocPtr doc);
};

class PscRegulatorData {
	/** Values of constants.*/
	std::map<int, int> m_consts;

	PacksParamMapper m_packs;

	/**Eprom number*/
	wxString m_eprom;

	/**Program number*/
	wxString m_program;

	/**Library number*/
	wxString m_library;

	/**Build number*/
	wxString m_build;

	PackType m_pack_type;

	bool m_ok;

	wxString m_error;

	/**Parse packs portion of regualtor response*/
	void ParsePacksResponse(xmlNodePtr node);

	/**Parse consts portion of regulator response*/ 
	void ParseConstsResponse(xmlNodePtr node);

public:
	std::map<int, int>& GetConstsValues() { return m_consts; }

	/**@see PacksParamMapper*/
	PacksParamMapper& GetPacksValues() { return m_packs; }

	PackType GetPacksType() { return m_pack_type; }

	const wxString& GetEprom() { return m_eprom; }

	const wxString& GetProgram() { return m_program; }

	const wxString& GetLibrary() { return m_library; }

	const wxString& GetBuild() { return m_build; }

	bool ResponseOK() { return m_ok; }

	const wxString& GetError() { return m_error; }

	/**Parse responses from regulator*/
	bool ParseResponse(xmlDocPtr node);

};

class SSLSocketConnection;

/**Object handling connections*/
class PSetDConnection : wxEvtHandler {
	/**server's port and address*/
	wxIPV4address m_ip;
	/**transmit buffer*/
	xmlChar* m_buffer;
	/**buffer position, i.e. place where data are inserted*/
	int m_buffer_pos;
	/**size of buffer*/
	int m_buffer_size;

	/**socket handling connection*/
	SSLSocketConnection *m_socket;

	/**object receiving events related to connection*/
	wxEvtHandler *m_receiver;

	enum {
		IDLE,			/**<nothing intersting happens*/
		CONNECTING,		/**<tcp connection process is in progress*/
		READING,		/**<data are read from the server*/
		WRITING			/**<data are written to the server*/
	} m_state;		/**<connection state*/

	/**closes connection*/
	void Cleanup();

	/**relases buffer*/
	void FreeBuffer();

	/**connects to a server*/
	void Connect();

	/**write query to the server
	 * @return true if operation succeeds, false if it failes or is still in progress*/
	bool WriteToServer();

	/**reads response from the the server
	 * @return true if operation succeeds, false if it failes or is still in progress*/
	bool ReadFromServer();

	/**Drives communication process. Takes actions depending on current state*/
	void Drive();

	/**Handles communication error. Cleans up buffer, sends event informing about that fact to @see m_handler*/
	void Error();

	/**Parsers response and passes it to @see m_handler*/
	void Finish();

	/**handles socket event, react appropriately*/
	void SocketEvent(wxSocketEvent& event);

	static const int max_buffer_size;

public:
	PSetDConnection(const wxIPV4address &ip, wxEvtHandler *receiver);

	/**Sends query to a server
	 * @param query query to send*/
	bool QueryServer(xmlDocPtr query);

	/**Cancels query*/
	void Cancel();

	DECLARE_EVENT_TABLE()
};

/**Object of this class carry responses from @see PSetDConnection*/
class PSetDResponse : public wxCommandEvent {
	/**indicates if communication finished successfully. i.e. parseable response was recevied from server*/
	bool m_ok;
	/**parsed response*/
	xmlDocPtr m_response;
public:
	PSetDResponse(bool ok, xmlDocPtr response);

	virtual wxEvent* Clone() const;

	/**@return @see m_ok*/
	bool Ok() const;

	/**@return @see m_response*/
	xmlDocPtr GetResponse() const;

};

DECLARE_EXPORTED_EVENT_TYPE(WXEXPORT, PSETD_RESP, -1)

typedef void (wxEvtHandler::*PSetDResponseEventFunction)(PSetDResponse&);

#define EVT_PSETD_RESP(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY( PSETD_RESP, id, -1, \
    (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction) \
    wxStaticCastEvent( PSetDResponseEventFunction, & fn ), (wxObject *) NULL ),


class PscFrame : public wxFrame, public ParamHolder {
protected:
	PscPacksWidget *m_packs_widget;

	wxSizer *m_main_sizer;

		/**Grid for constants editing values*/
	wxGrid *m_const_grid;

		/**Grid for packs editing */
	wxGrid *m_pack_grid;

		/**Notebook holding @see m_pack_grid and @see m_const_grid*/
	wxNotebook *m_notebook;

	void EnableEditingControls(bool enable);

	void StartWaiting();

	void StopWaiting();

	virtual void DoHandlePsetDResponse(bool ok, xmlDocPtr doc) = 0;

	virtual void DoHandleCloseMenuItem(wxCommandEvent& event) = 0;

	virtual void DoHandleReload(wxCommandEvent& event) = 0;

	virtual void DoHandleConfigure(wxCommandEvent& event) = 0;

	virtual void DoHandleSetPacksType(PackType pack_type) = 0;

	virtual void DoHandleSetPacks(wxCommandEvent &event) = 0;

	virtual void DoHandleSetConstants(wxCommandEvent &event) = 0;

	virtual void DoHandleReset(wxCommandEvent& event) = 0;

	virtual void DoHandleSaveReport(wxCommandEvent& event) = 0;

	virtual void DoHandleGetReport(wxCommandEvent& event) = 0;
public:
	void OnPSetDResponse(PSetDResponse &event);
	/** Widget constructor */
	PscFrame(wxWindow * parent, wxWindowID id = -1);

	void OnCloseMenuItem(wxCommandEvent &event);

	/** Get params and consts from server */
	void OnReloadMenuItem(wxCommandEvent &event);

	/** Set connection confiugration again */
	void OnConfigureMenuItem(wxCommandEvent &event);
		
	/** Generate set_pack_type XML */
	void OnSetTypeMenuItem(wxCommandEvent &event);

	/** Generate reset_packs XML */
	void OnResetMenutItem(wxCommandEvent &event);

	/** Generate set_packs XML */
	void OnSetPacksMenuItem(wxCommandEvent &event);

	/** Generate set_constans XML */
	void OnSetConstantsMenuItem(wxCommandEvent &event);

	void OnGetReportMenuItem(wxCommandEvent& event);

	void OnSaveReportMenuItem(wxCommandEvent& event);

	/** Start edition of packs. Event ignored for consts grid*/
	void OnGridEdit(wxGridEvent &event);

	/** Verifies and update constant value of a param. This method does nothing if packs grid is installed*/
	void OnGridEdited(wxGridEvent &event);

	virtual void SetConstsValues(std::map<int,int>& vals);

	virtual void SetPacksType(PackType pack_type);

	virtual void SetPacksValues(PacksParamMapper &pm);

	DECLARE_EVENT_TABLE()
};

/** Pack editing widget */
class PscPacksWidget : public wxDialog {
public:
	/** Widget constructor */
	PscPacksWidget(PscFrame* parent);
	/**Event called upon selection of grid line*/
	void OnSelected(wxListEvent &event);
	/**Dispalys context menu that allows user to edit/remove pack*/
	void OnContextMenu(wxListEvent &event);
	/**Starts editing of pack*/
	void OnMenuEdit(wxCommandEvent &event);
	/**Removes pack*/
	void OnMenuRemove(wxCommandEvent &event);
	/**Adds packs*/
	void OnMenuAdd(wxCommandEvent &event);
	/**Closes window*/
	void OnClose(wxCommandEvent &event);
	/**Sets columns width*/
	void SetColumns();
	/**Updates grid with current values of packs*/
	void UpdateGrid();
	/**@return time of currently selected pack*/
	PackTime GetCurrentTime();
	/**Shows widget for packs editing
	 * @param mapper object holding values of packs
	 * @param packs array of @see PscParamInfo
	 * @param number number of params that packs are to be edited
	 * @param period current period*/
	int ShowModal(PacksParamMapper* mapper, 
			std::map<int, PscParamInfo*>* packs,
			int number, 
			PackType pack_type);
private:
	/**parent @see PscFrame*/
	PscFrame* m_parent;
	/**@see PacksParamMapper*/
	PacksParamMapper* m_mapper;
	/**@see description of packs that are present is currently selected unit configuration*/
	std::map<int, PscParamInfo*>* m_packs_info;
	/**Current values of packs*/
	std::map<PackTime, int> m_pack_vals;
	/**List displaying packs*/
	wxListCtrl* m_tav_list;

	/**@see PscPackEdit*/
	PscPackEdit* m_pack_edit;

	/**index of currently selected pack*/
	int m_index;
	/**number of param that packs we are editing*/
	int m_number;
	/**type of period*/
	PackType m_pack_type;

	/**Starts editinig of currently selected pack*/
	void Edit();
		/** Context menu */
	wxMenu *m_context;
	DECLARE_EVENT_TABLE()
};

/** Data editing widget */
class PscPackEdit: public wxDialog {
	PscParamInfo* m_info;
public:
		/** Widget constructor */
	PscPackEdit(PscPacksWidget* parent);

	/**Starts editing of packs
	 * @param day of pack
	 * @param hour hour of pack
	 * @param minute minute of pack
	 * @param value value of pack
	 * @param info @see PscParamInfo describing editeg param*/
	int ShowModal(int day, int hour, int minute, int value, PscParamInfo *info); 
	/**Simply shows the widget*/
	int ShowModal();
	/**Starts editing of new pack
	 * @param period type of period
	 * @param info @see PscParamInfo object describing edited param*/
	int ShowModal(PackType pack_type, PscParamInfo *info);
	/**@return day of pack*/
	int GetDay();
	/**@return hour of pack*/
	int GetHour();
	/**@return minute of pack*/
	int GetMinute();
	/**@return value of pack*/
	int GetValue();
	/**Verifies data entered by user are valid*/
	virtual bool TransferDataFromWindow();
private:
	/*Updates widgets with bound values for current param*/
	void UpdateBounds();
	/**Widget for weekday choice*/
	wxComboBox* m_day;
	/**Widget for hour choice*/
	wxSpinCtrl* m_hour;
	/**Widget for miunte choice*/
	wxSpinCtrl* m_minute;
	/**Wiget displaying pack value*/
	wxTextCtrl* m_value_text;
	/**Packs value*/
	int m_value;
};

class MessagesGenerator {
	wxString m_path;
	wxString m_speed;
	wxString m_id;

	bool m_has_credentials;
	wxString m_username, m_password;

	xmlDocPtr StartMessage(xmlChar *type);
public:
	MessagesGenerator(wxString username, wxString password, wxString path, wxString speed, wxString id);
	MessagesGenerator(wxString path, wxString speed, wxString id);
	MessagesGenerator() { m_has_credentials = false; }

	void SetUsernamePassword(wxString username, wxString password) { m_has_credentials = true; m_username = username; m_password = password;}
	void SetSpeed(wxString speed) { m_speed = speed; }
	void SetPath(wxString path) { m_path = path; }
	void SetId(wxString id) { m_id = id; }
	xmlDocPtr CreateResetSettingsMessage();
	xmlDocPtr CreateRegulatorSettingsMessage();
	xmlDocPtr CreateSetPacksTypeMessage(PackType type);
	xmlDocPtr CreateGetReportMessage();
	xmlDocPtr CreateSetPacksMessage(PacksParamMapper &vals, PackType pack_type);
	xmlDocPtr CreateSetConstsMessage(std::map<int, int> &vals);

};

class PscConfigurationUnit : public ParamHolder {
	wxString m_path;
	wxString m_id;
	wxString m_speed;
	wxString m_eprom;
	wxString m_library;
	wxString m_program;
	wxString m_build;

	void ParseParam(xmlNodePtr node);

public:
	void ParseDefinition(xmlNodePtr node);
	wxString GetEprom() { return m_eprom; }
	wxString GetSpeed() { return m_speed; }
	wxString GetPath() { return m_path; }
	wxString GetId() { return m_id; }
	wxString GetLibrary() { return m_library; }
	wxString GetProgram() { return m_program; }
	wxString GetBuild() { return m_build; }
	wxString SetSpeed(wxString speed) { return m_speed; }
	bool SwallowResponse(PscRegulatorData &rd);
};

std::vector<PscConfigurationUnit*> ParsePscSystemConfiguration(wxString path);

#endif				// _PSC_GUI_H
