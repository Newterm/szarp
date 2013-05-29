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

/* Remarks handling code 
 *
 * $Id$
 */

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#include <wx/thread.h>
#include <wx/listctrl.h>
#include <wx/socket.h>
#endif

#include <vector>
#include <list>
#include <map>
#include <set>
#include <openssl/ssl.h>
#include <sqlite3.h>
#include <xmlrpc-epi/xmlrpc.h>
#include <libxml/tree.h>

#ifndef MINGW32

/* cfglogin includes */
#include <deque>

#endif /*MINGW32*/

#include <boost/shared_ptr.hpp>

#include "biowxsock.h"

#include "wxlogging.h"

class Remark {
public:

	typedef std::pair<int, int> ID;
private:
	boost::shared_ptr<xmlDoc> m_doc;
	ID m_id;
public:
	Remark(xmlDocPtr doc);
	Remark();

	void SetPrefix(std::wstring prefix);
	void SetBaseName(std::wstring base_name);
	void SetSet(std::wstring set);
	void SetTitle(std::wstring title);
	void SetContent(std::wstring content);
	void SetAttachedPrefix(std::wstring prefix);
	void SetTime(time_t time);

	std::wstring GetPrefix();
	std::wstring GetBaseName();
	std::wstring GetTitle();
	std::wstring GetContent();
	std::wstring GetSet();
	std::wstring GetAuthor();
	std::wstring GetAttachedPrefix();
	time_t GetTime();

	ID GetId();
	void SetId(ID id);

	xmlDocPtr GetXML();

	~Remark();
};


class XMLRPCResponseEvent : public wxCommandEvent {
	boost::shared_ptr<_xmlrpc_request> m_request;
public:
	XMLRPCResponseEvent(XMLRPC_REQUEST request);
	boost::shared_ptr<_xmlrpc_request> GetRequest();
	wxEvent* Clone() const;

};

class XMLRPCConnection : public wxEvtHandler {

	SSLSocketConnection *m_socket;

	wxEvtHandler *m_handler;

	enum READ_STATE {
		CLOSED,
		READING_FIRST_LINE,
		READING_HEADERS,
		READING_CONTENT } m_read_state;


	std::string m_write_buf;
	size_t m_write_buf_pos;

	char* m_read_buf;
	size_t m_read_buf_read_pos;
	size_t m_read_buf_write_pos;	
	size_t m_read_buf_size;	
	size_t m_content_length;

	std::string CreateHTTPRequest(XMLRPC_REQUEST request);

	bool GetLine(std::string& line);

	void ReturnError(std::wstring error);

	void ReturnResponse();

	void ReadFirstLine(bool& more_data);

	void ReadHeaders(bool& more_data);

	void ReadContent(bool& more_data);

	void ProcessReadData();

	void ReadData();

	void WriteData();
public:
	XMLRPCConnection(wxString& address, wxEvtHandler *event_handler);

	void PostRequest(XMLRPC_REQUEST request);

	void OnSocketEvent(wxSocketEvent& e);

	void SetIPAddress(const wxString &ip);

	~XMLRPCConnection();

	DECLARE_EVENT_TABLE()
};

class DNSResponseEvent : public wxCommandEvent {
	std::wstring m_address;	
public:
	DNSResponseEvent(std::wstring address);
	std::wstring GetAddress();
	wxEvent* Clone() const;
};


class DNSResolver : public wxThread {
	wxEvtHandler *m_response_handler;
	bool m_finish;
	wxMutex m_mutex;
	wxSemaphore m_work_semaphore;
	std::wstring m_address;
	void DoResolve();
	bool CheckFinish();
public:
	DNSResolver(wxEvtHandler *event);	
	void Resolve(wxString address);
	virtual void* Entry();
	void Finish();
};

class RemarksConnection : public wxEvtHandler {
	DNSResolver m_resolver;

	long m_token;

	wxString m_username;
	wxString m_password;

	XMLRPCConnection *m_xmlrpc_connection;

	ConfigManager *m_config_manager;

	wxString m_address;

	class Command {
	protected:
		RemarksConnection* m_connection;
	public:
		Command(RemarksConnection* connection);
		virtual void Error(wxString error) = 0;
		virtual void Send() = 0;
		virtual void HandleResponse(XMLRPC_REQUEST) = 0;
	};

	class FetchRemarksCommand : public Command {
		RemarksHandler* m_remarks_handler;
		bool m_manually_triggered;
		time_t m_retrieval_time;
	public:
		FetchRemarksCommand(RemarksConnection* connection, RemarksHandler* remarks_handler, bool manually_triggered);
		virtual void Error(wxString error);
		virtual void Send();
		virtual void HandleResponse(XMLRPC_REQUEST);
	};

	class PostRemarkCommand : public Command {
		Remark m_remark;
		RemarkViewDialog *m_remark_view_dialog;
	public:
		PostRemarkCommand(RemarksConnection* connection, const Remark& remark, RemarkViewDialog *remark_view_dialog);
		virtual void Error(wxString error);
		virtual void Send();
		virtual void HandleResponse(XMLRPC_REQUEST);
	};

	class InsertOrUpdateParamCommand : public Command {
		xmlDocPtr m_param_xml;
		ParamEditControl *m_control;
		bool m_del;
	public:
		InsertOrUpdateParamCommand(RemarksConnection* connection, DefinedParam *param, ParamEditControl *control, bool del);
		virtual void Error(wxString error);
		virtual void Send();
		virtual void HandleResponse(XMLRPC_REQUEST);
		virtual ~InsertOrUpdateParamCommand();
	};

	class InsertOrUpdateSetCommand : public Command {
		xmlDocPtr m_set_xml;
		SetEditControl *m_control;
		bool m_del;
	public:
		InsertOrUpdateSetCommand(RemarksConnection* connection, DefinedDrawSet *param, SetEditControl *control, bool del);
		virtual void Error(wxString error);
		virtual void Send();
		virtual void HandleResponse(XMLRPC_REQUEST);
		virtual ~InsertOrUpdateSetCommand();
	};

	class FetchParamsAndSetsCommand : public Command {
		ConfigManager* m_cfg_mgr;
		SetsParamsReceivedEvent* m_event;
		time_t m_fetch_time;

		void ProcessParams(XMLRPC_VALUE vs, bool& params_updated);
		DefinedDrawInfo* ProcessDrawInfo(XMLRPC_VALUE vdi);
		void ProcessSets(XMLRPC_VALUE ss, bool& sets_updated);
		bool AddSet(DefinedDrawSet *set);
	public:
		FetchParamsAndSetsCommand(RemarksConnection* connection, ConfigManager* cfg_mgr, SetsParamsReceivedEvent *m_event = NULL);
		virtual void Error(wxString error);
		virtual void Send();
		virtual void HandleResponse(XMLRPC_REQUEST);
	};

	std::list<boost::shared_ptr<Command> > m_command_list;

	bool m_address_resolved;

	time_t m_retrieval_time;

	RemarksHandler *m_remarks_handler;

	XMLRPC_REQUEST CreateMethodCallRequest(const char* method_name);

	XMLRPC_REQUEST CreateMethodCallRequestWithToken(const char* method_name);

	XMLRPC_REQUEST CreateLoginRequest();

	void HandleLoginResponse(XMLRPC_REQUEST response);
	void HandleFault(XMLRPC_REQUEST response);

	void PostRequest(XMLRPC_REQUEST request);
	void SendCommand();

public:
	RemarksConnection(wxString &address, RemarksHandler *handler, ConfigManager *config_manager);
	bool Busy();
	void OnXMLRPCResponse(XMLRPCResponseEvent &event);
	void OnDNSResponse(DNSResponseEvent &event);
	void NotifyAllCommandsAboutError(const wxString &error);
	void SetIPAddress(wxString &ip);
	void SetUsernamePassword(wxString username, wxString password);
	void SetRemarksAddDialog(RemarkViewDialog *dialog);
	void FetchNewRemarks(bool notify_about_success = true);
	void FetchNewParamsAndSets(SetsParamsReceivedEvent *event = NULL);
	void PostRemark(Remark &remark, RemarkViewDialog *remark_view_dialog);
	void InsertOrUpdateParam(DefinedParam *param, ParamEditControl* control, bool del);
	void InsertOrUpdateSet(DefinedDrawSet *set, SetEditControl* control, bool del);
	void Login();
	void TriggerRequest();

	~RemarksConnection();

	DECLARE_EVENT_TABLE()
};

class RemarksResponseEvent : public wxCommandEvent {
public:
	enum RESPONSE_TYPE {
		RANGE_RESPONE,
		BASE_RESPONSE };

private:
	std::vector<Remark> m_remarks;
	RESPONSE_TYPE m_response_type;
public:
	RemarksResponseEvent(std::vector<Remark> remarks, RESPONSE_TYPE response_type);
	std::vector<Remark>& GetRemarks();
	RESPONSE_TYPE GetResponseType();
	wxEvent* Clone() const;

};

DECLARE_EVENT_TYPE(REMARKSRESPONSE_EVENT, -1)

class RemarksStoredEvent : public wxCommandEvent {
	std::vector<Remark> m_remarks;
	time_t m_fetch_time;
	bool m_manually_triggered;
public:
	RemarksStoredEvent(std::vector<Remark> remarks, time_t fetch_time, bool manually_triggered);
	std::vector<Remark>& GetRemarks();
	time_t GetFetchTime();
	bool GetManuallyTriggered();
	
	wxEvent* Clone() const;
};

DECLARE_EVENT_TYPE(REMARKSSTORED_EVENT, -1)

class RemarksStorage : public wxThread {
	class Query {
	protected:
		RemarksStorage *m_storage;
	public:
		Query(RemarksStorage* storage);
		virtual void Execute() = 0;
		virtual ~Query() {};
	};

	class StoreRemarksQuery : public Query {
		std::vector<Remark> m_remarks;
		time_t m_fetch_time;
		bool m_manually_triggered;
	public:
		StoreRemarksQuery(RemarksStorage* storage, std::vector<Remark> &remarks, time_t fetch_time, bool manually_triggered);
		virtual void Execute();
		virtual ~StoreRemarksQuery() {};
	};

	class FetchRemarksQuery : public Query {
		std::wstring m_prefix;
		std::wstring m_set;
		time_t m_from_time;
		time_t m_to_time;
		wxEvtHandler *m_receiver;
	public:
		FetchRemarksQuery(RemarksStorage* storage, std::wstring prefix, std::wstring set, time_t from_time, time_t to_time, wxEvtHandler *receiver);
		virtual void Execute();
		virtual ~FetchRemarksQuery() {};
	};

	class FetchAllRemarksQuery : public Query {
		std::wstring m_prefix;
		wxEvtHandler *m_receiver;
	public:
		FetchAllRemarksQuery(RemarksStorage* storage, std::wstring prefix, wxEvtHandler *receiver);
		virtual void Execute();
		virtual ~FetchAllRemarksQuery() {};
	};

	class FinishQuery : public Query {
	public:
		FinishQuery(RemarksStorage* storage);
		virtual void Execute();
		virtual ~FinishQuery() {};
	};

	friend class StoreRemarksQuery;
	friend class FetchRemarksQuery;
	friend class FinishQuery;

	sqlite3* m_sqlite;
	sqlite3_stmt* m_fetch_remarks_query;
	sqlite3_stmt* m_fetch_prefixes_query;
	sqlite3_stmt* m_store_remark_query;
	sqlite3_stmt* m_add_prefix_query;
	sqlite3_stmt* m_fetch_all_remarks_query;
	
	/**Semaphore counting number of elements in the queue*/
	wxSemaphore m_semaphore;

	/**Mutex guarding access to a queue*/
	wxMutex m_queue_mutex;

	/**Mutex guarding access to a prefixes list*/
	wxMutex m_prefixes_mutex;

	std::set<std::wstring> m_prefixes;

	std::list<Query*> m_queries;

	bool m_done;

	bool Init(wxString &error);
	bool CreateTable();
	void PrepareQuery();

	void AddPrefix(std::wstring prefix);
	void ExecuteStoreRemarks(std::vector<Remark>& remarks, time_t fetch_time, bool manually_triggered);
	std::vector<Remark> ExecuteGetRemarks(const std::wstring& prefix, const std::wstring& set, const time_t& start_date, const time_t &end_date);
	std::vector<Remark> ExecuteGetAllRemarks(const std::wstring& prefix);

	RemarksHandler *m_remarks_handler;
public:
	RemarksStorage(RemarksHandler *remarks_handler);

	~RemarksStorage();

	virtual void* Entry();

	void Finish();

	std::set<std::wstring> GetPrefixes();

	void StoreRemarks(std::vector<Remark>& remarks, time_t fetch_time, bool manually_triggered);

	void GetRemarks(const wxString& prefix,
			const wxString& set,
			const wxDateTime &start_date,
			const wxDateTime &end_date,
			wxEvtHandler *handler);

	void GetAllRemarks(const wxString& prefix, wxEvtHandler *handler);
};


class RemarksHandler : public wxEvtHandler {

	bool m_configured;

#ifndef MINGW32

        /* cfglogin variables */

        /* cfglogin method of authorization was used */
        bool m_cfglogin;
        /* Number of times login retry was used */
        unsigned int m_cfglogin_cnt;
        /* Hash history */
        std::deque<wxString> hash_history;

        /* Maximum number of times to allow login retry */
        static const unsigned int c_cfglogin_max;
        /* Event id for auto fetch timer */
        static const int c_timer_id;
        /* Event id for cfglogin timer */
        static const int c_cfglogin_timer_id;

        /* cfglogin timer */
        wxTimer m_cfglogin_timer;
        
#endif /*MINGW32*/

	wxString m_username;
	wxString m_password;
	wxString m_server;
	bool m_auto_fetch;

	RemarksConnection* m_connection;
	RemarksStorage* m_storage;
	ConfigManager* m_config_manager;

	std::vector<RemarksFetcher*> m_fetchers;

	wxTimer m_timer;

	void GetConfigurationFromSSCConfig();

#ifndef MINGW32

        /* cfglogin private methods */

        void GetConfigurationFromSzarpCfg();

#endif /*MINGW32*/

public:

	RemarksHandler(ConfigManager *config_manager);

	void AddRemarkFetcher(RemarksFetcher* fetcher);

	void RemoveRemarkFetcher(RemarksFetcher* fetcher);

	void NewRemarks(std::vector<Remark>& remarks, time_t fetch_time, bool manually_triggered);

	void Start();

	void Stop();

	std::set<std::wstring> GetPrefixes();

	bool Configured();

#ifndef MINGW32

        /* cfglogin public methods */

        bool CfgConfigured();
        wxString GetHistoryHash();
        void OnCfgLoginTimer(wxTimerEvent &event);
        
#endif /*MINGW32*/

	void GetConfiguration(wxString& username, wxString& password, wxString &server, bool& autofetch);

	wxString GetUsername() const;

	void SetConfiguration(wxString username, wxString password, wxString server, bool autofetch);

	RemarksConnection *GetConnection();

	RemarksStorage *GetStorage();

	void OnTimer(wxTimerEvent &event);

	void OnRemarksStored(RemarksStoredEvent &event);

	DECLARE_EVENT_TABLE();
};


class RemarkViewDialog : public wxDialog {

	wxString m_prefix;
	wxString m_set_name;
	wxDateTime m_time;

	void SetEditingMode(bool editing_mode);

	RemarksHandler *m_remarks_handler;

	RemarksConnection *m_remark_connection;
public:
	RemarkViewDialog(wxWindow *parent, RemarksHandler *remarks_handler);
	int NewRemark(wxString prefix, 
			wxString db_name,
			wxString set_name,
			wxDateTime time);

	wxString GetRemarkTitle();

	wxString GetRemarkContent();

	int ShowRemark(wxString db_name,
			wxString set_name,
			wxString author,
			wxDateTime time,
			wxString title,
			wxString content);

	void RemarkSent(bool ok, wxString error = _T(""));

	void OnAddButton(wxCommandEvent &event);

	void OnCancelButton(wxCommandEvent &event);

	void OnCloseButton(wxCommandEvent &event);

	void OnGoToButton(wxCommandEvent &event);

	void OnHelpButton(wxCommandEvent &event);

	DECLARE_EVENT_TABLE()

};


class RemarksListDialog : public wxDialog {
	wxListCtrl* m_list_ctrl;

	RemarksHandler* m_remarks_handler;

	std::vector<Remark> m_displayed_remarks;

	void ShowRemark(int index);

	DrawFrame* m_draw_frame;
public:
	RemarksListDialog(DrawFrame* parent, RemarksHandler *handler);

	void SetViewedRemarks(std::vector<Remark> &remarks);

	void OnClose(wxCloseEvent& event);

	void OnCloseButton(wxCommandEvent& event);

	void OnOpenButton(wxCommandEvent& event);

	void OnRemarkItemActivated(wxListEvent &event);

	void OnHelpButton(wxCommandEvent &event);

	DECLARE_EVENT_TABLE()

};

class RemarksFetcher : public wxEvtHandler, public DrawObserver {
	std::map<Remark::ID, Remark> m_remarks;

	DrawFrame *m_draw_frame;

	RemarksHandler *m_remarks_handler;

	DrawsController *m_draws_controller;

	bool m_awaiting_for_whole_base;

	void Fetch();

public:
	RemarksFetcher(RemarksHandler *remarks_handler, DrawFrame* m_draw_frame);

	virtual void ScreenMoved(Draw* draw, const wxDateTime &start_time);

	virtual void DrawInfoChanged(Draw *draw);

	virtual void Attach(DrawsController *d);

	virtual void Detach(DrawsController *d);

	virtual void DrawSelected(Draw *draw);

	void ShowRemarks();

	void OnShowRemarks(wxCommandEvent &e);

	void ShowRemarks(const wxDateTime &from_time, const wxDateTime &to_time);

	void RemarksReceived(std::vector<Remark>& remarks);

	void OnRemarksResponse(RemarksResponseEvent &e);

	virtual ~RemarksFetcher();

	DECLARE_EVENT_TABLE()
};
