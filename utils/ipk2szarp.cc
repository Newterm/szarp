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
 * 2002 Pawe³ Pa³ucha
 *
 * $Id$
 * Konweter konfiguracji IPK -> SZARP.
 */


#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#include "conversion.h"

#include <iostream>
#include <string>

using namespace std;

#include <stdlib.h>
#include <libgen.h>

#include "szarp_config.h"
#include "liblog.h"

void usage(void)
{
	cout << "Usage: ipk2szarp [OPTION]... [INPUT_FILE [OUTPUT_DIR]]\n";
	cout << "Converts new XML configuration format to SZARP 2.1 config files.\n\n";
	cout << "  INPUT_FILE          XML config file (default is params.xml) \n";
	cout << "Options:\n";
	cout << "  -h, --help          print this info and exit\n";
	cout << "\nYou can also specify '--debug=<n>' option to set debug level from 0 (errors\n  only) to 9." << endl;
}

int main(int argc, char* argv[])
{
	TSzarpConfig *sc;
	int i;
	char *input;

	loginit_cmdline(2, NULL, &argc, argv);
	log_info(0);
	sc = new TSzarpConfig();

	xmlInitParser();
	LIBXML_TEST_VERSION
	xmlInitializePredefinedEntities();

	input = NULL;
	
	while (argc > 1) {
		if (!strcmp(argv[1], "--help") || (!strcmp(argv[1], "-h"))) {
			usage();
			return 1;
		} else {
			input = strdup(argv[1]);
			argc--;
			argv++;
			break;
		}
		argc--;
		argv++;
	}

	if (input == NULL)
		input = strdup("params.xml");
	sz_log(10, "Parsed options summary: , input='%s'", input);
	
	i = sc->loadXML(SC::A2S(input));
	if (i) {
		cout << "Error while reading configuration.\n";
		return 1;
	}

	//check for parameters repetitions in params.xml
	int ret = sc->checkConfiguration();

	delete sc;
	free(input);
	xmlCleanupParser();

	sz_logdone();

	return ret;
}

