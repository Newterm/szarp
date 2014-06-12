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
 * $Id$

 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 *
 * Reads value(s) from szarp base.
 *
 * Usage:
 * szbreader [ -Dname=value ... ] <parameter name> <EPOCH>
 * szbreader [ -Dname=value ... ] <parameter name> <EPOCH START> <EPOCH END>
 *
 * First type of call reads parameter from base at given time (time as seconds
 * since EPOCH), seconds reads average of values between EPOCH START
 * (including) and EPOCH END (excluding). 
 * Single value is printed on stdout, NO_DATA if no data is found.
 * Return code is 0 on normal exit, 1 on error. No data is printed on stdout
 * on error.
 */
 

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "liblog.h"
#include "libpar.h"
#include "szarp_config.h"
#include "szbase/szbbase.h"

#include <iostream>
#include "conversion.h"

using namespace std;

#define SZARP_CFG		"/etc/szarp/szarp.cfg"

void Usage()
{
	cerr << "\n\
Usage:\n\
  szbreader [ -Dname=value ... ] <parameter name> <EPOCH>\n\
  szbreader [ -Dname=value ... ] <parameter name> <EPOCH START> <EPOCH END>\n\
\n\
First type of call reads parameter from base at given time (time as seconds\n\
since EPOCH), seconds reads average of values between EPOCH START\n\
(including) and EPOCH END (excluding). \n\
Single value is printed on stdout, NODATA if no data is found.\n\
Return code is 0 on normal exit, 1 on error. No data is printed on stdout\n\
on error.\n";
}

int main(int argc, char** argv)
{
	loginit_cmdline(2, NULL, &argc, argv);
	log_info(0);
	libpar_read_cmdline(&argc, argv);
	libpar_init();

	if ((argc != 3) && (argc != 4)) {
		Usage();
		return 1;
	}
	
	char *path = libpar_getpar("", "IPK", 0);
	if (path == NULL) {
		sz_log(0, "'IPK' not found in szarp.cfg");
		return 1;
	}
	char *dir = libpar_getpar("", "datadir", 0);
	if (dir == NULL) {
		sz_log(0, "'datadir' not found in szarp.cfg");
		return 1;
	}

	TSzarpConfig ipk;
	if (ipk.loadXML(SC::L2S(path))) {
		sz_log(0, "Error loading IPK from file '%s'", path);
		return 1;
	}
	free(path);

	TParam *p = ipk.getParamByName(SC::L2S(argv[1]));
	if (p == NULL) {
		sz_log(0, "Parameter '%s' not found in IPK", argv[1]);
		return 1;
	}

	char *tmp;
	time_t start = strtol(argv[2], &tmp, 10);
	if ((argv[2][0] == 0) || (tmp[0] != 0)) {
		sz_log(0, "'%s' is not a correct number", argv[2]);
		return 1;
	}
	time_t end;
	if (argc < 4) {
		end = 0;
	} else {
		end = strtol(argv[3], &tmp, 10);
		if ((argv[3][0] == 0) || (tmp[0] != 0)) {
			sz_log(0, "'%s' is not a correct number", argv[2]);
			return 1;
		}
	}

	IPKContainer::Init(SC::L2S(PREFIX), SC::L2S(PREFIX), L"", new NullMutex());
	Szbase::Init(SC::L2S(PREFIX), NULL);
	szb_buffer_t* buf = szb_create_buffer(Szbase::GetObject(), SC::L2S(dir), 1, &ipk);
	if (buf == NULL) {
		sz_log(0, "Error creating szbase buffer");
		return 1;
	}
	free(dir);

	SZBASE_TYPE data;
	if (end == 0) {
		data = szb_get_data(buf, p, start);
	} else {
		data = szb_get_avg(buf, p, start, end);
	}
	wchar_t b[20];
	p->PrintValue(b, 19, data, L"NODATA");
	wcout << b;

	szb_free_buffer(buf);
	
	return 0;
}


