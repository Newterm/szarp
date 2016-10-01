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
#include "libpar.h"

#define SZARP_CFG		"/etc/" PACKAGE_NAME "/" PACKAGE_NAME ".cfg"

void usage(void)
{
	cout << "Usage: ipk2szarp [OPTIONS] [INPUT_FILE]\n";
	cout << "Parses params.xml and checks for errors.\n\n";
	cout << "  INPUT_FILE          XML config file (default is params.xml) \n";
	cout << "Options:\n";
	cout << "  -h, --help          print this info and exit\n";
	cout << "\nYou can also specify '--debug=<n>' option to set debug level from 0 (errors\n  only) to 9." << endl;
}

int main(int argc, char* argv[])
{
	char *input;
	char *ipk_path;

	loginit_cmdline(2, NULL, &argc, argv);
	log_info(0);

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

	char* path;
	if( input ) {
		path = input;
	} else {
		libpar_init_with_filename(SZARP_CFG, 1);
		if( !(ipk_path  = libpar_getpar("", "IPK", 1 ))) {
			cerr << "Cannot find IPK variable" << endl;
			return 1;
		}
		path = ipk_path;
	}

	TSzarpConfig *sc;

	/* test using both parsers as not all checks are implemented in loadXMLReader
	   but in parseXml(node) functions that are called by loadXMLDOM */
	sc = new TSzarpConfig();
	const int dom_result = sc->loadXMLDOM(SC::L2S(path));
	delete sc;

	sc = new TSzarpConfig();
	const int reader_result = sc->loadXMLReader(SC::L2S(path));

	int ret = !sc->checkConfiguration();

	delete sc;
	free(input);
	xmlCleanupParser();
	ret = (reader_result || dom_result) || ret;

	sz_logdone();

	return ret;
}

