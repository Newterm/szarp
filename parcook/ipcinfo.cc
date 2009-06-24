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

int
main(int argc, char *argv[])
{
    char parcookpat[129];
    char linedmnpat[129];
    FILE *patfile;
    ushort val, i;
    char buf[133];
    
    libpar_read_cmdline(&argc, argv);
    libpar_init();
    libpar_readpar("", "parcook_cfg", buf, sizeof(buf), 1);
    libpar_readpar("", "parcook_path", parcookpat, sizeof(parcookpat), 1);
    libpar_readpar("", "linex_cfg", linedmnpat, sizeof(linedmnpat), 1);
    libpar_done();  
    
    
    if ((patfile = fopen(buf, "r")) == NULL)
	ErrMessage(2, buf);
    fscanf(patfile, "%hu", &val);
    
    printf("\nSHM_PROBE: \t %08x\n", ftok(parcookpat,SHM_PROBE));
    printf("SHM_MINUTE: \t %08x\n", ftok(parcookpat,SHM_MINUTE));
    printf("SHM_MIN10: \t %08x\n", ftok(parcookpat,SHM_MIN10));
    printf("SHM_HOUR: \t %08x\n", ftok(parcookpat,SHM_HOUR));
    printf("SHM_ALERT: \t %08x\n", ftok(parcookpat,SHM_ALERT));
    
    for (i = 0; i < val; i++)
	printf("SHM_LIN%d: \t %08x\n", (i + 1), ftok(linedmnpat, i+2));
    
    printf("\nSEM_PARCOOK: \t %08x\n", ftok(parcookpat,SEM_PARCOOK));
    
    printf("\nMSG_SET: \t %08x\n", ftok(parcookpat, MSG_SET));
    printf("MSG_RPLY: \t %08x\n", ftok(parcookpat, MSG_RPLY));
    
    return 0;
}

