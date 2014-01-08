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
	cout << "  OUTPUT_DIR          directory where to place output SZARP 2.1 files, \n                      current is default\n";
	cout << "Options:\n";
	cout << "  -f, --force         overwrite existing files, without this option program\n                      fails if files already exist\n\n";
	cout << "  -p, --prefix=prefix prefix used to create Szarp Draw config file (ekrncor)\n                      name, without this option file ekrnXXXX.cor is created.\n";
	cout << "  -o, --old_ekrncor   use traditional format for ekrncor file, draws for \n                      parameters with auto indexes are ignored.\n";
	cout << "  -h, --help          print this info and exit\n";
	cout << "You can also specify '--debug=<n>' option to set debug level from 0 (errors\n  only) to 9.\n";
}

/* Try to guess configuration prefix from output directory path. */
char * guess_prefix(const char *dir)
{
	char *prefix;
	char *tmp = strdup(dir);

	cout << "Trying to guess prefix: ";
	prefix = basename(dirname(tmp));
	free(tmp);

	if (!prefix || !prefix[0] || strchr(prefix, '/')) {
		cout << "using default.\n";
		return NULL;
	} 

	cout << prefix << "\n";
	return strdup (prefix);
}

int main(int argc, char* argv[])
{
	TSzarpConfig *sc;
	int i, force, old_ekrncor;
	char *prefix, *input, *dir;

	loginit_cmdline(2, NULL, &argc, argv);
	log_info(0);
	sc = new TSzarpConfig();

	xmlInitParser();
	LIBXML_TEST_VERSION
	xmlInitializePredefinedEntities();

	force = 0;
	old_ekrncor = 0;
	prefix = input = dir = NULL;
	
	while (argc > 1) {
		if (!strcmp(argv[1], "-f") || (!strcmp(argv[1], "--force")))
			force = 1;
		else if (!strcmp(argv[1], "-o") || (!strcmp(argv[1], "--old_ekrncor")))
			old_ekrncor = 1;
		else if (!strcmp(argv[1], "-p")) {
			argc--;
			argv++;
			if (argc <= 1) {
				usage();
				return 1;
			}
			prefix = strdup(argv[1]);
		} else if (!strncmp(argv[1], "--prefix=", 9)) {
			prefix = strdup(argv[1]+9);
		} else if (!strcmp(argv[1], "--help") || (!strcmp(argv[1],
			"-h"))) {
			usage();
			return 1;
		} else {
			input = strdup(argv[1]);
			argc--;
			argv++;
			if (argc > 1)
				dir = strdup(argv[1]);
			goto out;
		}
		argc--;
		argv++;
	}
out:
	if (input == NULL)
		input = strdup("params.xml");
	if (dir == NULL)
		dir = get_current_dir_name();
	sz_log(10, "Parsed options summary: prefix='%s' force='%s', input='%s', output='%s'",
		prefix, force?"yes":"no", input, dir);
	
	i = sc->loadXML(SC::A2S(input));
	if (i) {
		cout << "Error while reading configuration.\n";
		return 1;
	}

	if (prefix == NULL) {
		prefix = guess_prefix(dir);
	}

	//check for parameters repetitions in params.xml
	sc->checkRepetitions();
	
	delete sc;
	free(input);
	free(dir);
	if (prefix)
		free(prefix);
	xmlCleanupParser();
	return 0;
}
