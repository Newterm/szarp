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
 * SZARP - Praterm S.A.
 * 2006 Krzysztof O³owski
 *
 * $Id$
 * Checks if repetitions of parameters names in params.xml occurred
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>

using namespace std;

#include "conversion.h"
#include "szarp_config.h"
#include "liblog.h"

void usage(void)
{
	cout << "\nUsage: ipkrep [OPTION] [INPUT_FILE]\n";
	cout << "  INPUT_FILE          XML config file (default is params.xml) \n";

	cout << "Options:\n";
	cout << "  -q, --quiet         do not print anything to standard output\n";
	cout << "  -h, --help          print this info and exit\n\n";

}

int main(int argc, char* argv[])
{
	TSzarpConfig *sc;
	char *input;
	int i,a;
	
	loginit_cmdline(2, NULL, &argc, argv);
	log_info(0);
	sc = new TSzarpConfig();

	xmlInitParser();
	LIBXML_TEST_VERSION
	xmlInitializePredefinedEntities();

	if (argc < 2) {
		//input = "-";
		sz_log(1,"No params.xml specified\n");
		return 0;
	} else {
		input = argv[1];
	}

	if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) { 
		usage();
		return 0;
	} else {
		if (!strcmp(argv[1], "-q") || !strcmp(argv[1], "-quiet") ) {
			input = argv[2];
		
			i = sc->loadXML(SC::A2S(input));
			if (i) {
				cout << "Error while reading configuration.\n";
				return 1;
			}
			a = sc->checkRepetitions(1);

		} else {
			i = sc->loadXML(SC::A2S(input));
			if (i) {
				cout << "Error while reading configuration.\n";
				return 1;
			}

			a = sc->checkRepetitions();
		}
	}
	
	
	delete sc;
	free(input);
	logdone();
	xmlCleanupParser();
	return a;
}
