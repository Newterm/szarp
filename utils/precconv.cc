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
 * SZARP Praterm
 * 2005 Pratem S.A. 
 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 *
 * $Id$
 * Konwersja warto¶ci zgodnie z precyzj± parametru.
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>
#include <string>

using namespace std;

#include <stdlib.h>
#include <libgen.h>
#include <math.h>

#include "szarp_config.h"
#include "liblog.h"
#include "libpar.h"
#include "conversion.h"

void usage(void)
{
	cerr << "Usage: precconv [OPTION ...] <PARAMETER> <VALUE> [VALUE2]\n";
	cerr << "Converts value to/from raw IPC/base value. Gets precision from IPK config.\n";
	cerr << "Options:\n";
	cerr << "  -t, --to            converts to raw value (default)\n";
	cerr << "  -f, --from          converts from raw value\n";
	cerr << "  -2, --double        converts 2-word value\n";
	cerr << "  -D<name>=<value>    set libpar variable\n";
	cerr << "  -h, --help          print this info and exit\n";
}



int main(int argc, char* argv[])
{
	TSzarpConfig *sc;
	int to = 1, i, dbl = 0;
	long val = 0;
	std::wstring name, value, value2, ipk;

	libpar_read_cmdline(&argc, argv);
	libpar_init();
	

	while (argc > 1) {
		if (!strcmp(argv[1], "-f") || !strcmp(argv[1], "--from")) {
			to = 0;
		} else if (!strcmp(argv[1], "-t") || !strcmp(argv[1], "--to")) {
			to = 1;
		} else if (!strcmp(argv[1], "-2") || !strcmp(argv[1],
				"--double")) {
			dbl = 1;
		} else if (!strcmp(argv[1], "--help") || (!strcmp(argv[1],
			"-h"))) {
			usage();
			return 1;
		} else {
			name = SC::L2S(argv[1]);
			argc--;
			argv++;
			if (argc > 1) {
				value = SC::L2S(argv[1]);
				argc--;
				argv++;
				if (dbl && !to) {
					if (argc <= 1) {
						usage();
						return 1;
					}
					value2 = SC::L2S(argv[1]);
				}
			} else {
				usage();
				return 1;
			}
			goto out;
		}
		argc--;
		argv++;
	}
out:
	if (name.empty() || value.empty() || (dbl && !to && value2.empty())) {
		usage();
		return 1;
	}
	
	xmlInitParser();
	LIBXML_TEST_VERSION
	xmlInitializePredefinedEntities();
	
	sc = new TSzarpConfig();
	
	ipk = SC::L2S(libpar_getpar("", "IPK", 1));
	i = sc->loadXML(ipk);
	
	if (i) {
		wcerr << "Error while reading configuration from file " << ipk << ".\n";
		return 1;
	}

	TParam* p = sc->getParamByName(name);
	if (p == NULL) {
		wcerr << "Param '" << name << "' not found in configuration.\n";
		return 1;
	}

	if (to == 0) {
		if (dbl) {
			wchar_t* eptr;
			val = (unsigned short)wcstol(value.c_str(), &eptr, 10);
			val = val << 16;
			val = val + (unsigned short)wcstol(value2.c_str(), &eptr, 10);
		}
#define BUF_SIZE 128
		wchar_t buffer[BUF_SIZE];
		p->PrintIPCValue(buffer, BUF_SIZE, val, L"NODATA");
		wcout << buffer << "\n";
	} else {
		if (value == L"NODATA") {
			wcout << 0xFFFF << "\n";
			if (dbl) {
				wcout << 0xFFFF << "\n";
			}
			return 0;
		}
		if (!dbl) for (TValue *val = p->GetFirstValue(); val; val = val->GetNext()) {
			if (val->GetString() == value) {
				wcout << val->GetInt() << "\n";
				return 0;
			}
		}
		wchar_t *eptr;
		double d = wcstod(value.c_str(), &eptr);
		for (int i = 0; i < p->GetPrec(); i++) {
			d *= 10;
		}
		if (dbl) {
			int l = (int)rint(d);
			wcout << (short)((l >> 16) & 0xFFFF) << "\n";
			wcout << (short)(l & 0xFFFF) << "\n";
		} else {
			wcout << (int)rint(d) << "\n";
		}
	}

	return 0;
}

