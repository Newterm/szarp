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
 * biblioteka MyInc
 * ipctools.c
 */


/* ipctools.c: Zmienne i procedury dostepu do dzielonych segmentow pamieci */

/*
 * Modified by Codematic 25.02.1998
 *
 * Dodana obsluga libpar - no more .pat files !
 * ipcInitialize() - teraz bezparametrowa
 * pluskwy w PTTGetLocal - 19.04.98
 */

#include <config.h>
#ifndef MINGW32

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define _HELP_H_
#define _ICON_ICON_

#include "szarp.h"
//#include "definable.h"
#include "ipctools.h"
#include "liblog.h"

static int SemDes, ProbeDes, MinuteDes, Min10Des, HourDes;
#if 0
static int DayDes, PTTDes;
#endif
static int AlertDes;

short *Probe;

short *Minute;

short *Min10;

short *Hour;

// short *Day;

unsigned char *Alert;

ushort VTlen, ExtraPars;

pPTTInfo PTT;

static char parcookpat[129];

int silent_err_function = 1;

static int local_errmessage(int no, const char* string)
{
    if( silent_err_function )
	return -1;
    
    ErrMessage(no, string);
    
    return 0;
}


int ipcInitialize()
{
	struct shmid_ds ipc_buf;

	char *c;

	c = libpar_getpar("", "parcook_path", 0);
	if (!c) { local_errmessage(7, "parcook_path"); return -1; }
	strncpy(parcookpat, c, sizeof(parcookpat));
	
	VTlen = 0;

	
    if ((ProbeDes = shmget(ftok(parcookpat, SHM_PROBE), 0, 00666)) == -1)
	 { local_errmessage(3, "parcook0"); return -1; }
    if ((MinuteDes = shmget(ftok(parcookpat, SHM_MINUTE), 0, 00666)) == -1)
	 { local_errmessage(3, "parcook1"); return -1; }
    if ((Min10Des = shmget(ftok(parcookpat, SHM_MIN10), 0, 00666)) == -1)
	 { local_errmessage(3, "parcook2"); return -1; }
    if ((HourDes = shmget(ftok(parcookpat, SHM_HOUR), 0, 00666)) == -1)
	 { local_errmessage(3, "parcook3"); return -1; }
    if ((AlertDes = shmget(ftok(parcookpat, SHM_ALERT), 0, 00666)) == -1)
	 { local_errmessage(3, "parcook6"); return -1; }
    if ((SemDes = semget(ftok(parcookpat, SEM_PARCOOK), SEM_LINE, 00666)) == -1)
	{ local_errmessage(5, "parcook0"); return -1; }

   /* pobierz rozmiar segmentu */
    if (shmctl(ProbeDes, IPC_STAT, &ipc_buf) == -1)
        { local_errmessage(3, "parcook0"); return -1; }
   VTlen = ipc_buf.shm_segsz / sizeof(short);
   sz_log(10, "ipcInitialize: ipc_buf.shm_segsz = %zd, VTlen = %d", 
   	 ipc_buf.shm_segsz, VTlen);
   
   return 0;
}

int ipcInitializewithSleep(int maxDelay)
{
    int result;
    int i;
    for (i = 0; ((result = ipcInitialize()) == -1 ) && i < maxDelay;++i)
	sleep(1);
    return result;
}

#if 0
int ipcPTTGetLocal(char *programname)
{
    char buf[1025];
    char *chptr, *c;
    FILE *PTTfile;
    ushort PTTrev, PTTblen, PTTlen, val, i;

    printf("*** ipcPTTGetLocal called ***\n");

    libpar_getkey(programname, "PTT", &c);
    if (!c) { local_errmessage(7, "PTT"); return -1; }
    strncpy(buf, c, sizeof(buf));
    
    if ((PTTfile = fopen(buf, "r")) == NULL) /* PTT.act */
	{ local_errmessage(2, buf); return -1; }
    fscanf(PTTfile, "%hu %hu %hu\n", &PTTrev, &PTTblen, &PTTlen); 
    if ((PTT = (pPTTInfo) calloc(PTTlen+1, sizeof(tPTTEntry))) == NULL)
	{ local_errmessage(1, "PTT"); return -1; }
		/* Codematic - trzeba jeszcze zaalokowac naglowek !!! */
    printf("PTT in local mode, reading %d definitions from file\n",
	   PTTlen);

    for (i = 0; i < PTTlen; i++) {
	fscanf(PTTfile, "%hu %hu %s ", &(PTT->tab[i].addr), &val,
	       PTT->tab[i].sym);
	PTT->tab[i].dot = (unsigned char) val;
	fgets(buf, 1024, PTTfile);
	if ((chptr = strrchr(buf, '#')) != NULL)
	    *chptr = 0;
	else if ((chptr = strrchr(buf, '\n')) != NULL)
	    *chptr = 0;
	if ((chptr = strrchr(buf, ';')) != NULL) {
	    strncpy(PTT->tab[i].alt, chptr + 1, sizeof(PTT->tab[i].alt));  
	    *chptr = 0; /* Codematic */
	} else
	    PTT->tab[i].alt[0] = 0;
	strncpy(PTT->tab[i].full, buf, sizeof(PTT->tab[i].full)); 
	printf("%s.%d[%d] %s; %s\n", PTT->tab[i].sym, PTT->tab[i].dot,
	       PTT->tab[i].addr, PTT->tab[i].full, PTT->tab[i].alt);
    }
    PTT->rev = PTTrev;
    PTT->blen = PTTblen;
    PTT->len = PTTlen;
    PTT->dlen = 0;

    fclose(PTTfile);

    /* parametry definiowalne - struktura PTT */
    InitDefinableStuff(programname);
    PTT = MergePTT(PTT);
    
    return 0;
}
#endif

#if 0
int ipcPTTGet(char *programname)
{
    char buf[1025];
    char *chptr, *c;
    FILE *PTTfile;
    ushort PTTrev, PTTblen, PTTlen, val, i;

    // debug
    //printf("ipcPTTGet called\n");

    libpar_getkey(programname, "PTT", &c);
    if (!c) { local_errmessage(7, "PTT"); return -1; }
    strncpy(buf, c, sizeof(buf));
    
    // debug
    //printf("ipcPTTGet: opening file '%s'...\n", buf);

    if ((PTTfile = fopen(buf, "r")) == NULL) /* PTT.act */
	{ local_errmessage(2, buf); return -1; }

    // debug
    //printf("ipcPTTGet: file opened\n", buf);
    
    fscanf(PTTfile, "%hu %hu %hu\n", &PTTrev, &PTTblen, &PTTlen);

    if ((PTTDes = shmget(ftok(parcookpat, SHM_PTT), PTTlen * sizeof(tPTTEntry),
			 00666)) < 0) {
	if ((PTTDes = shmget(ftok(parcookpat, SHM_PTT), 0, 00666)) < 0)
	    printf("PTT not defined in shm, reading %d definitions from file\n",
		   PTTlen);
	else {
	    printf("removing old PTT from shm, reading %d definitions from file\n",
		   PTTlen);
	    shmctl(PTTDes, IPC_RMID, NULL);
	}

    	// debug
    	//printf("ipcPTTGet: taking shmget..\n");
	if ((PTTDes = shmget(ftok(parcookpat, SHM_PTT), PTTlen * sizeof(tPTTEntry),
			     IPC_CREAT | 00666)) < 0)
	    { local_errmessage(3, "PTT definitions"); return -1; }
    	//printf("ipcPTTGet: shmget taken..\n");
    }

    // debug
    //printf("ipcPTTGet: allocating memory for PTT...\n");
    
    if ((PTT = (pPTTInfo) shmat(PTTDes, (void *) 0, 0)) == (void *) -1)
	{ local_errmessage(4, "PTT"); return -1; }

    // debug
    //printf("ipcPTTGet: allocated\n");

    if (PTT->rev != PTTrev || PTT->blen != PTTblen || PTT->len != PTTlen) {
	for (i = 0; i < PTTlen; i++) {
	    fscanf(PTTfile, "%hu %hu %s ", &(PTT->tab[i].addr), &val,
		   PTT->tab[i].sym);
	    PTT->tab[i].dot = (unsigned char) val;
	    fgets(buf, 1024, PTTfile);
	    if ((chptr = strrchr(buf, '#')) != NULL)
		*chptr = 0;
	    else if ((chptr = strrchr(buf, '\n')) != NULL)
		*chptr = 0;
	    if ((chptr = strrchr(buf, ';')) != NULL) {
		strcpy(PTT->tab[i].alt, chptr + 1);
		*chptr = 0;
	    } else
		PTT->tab[i].alt[0] = 0;
	    strcpy(PTT->tab[i].full, buf);
	    printf("%s.%d[%d] %s; %s\n", PTT->tab[i].sym, PTT->tab[i].dot,
		   PTT->tab[i].addr, PTT->tab[i].full, PTT->tab[i].alt);
	}
	PTT->rev = PTTrev;
	PTT->blen = PTTblen;
	PTT->len = PTTlen;
    }
    fclose(PTTfile);

   InitDefinableStuff(programname);

   PTT = MergePTT(PTT);   

return 0;
}
#endif

int ipcRdGetInto(unsigned char shmdes)
{
    struct sembuf Sem[4];
    switch (shmdes) {
    case SHM_PROBE:
	if ((Probe = (short *) shmat(ProbeDes, (void *) 0, 0)) == (void *) -1)
	    { local_errmessage(4, "parcook0"); return -1; }
	Sem[0].sem_flg = Sem[1].sem_flg = SEM_UNDO;
	Sem[0].sem_num = SEM_PROBE + 1;
	Sem[0].sem_op = 0;
	Sem[1].sem_num = SEM_PROBE;
	Sem[1].sem_op = 1;
	semop(SemDes, Sem, 2);
	break;
    case SHM_MINUTE:
	if ((Minute = (short *) shmat(MinuteDes, (void *) 0, 0)) == (void *) -1)
	    { local_errmessage(4, "parcook1"); return -1; }
	Sem[0].sem_flg = Sem[1].sem_flg = SEM_UNDO;
	Sem[0].sem_num = SEM_MINUTE + 1;
	Sem[0].sem_op = 0;
	Sem[1].sem_num = SEM_MINUTE;
	Sem[1].sem_op = 1;
	semop(SemDes, Sem, 2);
	break;
    case SHM_MIN10:
	if ((Min10 = (short *) shmat(Min10Des, (void *) 0, 0)) == (void *) -1)
	    { local_errmessage(4, "parcook2"); return -1; } 
	Sem[0].sem_flg = Sem[1].sem_flg = SEM_UNDO;
	Sem[0].sem_num = SEM_MIN10 + 1;
	Sem[0].sem_op = 0;
	Sem[1].sem_num = SEM_MIN10;
	Sem[1].sem_op = 1;
	semop(SemDes, Sem, 2);
	break;
    case SHM_HOUR:
	if ((Hour = (short *) shmat(HourDes, (void *) 0, 0)) == (void *) -1)
	    { local_errmessage(4, "parcook3"); return -1; }
	Sem[0].sem_flg = Sem[1].sem_flg = SEM_UNDO;
	Sem[0].sem_num = SEM_HOUR + 1;
	Sem[0].sem_op = 0;
	Sem[1].sem_num = SEM_HOUR;
	Sem[1].sem_op = 1;
	semop(SemDes, Sem, 2);
	break;
#if 0
    case SHM_DAY:
	if ((Day = (short *) shmat(DayDes, (void *) 0, 0)) == (void *) -1)
	    { local_errmessage(4, "parcook4"); return -1; }
	Sem[0].sem_flg = Sem[1].sem_flg = SEM_UNDO;
	Sem[0].sem_num = SEM_DAY + 1;
	Sem[0].sem_op = 0;
	Sem[1].sem_num = SEM_DAY;
	Sem[1].sem_op = 1;
	semop(SemDes, Sem, 2);
	break;
#endif
    case SHM_ALERT:
	if ((Alert = (unsigned char *) shmat(AlertDes, (void *) 0, 0)) == (void *) -1)
	    { local_errmessage(4, "parcook5"); return -1; }
	Sem[0].sem_flg = Sem[1].sem_flg = SEM_UNDO;
	Sem[0].sem_num = SEM_ALERT + 1;
	Sem[0].sem_op = 0;
	Sem[1].sem_num = SEM_ALERT;
	Sem[1].sem_op = 1;
	semop(SemDes, Sem, 2);
	break;
    default:
	break;
    }
return 0;
}

int ipcRdGetOut(unsigned char shmdes)
{
    struct sembuf Sem[4];
    switch (shmdes) {
    case SHM_PROBE:
	Sem[0].sem_flg = SEM_UNDO;
	Sem[0].sem_num = SEM_PROBE;
	Sem[0].sem_op = -1;
	semop(SemDes, Sem, 1);
	shmdt((void *) Probe);
	break;
    case SHM_MINUTE:
	Sem[0].sem_flg = SEM_UNDO;
	Sem[0].sem_num = SEM_MINUTE;
	Sem[0].sem_op = -1;
	semop(SemDes, Sem, 1);
	shmdt((void *) Minute);
	break;
    case SHM_MIN10:
	Sem[0].sem_flg = SEM_UNDO;
	Sem[0].sem_num = SEM_MIN10;
	Sem[0].sem_op = -1;
	semop(SemDes, Sem, 1);
	shmdt((void *) Min10);
	break;
    case SHM_HOUR:
	Sem[0].sem_flg = SEM_UNDO;
	Sem[0].sem_num = SEM_HOUR;
	Sem[0].sem_op = -1;
	semop(SemDes, Sem, 1);
	shmdt((void *) Hour);
	break;
#if 0
    case SHM_DAY:
	Sem[0].sem_flg = SEM_UNDO;
	Sem[0].sem_num = SEM_DAY;
	Sem[0].sem_op = -1;
	semop(SemDes, Sem, 1);
	shmdt((void *) Day);
	break;
#endif
    case SHM_ALERT:
	Sem[0].sem_flg = SEM_UNDO;
	Sem[0].sem_num = SEM_ALERT;
	Sem[0].sem_op = -1;
	semop(SemDes, Sem, 1);
	shmdt((void *) Alert);
	break;
    default:
	break;
    }
return 0;
}

int ipcWrGetInto(unsigned char shmdes)
{
    struct sembuf Sem[4];
    switch (shmdes) {
    case SHM_PROBE:
	if ((Probe = (short *) shmat(ProbeDes, (void *) 0, 0)) == (void *) -1)
	    { local_errmessage(4, "parcook0"); return -1; }
	Sem[0].sem_flg = Sem[1].sem_flg = SEM_UNDO;
	Sem[0].sem_num = SEM_PROBE;
	Sem[0].sem_op = 0;
	Sem[1].sem_num = SEM_PROBE + 1;
	Sem[1].sem_op = 1;
	semop(SemDes, Sem, 2);
	break;
    case SHM_MINUTE:
	if ((Minute = (short *) shmat(MinuteDes, (void *) 0, 0)) == (void *) -1)
	    { local_errmessage(4, "parcook1"); return -1; }
	Sem[0].sem_flg = Sem[1].sem_flg = SEM_UNDO;
	Sem[0].sem_num = SEM_MINUTE;
	Sem[0].sem_op = 0;
	Sem[1].sem_num = SEM_MINUTE + 1;
	Sem[1].sem_op = 1;
	semop(SemDes, Sem, 2);
	break;
    case SHM_MIN10:
	if ((Min10 = (short *) shmat(Min10Des, (void *) 0, 0)) == (void *) -1)
	    { local_errmessage(4, "parcook2"); return -1; }
	Sem[0].sem_flg = Sem[1].sem_flg = SEM_UNDO;
	Sem[0].sem_num = SEM_MIN10;
	Sem[0].sem_op = 0;
	Sem[1].sem_num = SEM_MIN10 + 1;
	Sem[1].sem_op = 1;
	semop(SemDes, Sem, 2);
	break;
    case SHM_HOUR:
	if ((Hour = (short *) shmat(HourDes, (void *) 0, 0)) == (void *) -1)
	    { local_errmessage(4, "parcook3"); return -1; }
	Sem[0].sem_flg = Sem[1].sem_flg = SEM_UNDO;
	Sem[0].sem_num = SEM_HOUR;
	Sem[0].sem_op = 0;
	Sem[1].sem_num = SEM_HOUR + 1;
	Sem[1].sem_op = 1;
	semop(SemDes, Sem, 2);
	break;
#if 0
    case SHM_DAY:
	if ((Day = (short *) shmat(DayDes, (void *) 0, 0)) == (void *) -1)
	    { local_errmessage(4, "parcook4"); return -1; }
	Sem[0].sem_flg = Sem[1].sem_flg = SEM_UNDO;
	Sem[0].sem_num = SEM_DAY;
	Sem[0].sem_op = 0;
	Sem[1].sem_num = SEM_DAY + 1;
	Sem[1].sem_op = 1;
	semop(SemDes, Sem, 2);
	break;
#endif
    case SHM_ALERT:
	if ((Alert = (unsigned char *) shmat(AlertDes, (void *) 0, 0)) == (void *) -1)
	    { local_errmessage(4, "parcook5"); return -1; }
	Sem[0].sem_flg = Sem[1].sem_flg = SEM_UNDO;
	Sem[0].sem_num = SEM_ALERT;
	Sem[0].sem_op = 0;
	Sem[1].sem_num = SEM_ALERT + 1;
	Sem[1].sem_op = 1;
	semop(SemDes, Sem, 2);
	break;
    default:
	break;
    }

return 0;
}

int ipcWrGetOut(unsigned char shmdes)
{
    struct sembuf Sem[4];
    switch (shmdes) {
    case SHM_PROBE:
	Sem[0].sem_flg = SEM_UNDO;
	Sem[0].sem_num = SEM_PROBE + 1;
	Sem[0].sem_op = -1;
	semop(SemDes, Sem, 1);
	shmdt((void *) Probe);
	break;
    case SHM_MINUTE:
	Sem[0].sem_flg = SEM_UNDO;
	Sem[0].sem_num = SEM_MINUTE + 1;
	Sem[0].sem_op = -1;
	semop(SemDes, Sem, 1);
	shmdt((void *) Minute);
	break;
    case SHM_MIN10:
	Sem[0].sem_flg = SEM_UNDO;
	Sem[0].sem_num = SEM_MIN10 + 1;
	Sem[0].sem_op = -1;
	semop(SemDes, Sem, 1);
	shmdt((void *) Min10);
	break;
    case SHM_HOUR:
	Sem[0].sem_flg = SEM_UNDO;
	Sem[0].sem_num = SEM_HOUR + 1;
	Sem[0].sem_op = -1;
	semop(SemDes, Sem, 1);
	shmdt((void *) Hour);
	break;
#if 0
    case SHM_DAY:
	Sem[0].sem_flg = SEM_UNDO;
	Sem[0].sem_num = SEM_DAY + 1;
	Sem[0].sem_op = -1;
	semop(SemDes, Sem, 1);
	shmdt((void *) Day);
	break;
#endif
    case SHM_ALERT:
	Sem[0].sem_flg = SEM_UNDO;
	Sem[0].sem_num = SEM_ALERT + 1;
	Sem[0].sem_op = -1;
	semop(SemDes, Sem, 1);
	shmdt((void *) Alert);
	break;
    default:
	break;
    }

return 0;
}

#endif /* MINGW32 */
