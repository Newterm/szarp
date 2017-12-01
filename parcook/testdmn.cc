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
 @description_start
 @class 2
 @devices Daemon reads raw IPC values from text file, one parameter per line.
 @devices.pl Demon czyta nieprzeliczone warto¶ci parametrów z podanego pliku tekstowego, jeden parametr
 w linii.
 @description_end
*/


#define _IPCTOOLS_H_		/* Nie dolaczaj ipctools.h z szarp.h */
#define _HELP_H_
#define _ICON_ICON_

#include<sys/types.h>
#include<sys/stat.h>
#include<sys/uio.h>
#include<sys/time.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<sys/sem.h>
#include<sys/msg.h>
#include<fcntl.h>
#include<termio.h>
#include<unistd.h>
#include<signal.h>
#include<string.h>
#include<errno.h>
#include<stdio.h>
#include<stdlib.h>

#include "config.h"
#include "szarp.h"
#include "liblog.h"
#include "ipcdefines.h"

#define MAXLINES 48

#define MAX_PARS 100

static int LineNum;

static int SemDes;

static int ShmDes;

static unsigned char UnitsOnLine;

static unsigned char RadiosOnLine;

static int DirectLine;

static int VTlen;

static short *ValTab;

static struct sembuf Sem[2];

static char parcookpat[81];

static char linedmnpat[81];

char *linedev;			/* path to file with data */

struct phUnit {
	unsigned char UnitCode;	/* kod sterownika */
	unsigned char RapId;	/* kod raportu */
	unsigned char SubId;	/* subkod raportu */
	unsigned char ParIn;	/* liczba zbieranych parametrow */
	unsigned char ParOut;	/* liczba ustawianych parametrów */
	unsigned short ParBase;	/* adres bazowy parametrow w tablicy linii */
	short *Pars;		/* tablica parametrów do wys³ania */
	unsigned char *Sending;	/* znaczniki potwierdzeñ wys³ania parametru */
	long *rtype;		/* tablica ident komunikatów potwierdzeñ */
	int *Vals;		/* odczytane warto¶ci */
};

typedef struct phUnit tUnit;

struct phRadio {
	char RadioName[41];
	unsigned char NumberOfUnits;
	tUnit *UnitsInfo;
};

typedef struct phRadio tRadio;

tUnit *UnitsInfo;

tRadio *RadioInfo;

void Initialize(unsigned char linenum)
{
	char ch;
	char *chptr;
	unsigned char i, ii, j, maxj;
	int val, val1, val2, val3, val4;
	char linedefname[81];
	FILE *linedef;

	libpar_init();
	libpar_readpar("", "parcook_path", parcookpat, sizeof(parcookpat), 1);
	libpar_readpar("", "linex_cfg", linedmnpat, sizeof(linedmnpat), 1);
	libpar_done();
	sz_log(10, "testdmn: linex_cfg: %s", linedmnpat);

	VTlen = 0;
	sprintf(linedefname, "%s/line%d.cfg", linedmnpat, linenum);

	if ((linedef = fopen(linedefname, "r")) == NULL) {
		sz_log(2, "testdmn: cannot open file '%s'", linedefname);
		exit(1);
	}
	if (fscanf(linedef, "%d\n", &val) > 0) {
		RadiosOnLine = 0;
		if (val > 0) {
			UnitsOnLine = val;
			DirectLine = 0;
		}
		else {
			UnitsOnLine = 1;
			DirectLine = 1;
		}
	}
	else {
		int ret = fscanf(linedef, "%c%d\n", &ch, &val);
		if ( ret != 2 ) {
			sz_log(1, "testdmn: fscanf error, could not read given values");
			exit(1);
		}
		if (ch != 'R')
			exit(-1);
		else
			RadiosOnLine = (unsigned char) val;
	}
	sz_log(2, "testdmn: deamon for line %d\n", linenum);
	if (RadiosOnLine)
		sz_log(2, "testdmn: allocating structures for %d radios\n",
		    RadiosOnLine);
	else
		sz_log(2, "testdmn: allocating structures for %d units\n",
		    UnitsOnLine);
	maxj = RadiosOnLine ? RadiosOnLine : 1;
	if ((RadioInfo = (tRadio *) calloc(maxj, sizeof(tRadio))) == NULL) {
		sz_log(1, "testdmn: memory allocating error (1)");
		exit(1);
	}
	for (j = 0; j < maxj; j++) {
		if (RadiosOnLine) {
			if (fgets(RadioInfo[j].RadioName, 40, linedef) == NULL) {
				sz_log(1, "testdmn: fgets error, could not copy given linedef");
				exit(1);
			}

			if ((chptr =
			     strrchr(RadioInfo[j].RadioName, '\n')) != NULL)
				*chptr = 0;
			int ret = fscanf(linedef, "%d\n", &val);
			if ( ret != 1 ) {
				sz_log(1, "testdmn: fscanf error, could not read given values");
				exit(1);
			}
			RadioInfo[j].NumberOfUnits = UnitsOnLine =
				(unsigned char) val;
			sz_log(2, "testdmn: radio <%s> drives %d units\n",
			    RadioInfo[j].RadioName, UnitsOnLine);
		}
		if ((UnitsInfo =
		     (tUnit *) calloc(UnitsOnLine, sizeof(tUnit))) == NULL) {
			sz_log(1, "testdmn: memory allocating error (2)");
			exit(1);
		}
		for (i = 0; i < UnitsOnLine; i++) {
			int ret = fscanf(linedef, "%c %u %u %u %u %u\n",
			       &UnitsInfo[i].UnitCode, &val, &val1, &val2,
			       &val3, &val4);
			if (ret != 6) {
				sz_log(1, "testdmn: fscanf error, could not read given values");
				exit(1);
			}
			UnitsInfo[i].RapId = (unsigned char) val;
			UnitsInfo[i].SubId = (unsigned char) val1;
			UnitsInfo[i].ParIn = (unsigned char) val2;
			UnitsInfo[i].ParOut = (unsigned char) val3;
			UnitsInfo[i].ParBase = VTlen;
			VTlen += val2;
			sz_log(3,
			    "testdmn: unit %d: code=%c %d %d, parameters: in=%d, out=%d, base=%d\n",
			    i + 1, UnitsInfo[i].UnitCode, UnitsInfo[i].RapId,
			    UnitsInfo[i].SubId, UnitsInfo[i].ParIn,
			    UnitsInfo[i].ParOut, UnitsInfo[i].ParBase);
			if ((UnitsInfo[i].Pars =
			     (short *) calloc(UnitsInfo[i].ParOut,
					      sizeof(short))) == NULL) {
				sz_log(1, "testdmn: memory allocating error (3)");
				exit(1);
			}
			if ((UnitsInfo[i].Sending =
			     (unsigned char *) calloc(UnitsInfo[i].ParOut,
						      sizeof(unsigned char)))
			    == NULL) {
				sz_log(1, "testdmn: memory allocating error (4)");
				exit(1);
			}
			if ((UnitsInfo[i].rtype = (long *)
			     calloc(UnitsInfo[i].ParOut,
				    sizeof(long))) == NULL) {
				sz_log(1, "testdmn: memory allocating error (5)");
				exit(1);
			}
			if ((UnitsInfo[i].Vals = (int *)
			     calloc(UnitsInfo[i].ParIn, sizeof(int))) == NULL) {
				sz_log(1, "testdmn: memory allocating error (6)");
				exit(1);
			}
			for (ii = 0; ii < UnitsInfo[i].ParOut; ii++) {
				UnitsInfo[i].Pars[ii] = SZARP_NO_DATA;
				UnitsInfo[i].Sending[ii] = 0;
			}
		}
		RadioInfo[j].UnitsInfo = UnitsInfo;
	}
	if ((ShmDes = shmget(ftok(linedmnpat, linenum),
			     sizeof(short) * VTlen, 00600)) < 0) {
		sz_log(1, "testdmn: cannot get shared memory segment descriptor");
		exit(1);
	}
	if ((SemDes =
	     semget(ftok(parcookpat, SEM_PARCOOK), SEM_LINE + 2 * linenum, 00600)) < 0) {
		sz_log(1, "testdmn: cannot get parcook semaphor descriptor");
		exit(1);
	}
	if ((MsgSetDes = msgget(ftok(parcookpat, MSG_SET), 00666)) < 0) {
		sz_log(1, "testdmn: cannot get 'set' message queue descriptor");
		exit(1);
	}
	if ((MsgRplyDes = msgget(ftok(parcookpat, MSG_RPLY), 00666)) < 0) {
		sz_log(1, "testdmn: cannot get 'reply' message queue descriptor");
		exit(1);
	}
}

void ReadReport(unsigned char unit)
{
	ushort i;
	tMsgSetParam msg;
	int res, d;
	FILE *data;
	struct stat st;
	char buf[100];
	char *errptr;

	if ((stat(linedev, &st) == 0) && (st.st_mode & S_IFREG))
		data = fopen(linedev, "r");
	else
		data = NULL;

	for (i = 0; i < UnitsInfo[unit].ParIn; i++) {
		d = SZARP_NO_DATA;
		if (data && (fgets(buf, 100, data) != NULL)) {
			char *n = (char*)memchr(buf, '\n', 100);
			if (n)
				*n = 0;
			else
				buf[99] = 0;

			if (buf[0] == 0) 
				continue;

			d = (int)strtod(buf, &errptr); 
			if (*errptr != 0) 
				d = SZARP_NO_DATA;
		}
		UnitsInfo[unit].Vals[i] = d;
	}

	if (data)
		fclose(data);


	Sem[0].sem_num = SEM_LINE + 2 * (LineNum - 1) + 1;
	Sem[0].sem_op = 1;
	Sem[1].sem_num = SEM_LINE + 2 * (LineNum - 1);
	Sem[1].sem_op = 0;
	semop(SemDes, Sem, 2);

	for (i = 0; i < UnitsInfo[unit].ParIn; i++)
		ValTab[UnitsInfo[unit].ParBase + i] = UnitsInfo[unit].Vals[i];

	Sem[0].sem_num = SEM_LINE + 2 * (LineNum - 1) + 1;
	Sem[0].sem_op = -1;
	semop(SemDes, Sem, 1);

	for (i = 0; i < (ushort) UnitsInfo[unit].ParOut; i++)
		if (UnitsInfo[unit].Sending[i]) {
			if (UnitsInfo[unit].Sending[i] != 255) {
				msg.type = UnitsInfo[unit].rtype[i];
				msg.cont.param = i;
				msg.cont.value = UnitsInfo[unit].Pars[i];
				msg.cont.rtype =
					(long) LineNum *256L +
					(long) UnitsInfo[unit].UnitCode;
				msg.cont.retry = UnitsInfo[unit].Sending[i];

				res = msgsnd(MsgRplyDes, &msg,
					     sizeof(tSetParam), IPC_NOWAIT);
				sz_log(3,
				    "testdmn: sending message %ld confirming %ld setting param %d to %d\n",
				    msg.type, msg.cont.rtype, msg.cont.param,
				    msg.cont.value);
				sz_log(3,
				    "testdmn: send result %d with errno %d\n",
				    res, errno);
			}
			UnitsInfo[unit].Sending[i] = 0;
			UnitsInfo[unit].Pars[i] = SZARP_NO_DATA;

		}
}

int main(int argc, char *argv[])
{
	int curradio = 0, i;
	if(argc < 2) {
		fputs(("usage: testdmn [line_number] [filename] [options] \n\
		\rOptions:\n\
		\r\t-d, --delete  - read data form file and delete it immediatly\n"),stderr);
		exit(1);
	}
	
	
	libpar_read_cmdline(&argc, argv);
	loginit_cmdline(2, PREFIX "/logs/testdmn", &argc, argv);

	/* obs³uga opcji */
	LineNum = atoi(argv[1]);
	linedev = argv[2];

	Initialize(LineNum);

	if ((ValTab = (short *) shmat(ShmDes, (void *) 0, 0)) == (void *) -1) {
		sz_log(1, "testdmn: can not attach shared memory segment");
		exit(1);
	}

	while (1) {

		if (RadiosOnLine) {
			curradio =
				(unsigned char) (((int) curradio + 1) %
						 (int) RadiosOnLine);
			UnitsOnLine = RadioInfo[curradio].NumberOfUnits;
			UnitsInfo = RadioInfo[curradio].UnitsInfo;
			for (i = 0; i < UnitsOnLine; i++) {
				ReadReport(i);
			}
			continue;
		}
		for (i = 0; i < UnitsOnLine; i++) {
			sleep(10);
			ReadReport(i);
		}
		if(argc==4 && (strcmp(argv[3],"-d")==0 || strcmp(argv[3],"--delete")==0)) {
		unlink(linedev);
		}	
	}
}
