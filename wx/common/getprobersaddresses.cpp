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

#include <string>
#include <sstream>
#include <map>
#include <cstdlib>

#include <wx/string.h>

#include "conversion.h"
#include "libpar.h"
#include "getprobersaddresses.h"

std::map<wxString, std::pair<wxString, wxString> > get_probers_addresses() {
	std::map<wxString, std::pair<wxString, wxString> > ret;
	char *_servers = libpar_getpar("available_probes_servers", "servers", 0);
	if (_servers == NULL)
		return ret;

	std::stringstream ss(_servers);
	std::string section;
	while (ss >> section) {
		char *_prefix = libpar_getpar(section.c_str(), "prefix", 0);
		char *_address = libpar_getpar(section.c_str(), "address", 0);
		char *_port = libpar_getpar(section.c_str(), "port", 0);
		if (_prefix != NULL && _address != NULL && _port  != NULL) {
			std::wstring address = SC::A2S(_address);
			std::wstring port = SC::A2S(_port);
			ret[SC::A2S(_prefix)] = std::make_pair(address, port); 
		}
		std::free(_prefix);
		std::free(_address);
		std::free(_port);
	}
	std::free(_servers);
	return ret;
}
