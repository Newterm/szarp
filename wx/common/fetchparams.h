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
/* $Id$
 *
 * SZARP

 * ecto@praterm.com.pl
 */

#ifndef __FETCHPARAMS_H__
#define __FETCHPARAMS_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/hashmap.h>
#include <wx/thread.h>

#include "httpcl.h"
#include "parlist.h"

#ifndef NO_CURL


/** XSLTD client thread (detached - create only on heap!) */
class szParamFetcher : public wxThread {
	static const int tick_len = 100;
public:
	/** Empty constructor, use Create() to create thread. */
	szParamFetcher();
	/**
	 * @param _baseurl path to xsltd server
	* @param _notify object, which gets wxEVT_SZ_FETCH messages on new data
   	* @param http pointer to HTTP client to use for quering server
	* @return true on success, false on error
	*/
	szParamFetcher(const wxString &_baseurl, wxEvtHandler *_notify,	szHTTPCurlClient* http);

	virtual bool Create(const wxString &_baseurl, wxEvtHandler *_notify,
			szHTTPCurlClient* http);
	/**
	 * Set new base url (path to XSLTD server). Pause thread
	 * before calling this method, because we don't have a proper url to
	 * fetch!
	 * @param baseurl new base url
	 * @return true on success, false on error
	 */
	bool ResetBase(const wxString &baseurl);
	/**
	* Set source to predefined (params.xml) report
	* @return true on success, false on failure
	*/
	bool SetReportName(const wxString &report);

	/**Registers a control raport
	 * @param cname path to control raport resources
	 * @param list of control raport params*/
	bool RegisterControlRaport(const wxString &cname, szParList& params, char *userpwd);
	/**
	* Set source to given param list
	* @return true on success, false on failure
	*/
	bool SetSource(const szProbeList &probes,TSzarpConfig *ipk);

	bool SetSource(const szParList &params);
	/** @return result of last server query */
	szParList& GetParams();
	/**
	* Set query rate
	* @param period time [s] beetween queries
	*/
	void SetPeriod(int period) { m_period = period * 1000; }
	/** Get query rate in seconds. */
	int GetPeriod() { return m_period / 1000; }
	/** Enter critical section */
	inline void Lock() { m_csec.Enter(); }
	/** Leave critical section */
	inline void Unlock() { m_csec.Leave(); }
	/** @return if current data is valid */
	inline bool IsValid() { return m_valid; }
	/** Force fetching on nearest time tick. */
	void ResetTicker();
	/** Sets url to the report*/
	bool SetURL(const wxString& url);
protected:
	/** main thread loop */
	virtual ExitCode Entry();
	/**
	* Set url to query
	* @param url complete query url
	* @return true on success, false on failure
	*/
	bool SetSource_u(const wxString &url);
	/** Make query */
	bool Fetch();

	void SuckParListToProbes(szParList& parlist, bool values);
	/**
	* Register custom report
	* @param params list of params
	*/
	wxString Register(const szParList &params);
	
	int m_period;		/**< query rate (ms) */
	long int m_ticker;	/**< current tick */
	wxString m_baseurl;	/**< path to XSLTD server */
	wxString m_url;		/**< actual query url*/
	szParList m_cust_list;
				/**< list of params for custom report */
	bool m_custom;		/**< true for custom reports */
	wxEvtHandler *m_notify;
				/**< who to notify on fresh data (wxEVT_SZ_FETCH) */
	szHTTPCurlClient *m_http;	/**< HTTP client for quering server */
	bool m_valid;		/**< is current query valid */
	wxCriticalSection m_csec;
				/**< critical section for data access */
	wxCriticalSection m_comsec;
				/**< critical section for communication */
	szParList m_report;
				/**< results from last query - param list */
	bool m_can_fetch;	/**< flag to disable fetching while registering params */
};

BEGIN_DECLARE_EVENT_TYPES()
  DECLARE_LOCAL_EVENT_TYPE(wxEVT_SZ_FETCH, 7000)
END_DECLARE_EVENT_TYPES()

#if wxCHECK_VERSION(2,5,0)
#define EVT_SZ_FETCH(fn) \
  DECLARE_EVENT_TABLE_ENTRY( \
          wxEVT_SZ_FETCH, \
          wxID_ANY, wxID_ANY, \
          (wxObjectEventFunction)(wxEventFunction) \
            wxStaticCastEvent( wxCommandEventFunction, &fn ), \
          (wxObject *) NULL \
  ),
#else //wxCHECK_VERSION
#define EVT_SZ_FETCH(fn) \
  DECLARE_EVENT_TABLE_ENTRY( \
          wxEVT_SZ_FETCH, \
          wxID_ANY, wxID_ANY, \
          (wxObjectEventFunction)(wxEventFunction) \
            (wxCommandEventFunction) &fn, \
          (wxObject *) NULL \
  ),
#endif //wxCHECK_VERSION

#endif // NO_CURL

#endif //__FETCHPARAMS_H__
