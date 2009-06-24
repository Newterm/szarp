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
#include "cfgnames.h"

#include <map>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/dir.h>
#include <wx/tokenzr.h>
#include <wx/config.h>
#include <libxml/xmlreader.h>
#include "cconv.h"

ConfigNameHash GetConfigTitles(const wxString &szarp_dir) { 

	ConfigNameHash result;

	wxDir dir(szarp_dir);
	wxString prefix;

	std::map<wxString, std::pair<wxString, time_t> > ci;

	wxConfig* cfg = new wxConfig(_T("SZARPCONFIGURATIONS"));
	if (cfg->Exists(_T("CONFIGURATIONS"))) {
		wxString cn = cfg->Read(_T("CONFIGURATIONS"));

		wxStringTokenizer tknz(cn, _T("/"), wxTOKEN_STRTOK);
		while (tknz.HasMoreTokens()) {
			wxString prefix = tknz.GetNextToken();
			if (!tknz.HasMoreTokens())
				break;

			wxString title = tknz.GetNextToken();
			if (!tknz.HasMoreTokens())
				break;

			wxString _time = tknz.GetNextToken();

			long t;
			_time.ToLong(&t);

			ci[prefix] = std::make_pair(title, time_t(t));
		}

	}

	bool changed = false;

	if (dir.GetFirst(&prefix, wxEmptyString, wxDIR_DIRS)) do {
		wxString file_path = szarp_dir + prefix + _T("/config/params.xml");
		if (!wxFile::Exists(file_path))
			continue;

		wxFileName file_name(file_path);

		std::map<wxString, std::pair<wxString, time_t> >::iterator i;
		if ((i = ci.find(prefix)) != ci.end()) {
			time_t t = i->second.second;

			if (t == (long)file_name.GetModificationTime().GetTicks()) {
				result[prefix] = i->second.first;
				continue;
			}

		}
	
		xmlTextReaderPtr reader = xmlNewTextReaderFilename(SC::S2A(file_path).c_str());
		if (reader == NULL)
			continue;

		while (xmlTextReaderRead(reader) == 1) {
			const xmlChar *name = xmlTextReaderConstName(reader);
			if (name == NULL) 
				continue;

			if (xmlStrcmp(name, BAD_CAST "params"))
				continue;

			xmlChar* title = xmlTextReaderGetAttribute(reader, BAD_CAST "title");
			if (title != NULL) {
				wxString _title = SC::U2S(title).c_str();
				result[prefix] = _title;

				changed = true;
				ci[prefix] = std::make_pair(_title, file_name.GetModificationTime().GetTicks());

				xmlFree(title);
			}
			break;
		}
		xmlFreeTextReader(reader);
	} while (dir.GetNext(&prefix));

	if (!changed) {
		delete cfg;
		return result;
	}

	wxString out;
	for (std::map<wxString, std::pair<wxString, time_t> >::iterator i = ci.begin();
			i != ci.end();
			i++) {
		out += i->first + _T("/");
		out += i->second.first + _T("/");

		wxString ts;
		ts << i->second.second;
		out += ts + _T("/");

	}

	cfg->Write(_T("CONFIGURATIONS"), out);
	cfg->Flush();
	delete cfg;

	return result;

}

