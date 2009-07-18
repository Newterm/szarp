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

#include <boost/shared_ptr.hpp>

#include "biowxsock.h"
#include "drawobs.h"

class RemarkViewDialog;

class Remark {
	boost::shared_ptr<xmlDoc> m_doc;
public:
	Remark(xmlDocPtr doc);
	Remark();

	void SetPrefix(std::wstring prefix);
	void SetBaseName(std::wstring base_name);
	void SetSet(std::wstring set);
	void SetTitle(std::wstring title);
	void SetContent(std::wstring content);
	void SetTime(time_t time);

	std::wstring GetPrefix();
	std::wstring GetBaseName();
	std::wstring GetTitle();
	std::wstring GetContent();
	std::wstring GetSet();
	std::wstring GetAuthor();
	time_t GetTime();

	int GetId();

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
	XMLRPCConnection(wxIPV4address& address, wxEvtHandler *event_handler);

	void PostRequest(XMLRPC_REQUEST request);

	void OnSocketEvent(wxSocketEvent& e);

	void SetIPAddress(wxIPV4address &ip);

	~XMLRPCConnection();

	DECLARE_EVENT_TABLE()
};

class RemarksHandler;
class RemarksConnection : public wxEvtHandler {
	long m_token;

	wxString m_username;
	wxString m_password;

	XMLRPCConnection *m_xmlrpc_connection;

	wxIPV4address m_address;

	enum {
		NONE, 
		FETCHING_REMARKS,
		POSTING_REMARK } m_current_action;

	bool m_pending_remarks_fetch;

	bool m_pending_remark_post;

	bool m_inform_about_successful_fetch;

	time_t m_retrieval_time;

	Remark m_remark_to_send;

	RemarkViewDialog *m_remark_add_dialog;

	RemarksHandler *m_remarks_handler;

	XMLRPC_REQUEST CreateMethodCallRequest(const char* method_name);

	XMLRPC_REQUEST CreateLoginRequest();
	XMLRPC_REQUEST CreateGetRemarksRequest();
	XMLRPC_REQUEST CreatePostRemarkRequest();

	void HandleRemarksResponse(XMLRPC_REQUEST response);
	void HandleLoginResponse(XMLRPC_REQUEST response);
	void HandlePostMessageResponse(XMLRPC_REQUEST response);
	bool HandleFault(XMLRPC_REQUEST response);

	void PostRequest(XMLRPC_REQUEST request);

public:
	RemarksConnection(wxIPV4address &address, RemarksHandler *handler);
	bool Busy();
	void SetRemarkAddDialog(RemarkViewDialog *add_dialog);
	void OnXMLRPCResponse(XMLRPCResponseEvent &event);
	void SetIPAddress(wxIPV4address &ip);
	void SetUsernamePassword(wxString username, wxString password);
	void SetRemarksAddDialog(RemarkViewDialog *dialog);
	void FetchNewRemarks(bool notify_about_success = true);
	void PostRemark(Remark &remark);

	~RemarksConnection();

	DECLARE_EVENT_TABLE()
};

class RemarksResponseEvent : public wxCommandEvent {
	std::vector<Remark> m_remarks;
public:
	RemarksResponseEvent(std::vector<Remark> remarks);
	std::vector<Remark>& GetRemarks();
	wxEvent* Clone() const;

};

DECLARE_EVENT_TYPE(REMARKSRESPONSE_EVENT, -1)


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
	public:
		StoreRemarksQuery(RemarksStorage* storage, std::vector<Remark> &remarks);
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
	void ExecuteStoreRemarks(std::vector<Remark>& remarks);
	std::vector<Remark> ExecuteGetRemarks(const std::wstring& prefix, const std::wstring& set, const time_t& start_date, const time_t &end_date);
public:
	RemarksStorage();

	~RemarksStorage();

	virtual void* Entry();

	void Finish();

	std::set<std::wstring> GetPrefixes();

	void StoreRemarks(std::vector<Remark>& remarks);

	void GetRemarks(const wxString& prefix,
			const wxString& set,
			const wxDateTime &start_date,
			const wxDateTime &end_date,
			wxEvtHandler *handler);
};


class RemarksFetcher;
class RemarksHandler : public wxEvtHandler {

	bool m_configured;

	wxIPV4address m_address;
	wxString m_username;
	wxString m_password;
	wxString m_server;
	bool m_auto_fetch;

	RemarksConnection* m_connection;
	RemarksStorage* m_storage;

	std::vector<RemarksFetcher*> m_fetchers;

	wxTimer m_timer;
public:
	RemarksHandler();

	void AddRemarkFetcher(RemarksFetcher* fetcher);

	void RemoveRemarkFetcher(RemarksFetcher* fetcher);

	void NewRemarks(std::vector<Remark>& remarks);

	void Start();

	void Stop();

	std::set<std::wstring> GetPrefixes();

	bool Configured();

	void GetConfiguration(wxString& username, wxString& password, wxString &server, bool& autofetch);

	void SetConfiguration(wxString username, wxString password, wxString server, bool autofetch);

	RemarksConnection *GetConnection();

	RemarksStorage *GetStorage();

	void OnTimer(wxTimerEvent &event);

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

	void ShowRemark(wxString db_name,
			wxString set_name,
			wxString author,
			wxDateTime time,
			wxString title,
			wxString content);

	void RemarkSent(bool ok, wxString error = _T(""));

	void OnAddButton(wxCommandEvent &event);

	void OnCancelButton(wxCommandEvent &event);

	void OnCloseButton(wxCommandEvent &event);

	DECLARE_EVENT_TABLE()
};


class RemarksListDialog : public wxDialog {
	wxListCtrl* m_list_ctrl;

	RemarksHandler* m_remarks_handler;

	std::vector<Remark> m_displayed_remarks;

	void ShowRemark(int index);
public:
	RemarksListDialog(wxWindow* parent, RemarksHandler *handler);

	void SetViewedRemarks(std::vector<Remark> &remarks);

	void OnClose(wxCloseEvent& event);

	void OnCloseButton(wxCommandEvent& event);

	void OnOpenButton(wxCommandEvent& event);

	void OnRemarkItemActivated(wxListEvent &event);

	DECLARE_EVENT_TABLE()
};

class DrawToolBar;
class DrawInfo;
class RemarksFetcher : public wxEvtHandler, public DrawObserver {
	std::map<int, Remark > m_remarks;

	DrawToolBar *m_tool_bar;

	wxWindow *m_toplevel_window;

	RemarksHandler *m_remarks_handler;

	Draw *m_current_draw;

	void Fetch(Draw *d);

public:
	RemarksFetcher(RemarksHandler *remarks_handler, DrawToolBar *tool_bar, wxWindow* top_level_window);

	virtual void ScreenMoved(Draw* draw, const wxDateTime &start_time);

	virtual void DrawInfoChanged(Draw *draw);

	virtual void Attach(Draw *d);

	virtual void Detach(Draw *d);

	void OnShowRemarks(wxCommandEvent &e);

	void ShowRemarks(const wxDateTime &from_time, const wxDateTime &to_time);

	void RemarksReceived(std::vector<Remark>& remarks);

	void OnRemarksResponse(RemarksResponseEvent &e);

	virtual ~RemarksFetcher();

	DECLARE_EVENT_TABLE()
};
