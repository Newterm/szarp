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
/* $Id$ */

/*
 * SZARP 2.0
 * parcook
 * ipcinfo.c
 */

#define _HELP_H_
#define _ICON_ICON_

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#include "szarp.h"
#include "ipcdefines.h"
#include "szarp_config.h"
#include "conversion.h"


int main(int argc, char *argv[])
{

	libpar_read_cmdline(&argc, argv);
	/* read params from szarp.cfg file */
	libpar_init_with_filename("/etc/" PACKAGE_NAME "/" PACKAGE_NAME ".cfg", 1);

	char *parcookpat = libpar_getpar("", "parcook_path", 1);
	char *linedmnpat = libpar_getpar("", "linex_cfg", 1);
	char *config_prefix = libpar_getpar("sender", "config_prefix", 1);
	libpar_done();

	IPKContainer::Init(SC::L2S(PREFIX), SC::L2S(PREFIX), L"pl");
	IPKContainer *ic = IPKContainer::GetObject();
	TSzarpConfig *ipk = ic->GetConfig(SC::L2S(config_prefix));
	if (ipk == NULL) {
		printf("Unable to load IPK for prefix %s", config_prefix);
		return 1;
	}

	printf("\nSHM_PROBE: \t %08x\n", ftok(parcookpat,SHM_PROBE));
	printf("SHM_MINUTE: \t %08x\n", ftok(parcookpat,SHM_MINUTE));
	printf("SHM_MIN10: \t %08x\n", ftok(parcookpat,SHM_MIN10));
	printf("SHM_HOUR: \t %08x\n", ftok(parcookpat,SHM_HOUR));
	printf("SHM_ALERT: \t %08x\n", ftok(parcookpat,SHM_ALERT));

	int linenum = 0;
	for (TDevice *d = ipk->GetFirstDevice(); d != NULL; d = d->GetNext()) {
		 ++linenum;
		 printf("SHM_LIN%d: \t %08x\n", (linenum), ftok(linedmnpat, linenum));
	}


	printf("\nSEM_PARCOOK: \t %08x\n", ftok(parcookpat,SEM_PARCOOK));

	printf("\nMSG_SET: \t %08x\n", ftok(parcookpat, MSG_SET));
	printf("MSG_RPLY: \t %08x\n", ftok(parcookpat, MSG_RPLY));

	free(config_prefix);
	free(linedmnpat);
	free(parcookpat);

	return 0;
}

