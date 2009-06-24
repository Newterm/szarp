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
 * Dialog for selecting SZARP raporter server.
 *

 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 * 
 * $Id$
 */

#include "cconv.h"
#include "serverdlg.h"

#include <wx/config.h>

#define KEY_SAVEDEFAULT	_T("ServerDlg/SaveDefault")
#define KEY_SERVER	_T("ServerDlg/ServerString")

wxString szServerDlg::GetServer(wxString def, wxString progname, bool always_show)
{
	if (!def.IsEmpty() && !always_show) {
		/* There is a default value and we are not forced to always
		 * show dialog. */
		return def;
	}
	
	wxString server;
	bool save_default = false;
	wxConfig::Get()->Read(KEY_SAVEDEFAULT, &save_default);
	
	if (!always_show && save_default) {
		if (wxConfig::Get()->Read(KEY_SERVER, &server, def)) {
			return server;
		}
	} else {
		def = wxEmptyString;
		wxConfig::Get()->Read(KEY_SERVER, &server, def);
	}
	def=server;

	wxDialog* dlg = new wxDialog(NULL, wxID_ANY, progname + wxString(_(": choose server")));
	wxSizer* top_s = new wxBoxSizer(wxVERTICAL);
	top_s->Add(new wxStaticText(dlg, wxID_ANY, 
		_("Enter server address and port\nfor example '192.168.1.1:8081'")),
		0,
		wxALL | wxALIGN_CENTER,
		10);
	wxTextCtrl* text;
	top_s->Add(text = new wxTextCtrl(dlg, wxID_ANY, def),
		0,
		wxALL | wxEXPAND,
		10);
	wxCheckBox* cb;
	top_s->Add(cb = new wxCheckBox(dlg, wxID_ANY, _("Save server address as default")),
		0,
		wxALL | wxALIGN_CENTER,
		10);
	cb->SetValue(save_default);
	wxSizer* but_s = new wxBoxSizer(wxHORIZONTAL);
	wxButton* def_but;
	but_s->AddStretchSpacer(1);
	but_s->Add(def_but = new wxButton(dlg, wxID_OK, _("Apply")), 
		0, 
		wxALL | wxEXPAND, 
		10);
	but_s->AddStretchSpacer(1);
	but_s->Add(new wxButton(dlg, wxID_CANCEL, _("Cancel")), 
		0, 
		wxALL | wxEXPAND,
		10);
	but_s->AddStretchSpacer(1);
	top_s->Add(but_s, 0, wxEXPAND, 0);
	def_but->SetDefault();
	dlg->SetSizer(top_s);
	top_s->SetSizeHints(dlg);
	int ret = dlg->ShowModal();
	if (ret == wxID_OK) {
		wxConfig::Get()->Write(KEY_SAVEDEFAULT, cb->GetValue());
		server = text->GetValue();
		//if (cb->GetValue() == true) {
			wxConfig::Get()->Write(KEY_SERVER, server);
		//}
	} else {
		if(always_show) {
			server = _T("Cancel");
		} else {
			server = wxEmptyString;
		}
	}
	delete dlg;
	return server;
}

TSzarpConfig * szServerDlg::GetIPK(wxString server, szHTTPCurlClient *m_http) {
	wxString path = server + _T("/params.xml");

	wxLogInfo(_T("DEBUG: ipk loading from '%s'"), path.c_str());
	
	TSzarpConfig *ipk = new TSzarpConfig();
	assert (ipk);

	long time = wxGetLocalTimeMillis().ToLong();

	wxProgressDialog* pr = new wxProgressDialog(_("Connecting to server"),
			_("Trying to connect to server ") + server + _(".\nPlease wait."));
	pr->Update(0);
	int ret;
	xmlDocPtr doc = m_http->GetXML(const_cast<char*>(SC::S2A(path).c_str()), NULL, NULL);
	wxString msg;
	if (doc == NULL) {
		ret = -1;
		if (m_http->GetError() != 0) {
			msg = SC::A2S(m_http->GetErrorStr());
		}
	} else {
		ret = ipk->parseXML(doc);
		xmlFreeDoc(doc);
	}

	time = wxGetLocalTimeMillis().ToLong() - time;
	if(time < 1000)
		wxMilliSleep(1000 - time);

	delete pr;
	if (ret) {
		wxMessageBox(_("Unable to connect to server ")
				+ server + _(".\n") +
				msg +
				_("\nIPK is not available.\n"),
				_("Connect error"), wxICON_ERROR | wxOK);
		delete ipk;
		ipk = NULL;
		return NULL;
	}
	
	return ipk;
}

bool szServerDlg::GetReports(wxString server, szHTTPCurlClient *m_http, wxString &title, wxArrayString &reports) {
	wxString path = server + _T("/xreports");

	wxLogInfo(_T("DEBUG: loading reports from '%s'"), path.c_str());
	long time = wxGetLocalTimeMillis().ToLong();

	wxProgressDialog* pr = new wxProgressDialog(_("Connecting to server"),
			_("Trying to connect to server ") + server + _(".\nPlease wait."));
	pr->Update(0);

	xmlDocPtr doc = m_http->GetXML(const_cast<char*>(SC::S2A(path).c_str()), NULL, NULL);

	wxString msg;
	bool error = false;

	reports.clear();

	if (doc == NULL) {
		if (m_http->GetError() != 0) {
			msg = SC::A2S(m_http->GetErrorStr()).c_str();
		}
		error = true;
	} else {
		xmlNodePtr node = NULL;

		if (doc->children == NULL || doc->children->children == NULL) {
			error = true;
			msg = _("Invalid document.");
		} else {

			xmlChar *prop = xmlGetProp(doc->children, (xmlChar *)"source");
			title = wxString((const char *)prop, wxConvUTF8);
			xmlFree(prop);

			for (node = doc->children->children; node != NULL; node = node->next) {
				if (node->type == XML_ELEMENT_NODE) {
					prop = xmlGetProp(node, (xmlChar *)"name");
					reports.Add(SC::U2S(prop).c_str());
					xmlFree(prop);
				}
			}
		}

		xmlFreeDoc(doc);
	}
	
	time = wxGetLocalTimeMillis().ToLong() - time;
	if(time < 1000)
		wxMilliSleep(1000 - time);

	delete pr;

	if (error) {
		wxMessageBox(_("Unable to connect to server ")
				+ server + _(".\n") +
				msg +
				_("\nReports are not available.\n"),
				_("Connect error"), wxICON_ERROR | wxOK);
	}

	reports.Sort();


	return !error;
}
