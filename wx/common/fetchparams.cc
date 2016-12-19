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

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/wx.h>
#include <wx/url.h>
#include <wx/thread.h>
#include <wx/datetime.h>

#include <libxml/xmlreader.h>
#include <libxml/nanohttp.h>
#include <libxml/uri.h>

#include <assert.h>

#include "conversion.h"
#include "fetchparams.h"
#include "parlist.cpp"
#ifndef NO_CURL

using namespace SC;
/*
{
	std::wstring W2S(const wxString& c) {
		return (std::wstring(c.ToStdWstring()));
	}
}
*/

szParamFetcher::szParamFetcher()
	: wxThread()
{
}

szParamFetcher::szParamFetcher(const wxString &_baseurl, wxEvtHandler *_notify,
		szHTTPCurlClient* http) : wxThread()
{
	Create(_baseurl, _notify, http);
}

bool szParamFetcher::Create(const wxString &_baseurl, wxEvtHandler *_notify,
		szHTTPCurlClient* http)
{
	m_http = http;
	assert(m_http);
	m_baseurl = _baseurl;
	if (m_baseurl.Right(1) != _T("/")) {
		m_baseurl += _T("/");
	}
	m_notify = _notify;
	m_period = 10000;
	m_ticker = 0;
	m_valid = false;
	m_custom = false;
	wxThread::Create();
	return true;
}
	
bool szParamFetcher::ResetBase(const wxString &baseurl)
{
	//assert (IsPaused());
	m_valid = false;
	m_url = wxEmptyString;
	m_baseurl = baseurl;
	// TODO some validation
	return !m_baseurl.IsEmpty();
}

bool szParamFetcher::SetSource_u(const wxString &url) 
{
	m_url = url;
	if (url.IsEmpty()) {
		m_valid = false;
	} else {
		m_valid = true; //FIXME: some kind of Fetch()
	}
	
	return m_valid;
}

bool szParamFetcher::SetReportName(const wxString &report) 
{
	xmlChar *escURI = xmlURIEscapeStr(SC::S2U(SC::W2S(wxString::FromUTF8(report.mb_str()))).c_str(), NULL);
	assert (escURI);
	wxCriticalSectionLocker cs(m_csec);
	wxString url = m_baseurl + _T("xreports?title=") + SC::U2S(escURI).c_str();
	m_custom = false;
	xmlFree(escURI);
	ResetTicker();
	return SetSource_u(url);
}

bool szParamFetcher::RegisterControlRaport(const wxString &cname, szParList& params, char *userpwd) {
	wxCriticalSectionLocker cs(m_csec);
	wxString url = m_baseurl + cname + _T("?set=1");
	size_t ret;

	xmlDocPtr doc = params.GetXML();
	char *buf = m_http->PostXML((char *)SC::S2U(SC::W2S(wxString::FromUTF8(url.c_str()))).c_str(), doc, userpwd, &ret);
	bool ok = buf != NULL;
	free(buf);
	SetSource_u(m_baseurl + cname);
	ResetTicker();
	return ok;
}

bool szParamFetcher::SetURL(const wxString &url) {
	wxCriticalSectionLocker cs(m_csec);
	ResetTicker();
	return SetSource_u(m_baseurl + url);
}

bool szParamFetcher::SetSource(const szParList &params) 
{
	wxCriticalSectionLocker cs(m_csec);
	wxString report = Register(params);

	if ( report.Length() == 0 ) {
		wxLogMessage(_T("fetch: cannot register"));
		return false;
	}
	wxString url = m_baseurl + _T("custom/") +  report;
	m_custom = true;
	m_cust_list = params;
	ResetTicker();
	m_report = szParList();
	return SetSource_u(url);
}

bool szParamFetcher::SetSource(const szProbeList &probes, TSzarpConfig *ipk) 
{
	wxCriticalSectionLocker cs(m_csec);
	szParList params;

	params.RegisterIPK(ipk);
	for (szProbeList::Node *node = probes.GetFirst(); node; node = node->GetNext())
	{
		szProbe *probe = node->GetData();
		if (probe != NULL) {
			params.Append(probe->m_param);
		}
	}

	wxString report = Register(params);

	if ( report.Length() == 0 ) {
		wxLogMessage(_T("fetch: cannot register"));
		return false;
	}
	wxString url = m_baseurl + _T("custom/") +  report;
	m_custom = true;
	ResetTicker();
	return SetSource_u(url);
	
}


szParList& szParamFetcher::GetParams()
{
	return m_report;
}

wxString szParamFetcher::Register(const szParList &params) 
{
	wxString url = m_baseurl + _T("custom/add");

	size_t ret;

	xmlDocPtr doc = params.GetXML();

	char *buf = m_http->PostXML((char *)SC::S2U(SC::W2S(wxString::FromUTF8(url.c_str()))).c_str(), doc, NULL, &ret);
	if (buf == NULL) {
		return wxEmptyString;
	}

	buf[ret] = '\0';

	wxString key = wxString::FromAscii("?title=") + wxString::FromAscii(buf);
	free(buf);
	return key;
}

szParamFetcher::ExitCode szParamFetcher::Entry() 
{
	while (!TestDestroy()) {
		if (m_ticker <= 0) {
			m_ticker = m_period;
			wxDateTime DT1 = wxDateTime::UNow();
			if ( Fetch() ) {
				if ( m_notify != NULL ) {
					wxCommandEvent ev(wxEVT_SZ_FETCH);
					wxPostEvent(m_notify, ev);
				}
			} 
			wxDateTime DT2 = wxDateTime::UNow();
			wxTimeSpan ts = DT2.Subtract( DT1 );
			m_ticker -= ts.GetMilliseconds().ToLong();
			wxLogMessage(_T("fetch: in %ldms"), ts.GetMilliseconds().ToLong());
		}
		m_ticker -= tick_len;
		Sleep(tick_len);
	}
	return NULL;
}

void szParamFetcher::ResetTicker()
{
	m_ticker = 0;
}

bool szParamFetcher::Fetch() 
{
	if ( m_url.Length() == 0 ) {
		wxLogMessage(_T("fetch: url not set"));
		return false;
	}

	xmlDocPtr xml = m_http->GetXML((char *)SC::S2U(SC::W2S(wxString::FromUTF8(m_url.c_str()))).c_str(), NULL, 0);
	if (xml == NULL) {
		if (m_custom && m_http->GetError() == 0) /**register raport again (only if ti isn't a network failuer*/
			SetSource(m_cust_list);
		return false;
	}

	wxCriticalSectionLocker cs(m_csec);
	m_report.LoadXML(xml);
	return true;
}
	
DEFINE_LOCAL_EVENT_TYPE(wxEVT_SZ_FETCH)

#endif // NO_CURL
