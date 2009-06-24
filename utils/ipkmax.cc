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
#include <iostream>
using namespace std;

#include "conversion.h"
#include "szarp_config.h"
#include "liblog.h"

void usage(void)
{
	cout << "Returns maximum baseInd attribute from file.\n";
	cout << "Usage: ipkmax {--help|-h}                           print info end exit\n";
	cout << "       ipkmax [input_file]                          printf max baseInd\n\n";
	cout << "  input_file 	       XML config file (default is params.xml) \n";
	cout << "  You can also specify '--debug=<n>' option to set debug level from 0 (errors\n  only) to 9.\n";
}

int main(int argc, char* argv[])
{
	TSzarpConfig *sc;
	int i;

	loginit_cmdline(2, NULL, &argc, argv);
	sc = new TSzarpConfig();
	xmlInitParser();
	xmlInitializePredefinedEntities();
	if (argc > 1) {
		if ((!strcmp(argv[1], "--help")) || (!strcmp(argv[1], "-h"))) {
			usage();
			return 0;
		}
		i = sc->loadXML(SC::A2S(argv[1]));
	} else
		i = sc->loadXML(L"params.xml");
	if (i) {
		cout << "Error while reading configuration.\n";
		return 1;
	}
	cout << sc->GetMaxBaseInd() << "\n";
	delete sc;
	return 0;
}
