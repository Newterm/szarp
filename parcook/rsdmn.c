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
 */

/*
 * SZARP 2.0
 * parcook
 * rsdmn.c
 */

/*
 * ulepszony rsdmn
 * Modified by Codematic
 * Obsluga libpar 27.05.1998
 */

#define _IPCTOOLS_H_		/* Nie dolaczaj ipctools.h z szarp.h */
#define _HELP_H_
#define _ICON_ICON_

/* Odkomentowanie tej deklaracji spowoduje czyszczenie flag 
 * c_iflag i c_oflag portu. Czyszczenie flag powoduje niedzia³anie
 * demona z przej¶ciówk± USB/RS232 na driverze PL2303. Nie ¿eby
 * niewyczyszczenie flag cokolwiek psulo (chyba zeby ktos z zewnatrz 
 * cos dziwnego). Wniosek - lepiej nie czy¶ciæ. 
 * ERRATA - trzeba czy¶ciæ ;-) */
#define CLEAR_IOFLAGS

#include<sys/types.h>
#include<sys/stat.h>
#include<sys/uio.h>
#include<sys/time.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<sys/sem.h>
#include<sys/msg.h>
#include<fcntl.h>
#include<time.h>
#include<termio.h>
#include<unistd.h>
#include<signal.h>
#include<string.h>
#include<errno.h>
#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include <ctype.h>

#include "config.h"
#include "szarp.h"
#include "ipcdefines.h"

#define MAXLINES 32
#define MAX_PARS 100
/* suma kontrolna transmisji z sendera - numer pseudo-parametru */
#define CHECKSUM_PARAM 299

/* domyslny odstep pomiedzy zapytaniami do sterownika w sekundach */
#define DEFAULT_DELAY   10

#define DEFAULT_TTY "/dev/ttyS%d"

#define SI_BASE1 0xD0000
#define SI_BASE2 0xD8000
#define SI_MOD1 SI_BASE1+0x180
#define SI_MOD2 SI_BASE1+0x280
#define SI_MOD3 SI_BASE1+0x380
#define SI_MOD4 SI_BASE1+0x480
#define SI_MOD5 SI_BASE2+0x180
#define SI_MOD6 SI_BASE2+0x280
#define SI_MOD7 SI_BASE2+0x380
#define SI_MOD8 SI_BASE2+0x480
#define FOUND 0
#define NOT_FOUND -1

#define YES 1
#define NO 0

static int      LineDes;

static int      LineNum;

static int      SemDes;

static int      ShmDes;

static int      CheckSum;

static unsigned char UnitsOnLine;

static unsigned char RadiosOnLine;

static unsigned char DirectLine;

static unsigned char SpecialUnit;

static unsigned char Diagno = 0;

static unsigned char Simple = 0;

static unsigned char Watch = 0;

static unsigned char UWatch = 0; 

static unsigned char Verbose = 0; 

static unsigned char Quiet = 0; 

static unsigned char *ReadBuf; /* Bufor wszystkich odczytanych danych ze sterownika */
#define  READ_BUF_SIZE 10000

static int      VTlen;

static short   *ValTab;

static struct sembuf Sem[2];

static char     parcookpat[81];

static char     linedmnpat[81];

static char     MyName[] = "Hostxx";

struct phUnit {
	unsigned char   UnitCode;	/* kod sterownika */
	unsigned char   RapId;	/* kod raportu */
	unsigned char   SubId;	/* subkod raportu */
	unsigned char   ParIn;	/* liczba zbieranych parametrow */
	unsigned char   ParOut;	/* liczba ustawianych parametrów */
	unsigned short  ParBase;	/* adres bazowy parametrow w tablicy
					 * linii */
	unsigned char   SumPeriod;	/* okres podstawowego usredniania */
	unsigned char   SumCnt;	/* ilosc pomiarow w buforze usredniania */
	unsigned char   PutPtr;	/* wskaznik na biezacy element w buforze */
	short          *Pars;	/* tablica parametrów do wys³ania */
	unsigned char  *Sending;	/* znaczniki potwierdzeñ wys³ania
					 * parametru */
	int            *AvgSum;	/* sumy buforow usrednien */
	short         **AvgBuf;	/* bufory usrednien */
};

typedef struct phUnit tUnit;

struct phRadio {
	char            RadioName[41];
	unsigned char   NumberOfUnits;
	tUnit          *UnitsInfo;
};

typedef struct phRadio tRadio;

tUnit          *UnitsInfo;

tRadio         *RadioInfo;

/* zwraca aktualn± datê i godzinê */
char *GetLocalDateTime()
{
 time_t timer;
 struct tm *tblock;
 timer = time(NULL);
 tblock = localtime(&timer);
 return (asctime(tblock));
}

/* Formatuje ci±g na wzór terminala HEX-owego */
void DrawHex(unsigned char *c)
{
	int i, j,  k;
	unsigned char ic[17];
	j=0;
	for (i=0;i<strlen((char*)c);i++){
		printf("%02X ",c[i]);
		if ((c[i]>=0x20)&&(c[i])<=0x7f) /* Tylko znaki drukowalne */
		ic[j] = c[i] ;
		else
			ic[j]='.';
		j++;
		ic[j]=0;
		if (((i+1) % 16 == 0)){
			printf("%c%c%c",0x20,0x20,0x20);
			printf("%s\n",ic);
			j = 0;
		}
	}

	if ((i+1) % 16 != 0){
			for (k = 0;k<(16 - (i % 16));k++)
			printf("%c%c%c",0x20,0x20,0x20);
			printf("%c%c%c",0x20,0x20,0x20);
			printf("%s\n",ic);
	}
}

/* Funkcja czyta linia po linii plik textowy */
int readline (FILE *f,unsigned char *buf, int buflen)
{
 int i;
 int result;
 for (i=0;i<buflen;i++)
 buf[i]='\0';
 i = 0;
 result = 0;
 while ((i<buflen)&&(result!=0x0A)&&(result!=0x0D)){
 result = fgetc(f);
 if (result==EOF) 
	 return EOF;
 buf[i] = (unsigned char)result;
 i++;
 }
 i --;
 switch (result){
 case 0x0A:
  buf[i] = '\0';    
 break;
 
 case 0x0D:
    buf[i] = '\0';
    result = fgetc(f);
    break;
 }
 return 0;
}

char CheckString(char *c)
{
 int _i;
 char dgt;
 dgt = YES;
 for (_i=0;_i<strlen(c);_i++)
	 if (!isdigit(c[_i])){
		 dgt = NO;
	 }
 return dgt;
}

/* Funkcja rozkodowuje numer linii na podstawie parcook.cfg */
/* gdy b³±d zwraca NOT_FOUND */
int GetNumberOfLine(char *_devicepath)
{
	char            parcookname[81];
	FILE		*parcookfile;
	int 		numberofline;
	char		devicepath[100];
	int 		filelinestatus;
	unsigned char	filelinebuffer[1000];
	unsigned char	parambuffer[100];
	char 	foundnumberofline; /* Flaga znalezienia wlasciwego numeru linii */
	int		_i, _j;
	unsigned int	linectr;
	unsigned int	numberofdevices ;
	sprintf(parcookname, "%s/parcook.cfg", linedmnpat);
	parcookfile = fopen(parcookname,"r");
	if (parcookfile == NULL){
		ErrMessage(2, parcookname);
	}

	numberofline = NOT_FOUND ;
	foundnumberofline = NOT_FOUND ;
	filelinestatus = ~EOF;
	_j = 0;
	_i=0;
	linectr = 0;
	numberofdevices = 0;
	while ((filelinestatus!=EOF) && (foundnumberofline==NOT_FOUND) && (linectr<=numberofdevices) ){
		/* Odczyt calej linii */
		 filelinestatus=readline(parcookfile,filelinebuffer,sizeof(filelinebuffer));
		 _i=0;
		 /* Odczyt ilo¶ci urz±dzeñ */
		 if (linectr == 0){
			 while ((_i<sizeof(parambuffer)) && (filelinebuffer[_i]!=0x20) && (_j < strlen ((char*)filelinebuffer)) ){
				parambuffer[_i] = filelinebuffer[_i];
				parambuffer[_i+1] = 0;
				_i++ ;
			 }
		 	numberofdevices = atoi((char*)parambuffer); 
		}


		if (linectr>0){
		 /* Czytanie numeru linii */
			 while ((_i<sizeof(parambuffer)) && (filelinebuffer[_i]!=0x20) && (_i < strlen ((char*)filelinebuffer)) ){
				parambuffer[_i] = filelinebuffer[_i];
				parambuffer[_i+1] = 0;
				_i++ ;
		 	}
			 numberofline = atoi((char*)parambuffer);
			 /* Pomijanie identyfikatora SHM & sciezki do demona */

			for (_j=0;_j<2;_j++){
			 	_i++ ; /* Pominiecie spacji */
				while ((_i<strlen((char*)filelinebuffer)) && (filelinebuffer[_i]!=0x20) ){
					_i++;
				}

			}
			 _i++; /* Pomijamy 0x20 */
			 _j = _i;
			 memset(parambuffer,0,sizeof(parambuffer));
			 while (((_j - _i)<sizeof(parambuffer)) && (filelinebuffer[_j]!=0x20) && (_j < strlen((char*)filelinebuffer)) ){
				parambuffer[_j - _i] = filelinebuffer[_j];
				parambuffer[(_j - _i)+1] = 0;
				_j++;
			 }
			 strcpy(devicepath,(char*)parambuffer);
	
			 if (!strcmp (devicepath, _devicepath)){
				foundnumberofline = FOUND ;
			 }


		}
		linectr++;
	}
	fclose(parcookfile);
	if (foundnumberofline == FOUND){
		return (numberofline);
	}else{
		return (NOT_FOUND);

	}
}

void VerboseMode(char id,int LineDes)
{
	char Query[10];
	int _i,_j;

	sprintf(Query,"\x02P%c\x03\n",id);

	printf("On %s sent data: \n",GetLocalDateTime());
	DrawHex((unsigned char*)Query);

	write(LineDes,Query,strlen(Query));
	sleep(5);
	memset(ReadBuf,0,READ_BUF_SIZE);
	_j=0;
	for (_i = 0; read(LineDes, &ReadBuf[_i], 1) == 1; _i++) {
		usleep(1000);
		_j++;
	}

	if (_j){
		printf("On %s received data: \n",GetLocalDateTime());
		DrawHex(ReadBuf);
	}
}

void QuietMode(int LineDes)
{
	int _i, _j;
	sleep(5); /* Odswiezanie */
	memset(ReadBuf,0,READ_BUF_SIZE);
	_j=0;
	for (_i = 0; read(LineDes, &ReadBuf[_i], 1) == 1; _i++) {
		usleep(1000);
		_j++;
	}

	if (_j){
		printf("On %s received data: \n",GetLocalDateTime());
		DrawHex(ReadBuf);
	}
}

/** To jest hack dla E³ku - panowie próbuj± tam udawaæ nasze regulatory i nie
 * podoba im siê znak o kodzie 17, który wysy³amy na pocz±tku zapytania. */
int g_no17 = 0;

void InitPaths()
{
	char *c;
	libpar_init();
	libpar_readpar("", "parcook_path", parcookpat, sizeof(parcookpat), 1);
	libpar_readpar("", "linex_cfg", linedmnpat, sizeof(linedmnpat), 1);
	c = libpar_getpar("", "no17", 0);
	if (c && !strcmp(c, "yes")) {
		g_no17 = 1;
		if (Verbose) {
			printf("0x11 hack enabled!\n");
		}
	}
	libpar_done();
}

void Initialize(unsigned char linenum)
{
	char            ch;
	char           *chptr;
	unsigned char   i,
	                ii,
	                iii,
	                j,
	                maxj;
	int             val,
	                val1,
	                val2,
	                val3,
	                val4;
	char            linedefname[81];
	FILE           *linedef;

	VTlen = 0;
	sprintf(linedefname, "%s/line%d.cfg", linedmnpat, linenum);
	if ((linedef = fopen(linedefname, "r")) == NULL)
		ErrMessage(2, linedefname);

	if (fscanf(linedef, "%d\n", &val) > 0) {
		RadiosOnLine = 0;
		if (val > 0) {
			UnitsOnLine = val;
			DirectLine = 0;
		} else if (val < 0) {
			UnitsOnLine = 1;
			DirectLine = 1;
			SpecialUnit = abs(val);
		} else {
			UnitsOnLine = 1;
			DirectLine = 1;
		}
	} else {
		fscanf(linedef, "%c%d\n", &ch, &val);
		if (ch != 'R')
			exit(-1);
		else
			RadiosOnLine = (unsigned char) val;
	}
	if (Diagno || UWatch || Watch) {
		printf("Cooking deamon for line %d\n", linenum);
		if (RadiosOnLine)
			printf("Allocating structures for %d radios\n",
			       RadiosOnLine);
		else if (SpecialUnit)
			printf
			    ("Allocating structures for Special Unit code: %d\n",
			     SpecialUnit);
		else
			printf("Allocating structures for %d units\n",
			       UnitsOnLine);
	}
	maxj = RadiosOnLine ? RadiosOnLine : 1;
	if ((RadioInfo = (tRadio *) calloc(maxj, sizeof(tRadio))) == NULL)
		ErrMessage(1, NULL);
	for (j = 0; j < maxj; j++) {
		if (RadiosOnLine) {
			fgets(RadioInfo[j].RadioName, 40, linedef);
			if ((chptr =
			     strrchr(RadioInfo[j].RadioName, '\n')) != NULL)
				*chptr = 0;
			fscanf(linedef, "%d\n", &val);
			RadioInfo[j].NumberOfUnits = UnitsOnLine =
			    (unsigned char) val;
			if (Diagno)
				printf("Radio <%s> drives %d units\n",
				       RadioInfo[j].RadioName, UnitsOnLine);
		}
		if ((UnitsInfo =
		     (tUnit *) calloc(UnitsOnLine, sizeof(tUnit))) == NULL)
			ErrMessage(1, NULL);
		for (i = 0; i < UnitsOnLine; i++) {
			fscanf(linedef, "%c %u %u %u %u %u\n",
			       &UnitsInfo[i].UnitCode, &val, &val1, &val2,
			       &val3, &val4);
			UnitsInfo[i].RapId = (unsigned char) val;
			UnitsInfo[i].SubId = (unsigned char) val1;
			UnitsInfo[i].ParIn = (unsigned char) val2;
			UnitsInfo[i].ParOut = (unsigned char) val3;
			UnitsInfo[i].SumPeriod = (unsigned char) val4;
			UnitsInfo[i].SumCnt = 0;
			UnitsInfo[i].PutPtr = 0;
			UnitsInfo[i].ParBase = VTlen;
			VTlen += val2;
			if (Diagno || UWatch || Watch)
				printf
				    ("Unit %d: code=%c %d %d, parameters: in=%d, out=%d, base=%d, period=%d\n",
				     i, UnitsInfo[i].UnitCode,
				     UnitsInfo[i].RapId, UnitsInfo[i].SubId,
				     UnitsInfo[i].ParIn, UnitsInfo[i].ParOut,
				     UnitsInfo[i].ParBase,
				     UnitsInfo[i].SumPeriod);
			if ((UnitsInfo[i].Pars = (short *)
			     calloc(UnitsInfo[i].ParOut,
				    sizeof(short))) == NULL)
				ErrMessage(1, NULL);
			if ((UnitsInfo[i].Sending = (unsigned char *)
			     calloc(UnitsInfo[i].ParOut,
				    sizeof(unsigned char))) == NULL)
				ErrMessage(1, NULL);
			if ((UnitsInfo[i].AvgSum = (int *)
			     calloc(UnitsInfo[i].ParIn, sizeof(int))) == NULL)
				ErrMessage(1, NULL);
			if ((UnitsInfo[i].AvgBuf = (short **)
			     calloc(UnitsInfo[i].ParIn,
				    sizeof(short *))) == NULL)
				ErrMessage(1, NULL);
			for (ii = 0; ii < UnitsInfo[i].ParOut; ii++) {
				UnitsInfo[i].Pars[ii] = SZARP_NO_DATA;
				UnitsInfo[i].Sending[ii] = 0;
			}
			for (ii = 0; ii < UnitsInfo[i].ParIn; ii++) {
				if ((UnitsInfo[i].AvgBuf[ii] = (short *)
				     calloc(UnitsInfo[i].SumPeriod,
					    sizeof(short))) == NULL)
					ErrMessage(1, NULL);
				for (iii = 0; iii < UnitsInfo[i].SumPeriod;
				     iii++)
					UnitsInfo[i].AvgBuf[ii][iii] =
					    SZARP_NO_DATA;
			}
		}
		RadioInfo[j].UnitsInfo = UnitsInfo;
	}
	if (Simple || Watch)
		return;

	if ((ShmDes = shmget(ftok(linedmnpat, linenum),
			     sizeof(short) * VTlen, 00600)) < 0)
		ErrMessage(3, "rsdmn");
	if ((SemDes =
	     semget(ftok(parcookpat, SEM_PARCOOK), SEM_LINE + 2 * linenum, 00600)) < 0)
		ErrMessage(5, "parcook sem");
	if ((MsgSetDes = msgget(ftok(parcookpat, MSG_SET), 00666)) < 0)
		ErrMessage(6, "parcook set");
	if ((MsgRplyDes = msgget(ftok(parcookpat, MSG_RPLY), 00666)) < 0)
		ErrMessage(6, "parcook rply");
}


static int OpenLine(char *line, int baud, int stopb)
{
	int             linedes;
	struct termio   rsconf;

	linedes = open(line, O_RDWR | O_NDELAY | O_NONBLOCK);
	if (linedes < 0)
		return (-1);
	ioctl(linedes, TCGETA, &rsconf);
#ifdef CLEAR_IOFLAGS
	rsconf.c_iflag = 0;
	rsconf.c_oflag = 0;
#endif
	switch (baud) {
	case 2400:
		rsconf.c_cflag = B2400;
		break;
	case 19200:
		rsconf.c_cflag = B19200;
		break;
	case 9600:
	default:
		rsconf.c_cflag = B9600;
	}
	switch (stopb) {
	case 2:
		rsconf.c_cflag |= CSTOPB;
		break;
	case 1:
	default:
		break;
	}
	rsconf.c_cflag |= CS8 | CLOCAL | CREAD;
	rsconf.c_lflag = 0;
	rsconf.c_cc[4] = 0;
	rsconf.c_cc[5] = 0;
	ioctl(linedes, TCSETA, &rsconf);
	return (linedes);
}

static int ReadLine(char *ss)
{
	short           i;

	for (i = 0; read(LineDes, &ss[i], 1) == 1; i++) {
		CheckSum += (uint) ss[i];
		if (ss[i] < 32) {
			ss[i] = 0;
			return (i);
		}
	}
	ss[i] = 0;

	if (i == 0)
		return (-1);
	else
		return (i);
}

void ReadSpecial(unsigned char unit)
{
	static float    Counters[13][3];
	static unsigned char blinker;
	struct tm       at;
	static time_t   ltim;
	time_t          atim,
	                diftim;
	short           ind,
	                vals[MAX_PARS];
	float           fval;
	char            ss[129];
	unsigned char   i,
	                ok,
	                torm;

	atim = 0;
	diftim = 0;
	while (ReadLine(ss) >= 0) {
		at.tm_mon = 15;
		sscanf(ss, "Stan wagi w dniu %d-%d-%d o godz.%d:%d:%d",
		       &at.tm_year, &at.tm_mon, &at.tm_mday, &at.tm_hour,
		       &at.tm_min, &at.tm_sec);
		if (at.tm_mon != 15) {
			if (Simple)
				printf
				    ("Current date %02d-%02d-%02d and time %02d:%02d:%02d captured\n",
				     at.tm_year, at.tm_mon, at.tm_mday,
				     at.tm_hour, at.tm_min, at.tm_sec);
			at.tm_mon -= 1;
			atim = mktime(&at);
			diftim = atim - ltim;
			if (Simple)
				printf("Counting window %ld seconds\n",
				       diftim);
			break;
		}
	}
	for (i = 0; i < 13; i++)
		Counters[i][blinker] = -1.0;
	while (ReadLine(ss) >= 0) {
		if (Simple && strlen(ss) > 0)
			printf("%s\n", ss);
		ind = 0;
		sscanf(ss, "    -  Licznik  nr %hd %f", &ind, &fval);
		if (ind > 0 && ind < 14) {
			/*
			 * printf("Couter %d captured value %0.2f\n", ind,
			 * fval); 
			 */
			Counters[ind - 1][blinker] = fval;
			/*
			 * Counters[ind-1][blinker]=Counters[ind-1][!blinker]+(float)ind/100.0; 
			 */
		}
	}
	ok = 1;
	for (i = 0; i < 13; i++) {
		/*
		 * printf("Counters[%d][%d] - Counters[%d][%d] = %0.2f - %0.2f 
		 * \n", i, blinker, i, !blinker, Counters[i][blinker],
		 * Counters[i][!blinker]); 
		 */
		Counters[i][2] = ltim == 0 ? 0.0 :
		    (Counters[i][blinker] -
		     Counters[i][!blinker]) / diftim * 3600.0;
		if (Simple)
			printf("Flow[%d]=%0.2f T/h\n", i, Counters[i][2]);
		ok &= (Counters[i][blinker] > -0.5);
	}
	if (ok) {
		if (Simple)
			printf("Ok.\n");
		blinker = !blinker;
		ltim = atim;
	}
	torm =
	    UnitsInfo[unit].AvgBuf[0][UnitsInfo[unit].PutPtr] != SZARP_NO_DATA;
	for (i = 0; i < UnitsInfo[unit].ParIn; i++) {
		if (torm)
			UnitsInfo[unit].AvgSum[i] -=
			    UnitsInfo[unit].AvgBuf[i][UnitsInfo[unit].PutPtr];
		if (ok) {
			vals[i] = rint(Counters[i][2] * 100.0);
			UnitsInfo[unit].AvgSum[i] += vals[i];
		} else
			vals[i] = SZARP_NO_DATA;
		UnitsInfo[unit].AvgBuf[i][UnitsInfo[unit].PutPtr] = vals[i];
	}
	UnitsInfo[unit].PutPtr = (unsigned char) (UnitsInfo[unit].PutPtr + 1) %
	    UnitsInfo[unit].SumPeriod;
	if (torm)
		UnitsInfo[unit].SumCnt--;
	if (ok)
		UnitsInfo[unit].SumCnt++;
	if (Simple || Watch)
		return;
	Sem[0].sem_num = SEM_LINE + 2 * (LineNum - 1) + 1;
	Sem[0].sem_op = 1;
	Sem[1].sem_num = SEM_LINE + 2 * (LineNum - 1);
	Sem[1].sem_op = 0;
	Sem[0].sem_flg = Sem[1].sem_flg = SEM_UNDO;
	semop(SemDes, Sem, 2);
	if (UnitsInfo[unit].SumCnt > 0)
		for (i = 0; i < UnitsInfo[unit].ParIn; i++)
			ValTab[UnitsInfo[unit].ParBase + i] = (short)
			    (UnitsInfo[unit].AvgSum[i] /
			     (int) UnitsInfo[unit].SumCnt);
	else
		for (i = 0; i < UnitsInfo[unit].ParIn; i++)
			ValTab[UnitsInfo[unit].ParBase + i] = SZARP_NO_DATA;
	if (Diagno)
		for (i = 0; i < (ushort) VTlen; i++) {
			printf("ValTab[%d]=%d", i, ValTab[i]);
			if (i >= UnitsInfo[unit].ParBase
			    && i <
			    (ushort) (UnitsInfo[unit].ParBase +
				      UnitsInfo[unit].ParIn))
				printf("\t\tval[%d]=%d",
				       i - UnitsInfo[unit].ParBase,
				       vals[i - UnitsInfo[unit].ParBase]);
			printf("\n");
		}
	Sem[0].sem_num = SEM_LINE + 2 * (LineNum - 1) + 1;
	Sem[0].sem_op = -1;
	Sem[0].sem_flg = SEM_UNDO;
	semop(SemDes, Sem, 1);
	if (Diagno) {
		for (i = 0; i < UnitsOnLine; i++) {
			printf("Cnt%d:%d\t", i, UnitsInfo[i].SumCnt);
			printf("Put%d:%d\t", i, UnitsInfo[i].PutPtr);
		}
		printf("\n++++++++++++++++++++++++\n\n");
	}
}

void ReadReport(unsigned char unit, char *chptr)
{
	char            ss[129];
	short           chks,
	                vals[MAX_PARS];
	ushort          i;
	unsigned char   ok,
	                torm,
	                parset;
	tMsgSetParam    msg;
	int ParamsCounter;
	short	*debugsums = NULL;
	unsigned short	sumsptr;
	short	sender_check_sum;


	CheckSum = 0;
	ParamsCounter = 0;
	sumsptr=0;
	if (Watch || UWatch){
		memset(ReadBuf,0,READ_BUF_SIZE);
		debugsums = (short *)malloc(sizeof(short)*UnitsInfo[unit].ParIn);
	}
	ok = (ReadLine(ss) > 0);

	/*
	 * UWAGA 
	 */
	/*
	 * if (UnitsInfo[unit].RapId!=255) 
	 */
	{
		/*
		 * UWAGA - pominiecie RapId 
		 */
		/*
		 * ok=(ok&&atoi(ss)==(int)UnitsInfo[unit].RapId); 
		 */
		if (Simple)
			printf("RapId: %s\n", ss);
	
		if ((Watch || UWatch)&& ok){
			sprintf((char*)ReadBuf,"%s%s\r",ReadBuf,ss);
			debugsums[sumsptr++] = CheckSum; 
		}else
		if ((Watch || UWatch)&& !ok){
			printf("Sorry - NO REPLY (RapId) \n");
		}


		ok = (ok && ReadLine(ss) > 0);
		/*
		 * UWAGA - pominiecie Subid 
		 */
		/*
		 * ok=(ok&&atoi(ss)==(int)UnitsInfo[unit].SubId); 
		 */
		if (Simple)
			printf("SubId: %s\n", ss);

		if ((Watch || UWatch)&& ok){
			sprintf((char*)ReadBuf,"%s%s\r",ReadBuf,ss);
			debugsums[sumsptr++] = CheckSum; 

		}else
		if ((Watch || UWatch)&& !ok){
			printf("Sorry - NO REPLY (SubId) \n");

		}

		ok = (ok && ReadLine(ss) > 0);

		if ((Watch || UWatch)&& ok){
			sprintf((char*)ReadBuf,"%s%s\r",ReadBuf,ss);
			debugsums[sumsptr++] = CheckSum; 
		}else
		if ((Watch || UWatch)&& !ok){
			printf("Sorry - NO REPLY (FunId) \n");
		}

		ok = (ok && ss[0] == UnitsInfo[unit].UnitCode);

		if ((Watch || UWatch) && !ok){
			printf("Sorry - Report: %c | Configuration: %c are mismatch!\n",ss[0], UnitsInfo[unit].UnitCode);
		}

		if (Simple)
			printf("FunId: %s\n", ss);

		ok = (ok && ReadLine(ss) > 0);
		if ((Watch || UWatch)&& ok){
			sprintf((char*)ReadBuf,"%s%s\r",ReadBuf,ss);
			debugsums[sumsptr++] = CheckSum; 
		}else
		if ((Watch ||UWatch) && !ok){
			printf("Sorry - NO REPLY (Date/Time)");
		}

	}
	for (i = 0; i < UnitsInfo[unit].ParIn; i++) {
		ok = (ok && ReadLine(ss) > 0);
		if (Watch || UWatch){
			if (ok) ParamsCounter++;
			sprintf((char*)ReadBuf,"%s%s\r",ReadBuf,ss);
			debugsums[sumsptr++] = CheckSum; 
		}

		vals[i] = atoi(ss);

		if (Simple)
			printf("vals[%d]=%d\n", i, vals[i]);
	}

	if ((Watch || UWatch)&& (ParamsCounter != UnitsInfo[unit].ParIn) ){
		printf("Sorry - Number of params mismatch; Received: %d, From configuration: %d\n ", ParamsCounter, UnitsInfo[unit].ParIn);
	}

	chks = CheckSum;
	ok = (ok && ReadLine(ss) > 0);
	if (Watch || UWatch){
		sprintf((char*)ReadBuf,"%s%s\r\r",ReadBuf,ss);
		debugsums[sumsptr++] = CheckSum; 
	}

	ok = (ok && chks == atoi(ss));

	if ((Watch || UWatch)&& ok == 0){
		printf("Sorry - Check sum mismatch; Received: %d, Calculated: %d\n ",atoi(ss),chks);
		printf("Partially checksums: \n");
		for (i = 0; i < UnitsInfo[unit].ParIn; i++) {
			printf("[%d] %d\n",i,debugsums[i]);
		}

	}

	if (Watch || UWatch){
		printf("On %s received data: \n", GetLocalDateTime());
		DrawHex(ReadBuf);
		free(debugsums);
	}

	if (Simple)
		printf("ok=%d\n\n", ok);
	torm =
	    UnitsInfo[unit].AvgBuf[0][UnitsInfo[unit].PutPtr] != SZARP_NO_DATA;
	for (i = 0; i < UnitsInfo[unit].ParIn; i++) {
		if (torm)
			UnitsInfo[unit].AvgSum[i] -=
			    UnitsInfo[unit].AvgBuf[i][UnitsInfo[unit].PutPtr];
		if (ok)
			UnitsInfo[unit].AvgSum[i] += vals[i];
		else
			vals[i] = SZARP_NO_DATA;
		UnitsInfo[unit].AvgBuf[i][UnitsInfo[unit].PutPtr] = vals[i];
	}
	UnitsInfo[unit].PutPtr = (unsigned char) (UnitsInfo[unit].PutPtr + 1) %
	    UnitsInfo[unit].SumPeriod;
	if (torm)
		UnitsInfo[unit].SumCnt--;
	if (ok)
		UnitsInfo[unit].SumCnt++;
	if (Simple || Watch)
		return;
	Sem[0].sem_num = SEM_LINE + 2 * (LineNum - 1) + 1;
	Sem[0].sem_op = 1;
	Sem[1].sem_num = SEM_LINE + 2 * (LineNum - 1);
	Sem[1].sem_op = 0;
	Sem[0].sem_flg = Sem[1].sem_flg = SEM_UNDO;
	semop(SemDes, Sem, 2);
	if (UnitsInfo[unit].SumCnt > 0)
		for (i = 0; i < UnitsInfo[unit].ParIn; i++)
			ValTab[UnitsInfo[unit].ParBase + i] = (short)
			    (UnitsInfo[unit].AvgSum[i] /
			     (int) UnitsInfo[unit].SumCnt);
	else
		for (i = 0; i < UnitsInfo[unit].ParIn; i++)
			ValTab[UnitsInfo[unit].ParBase + i] = SZARP_NO_DATA;
	while (ReadLine(ss) >= 0);
	if (Diagno)
		for (i = 0; i < (ushort) VTlen; i++) {
			printf("ValTab[%d]=%d", i, ValTab[i]);
			if (i >= UnitsInfo[unit].ParBase
			    && i <
			    (ushort) (UnitsInfo[unit].ParBase +
				      UnitsInfo[unit].ParIn))
				printf("\t\tval[%d]=%d",
				       i - UnitsInfo[unit].ParBase,
				       vals[i - UnitsInfo[unit].ParBase]);
			printf("\n");
		}
	/*
	 * UWAGA - poni¿szy fragment jest bez sensu, gdy¿ sender nigdy nie
	 * bêdzie oczekiwa³ komunikatów o takim typie. 
	 */
#if 0
	for (i = 0; i < (ushort) UnitsInfo[unit].ParOut; i++)
		if (UnitsInfo[unit].Sending[i]) {
			UnitsInfo[unit].Sending[i] = 0;
			if (ok) {
				msg.type =
				    (long) LineNum *256L +
				    (long) UnitsInfo[unit].UnitCode;
				msg.cont.param = i;
				msg.cont.value = UnitsInfo[unit].Pars[i];
				msgsnd(MsgRplyDes, &msg, sizeof(tSetParam),
				       IPC_NOWAIT);
			}
		}
#endif
	/* Koniec fragmentu bez sensu (co nie oznacza, ¿e zaczynaj± siê
	 * fragmenty z sensem)... */
	unit = (unsigned char) (unit + 1) % UnitsOnLine;
	/*
	 * UWAGA 
	 */
	/*
	 * sprintf(chptr,"\x02Pn"); 
	 */
	if (g_no17) {
		sprintf(chptr, "\x02Pn");
	} else {
		sprintf(chptr, "\x11\x02Pn");
	}
	parset = 0;
	msg.type = (long) LineNum *256L + (long) UnitsInfo[unit].UnitCode;

	sender_check_sum = 0;
	while (msgrcv(MsgSetDes, &msg, sizeof(tSetParam), msg.type, IPC_NOWAIT)
	       == sizeof(tSetParam))
		if (msg.cont.param < UnitsInfo[unit].ParOut) {
			sprintf(ss, "%d,%d,", msg.cont.param, msg.cont.value);
			strcat(chptr, ss);
			UnitsInfo[unit].Sending[i] = 1;
			parset = 1;
			sender_check_sum += msg.cont.param + msg.cont.value;
			sender_check_sum &= 0xffff;
		}
	if(parset)
	{
	  sprintf(ss, "%d,%d,", CHECKSUM_PARAM, sender_check_sum);
	  strcat(chptr, ss);
	}
	chptr[strlen(chptr) - parset] = 0;
	strcat(chptr, "\x03");	/* mo¿e dodaæ NL */
	Sem[0].sem_num = SEM_LINE + 2 * (LineNum - 1) + 1;
	Sem[0].sem_op = -1;
	Sem[0].sem_flg = SEM_UNDO;
	semop(SemDes, Sem, 1);
	if (Diagno) {
		for (i = 0; i < (int) UnitsOnLine; i++) {
			printf("Cnt%d:%d\t", i, UnitsInfo[i].SumCnt);
			printf("Put%d:%d\t", i, UnitsInfo[i].PutPtr);
		}
		printf("\n++++++++++++++++++++++++\n\n");
	}
}

void ConnectRadio(unsigned char unit)
{
	unsigned char   state = 0;
	char            cmdbuf[256];

	while (1) {
		printf("state: %d\n", state);
		switch ((int) state) {
		case 0:
			sprintf(cmdbuf, "");
			write(LineDes, cmdbuf, strlen(cmdbuf));
			sleep(5);
			sprintf(cmdbuf, "\r");
			write(LineDes, cmdbuf, strlen(cmdbuf));
			sleep(1);
			state = 1;
			break;
		case 1:
			while (ReadLine(cmdbuf) >= 0) {
				printf("cmdbuf->%s\n", cmdbuf);
				if (strfind(cmdbuf, "cmd:") >= 0) {
					state = 2;
					break;
				}
			}
			if (state != 2)
				state = 0;
			break;
		case 2:
			sprintf(cmdbuf, "disconnect\r");
			write(LineDes, cmdbuf, strlen(cmdbuf));
			sleep(3);
			state = 3;
			break;
		case 3:
			while (ReadLine(cmdbuf) >= 0) {
				printf("cmdbuf->%s\n", cmdbuf);
				if (strfind(cmdbuf, "cmd:") >= 0) {
					state = 4;
					break;
				}
			}
			if (state != 4)
				state = 0;
			break;
		case 4:
			sprintf(cmdbuf, "reset\r");
			write(LineDes, cmdbuf, strlen(cmdbuf));
			sleep(3);
			state = 5;
			break;
		case 5:
			while (ReadLine(cmdbuf) >= 0) {
				printf("cmdbuf->%s\n", cmdbuf);
				if (strfind(cmdbuf, "cmd:") >= 0) {
					state = 6;
					break;
				}
			}
			if (state != 6)
				state = 0;
			break;
		case 6:
			sprintf(cmdbuf, "mycall %s\r", MyName);
			write(LineDes, cmdbuf, strlen(cmdbuf));
			sleep(3);
			state = 7;
			break;
		case 7:
			while (ReadLine(cmdbuf) >= 0) {
				printf("cmdbuf->%s\n", cmdbuf);
				if (strfind(cmdbuf, "cmd:") >= 0) {
					state = 8;
					break;
				}
			}
			if (state != 8)
				state = 0;
			break;
		case 8:
			sprintf(cmdbuf, "connect %s\r",
				RadioInfo[unit].RadioName);
			write(LineDes, cmdbuf, strlen(cmdbuf));
			sleep(3);
			state = 9;
			break;
		case 9:
			while (ReadLine(cmdbuf) >= 0) {
				printf("cmdbuf->%s\n", cmdbuf);
				if ((int) strfind(cmdbuf, "CONNECTED to") >= 0) {
					state = 10;
					break;
				}
			}
			if (state != 10)
				state = 0;
			break;
		case 10:
			sprintf(cmdbuf, "trans\r");
			write(LineDes, cmdbuf, strlen(cmdbuf));
			sleep(3);
			while (ReadLine(cmdbuf) >= 0);
			return;
			break;
		default:
			break;
		}
	}
}

void Usage()
{
printf("Demon sterowników ZET, SK2000, SK4000\n");
printf("Sposób wywo³ania: \n");
printf("   rsdmn [s|d|u|w] [conf_entry] [device] [baud_rate] [stop_bits] [protocol] [--askdelay=] [--askradiodelay=]\n");
printf("gdzie: \n");
printf(" s - podstawowy tryb DEBUG bez komunikacji z parcookiem \n");
printf(" d - podstawowy tryb DEBUG wraz z komunikacj± z parcookiem \n");
printf(" w - rozszerzony tryb DEBUG bez komunikacji z parcookiem \n");
printf(" u - rozszerzony tryb DEBUG wraz z komunikacj± z parcookiem \n");
printf(" vX - rozszerzony tryb DEBUG bez komunikacji z parcookiem, z pominiêciem konfiguracji: X - id sterownika\n");
printf(" q - rozszerzony tryb DEBUG bez komunikacji z parcookiem tylko nas³uchiwanie portu\n");
printf(" conf_entry - numer linii demona. Parametr jest opcjonalny tylko przy jednym z trybów DEBUG \n");
printf(" device - ¶cie¿ka do portu szeregowego np. /dev/ttyS0\n");
printf(" baud_rate - ¶cie¿ka do portu szeregowego. domy¶lna warto¶æ 9600\n");
printf(" protocol - typ protoko³u (w zale¿no¶ci od urz±dzenia)\n");
printf(" --askdelay - czas opó¼nienia [s] pomiêdzy zapytaniami dla sterowników\n");
printf(" --askradiodelay - czas opó¼nienia [s] pomiêdzy zapytaniami dla radiomodemów\n");
exit(1);
}

int main(int argc, char *argv[])
{
	
	int             baud,
	                stopb;
	ushort          i;
	ushort		j;
	ushort		k;
	unsigned char   curradio = 0;
	unsigned char   parind;
	const unsigned char askdelay_str[] = "--askdelay";
	const unsigned char askradiodelay_str[] = "--askradiodelay";
	char		buffer[100];
	ushort		askdelay = DEFAULT_DELAY;
	ushort		askradiodelay = 20 ;
#define ASK_DELAY 0
#define ASK_RADIO_DELAY 1
	unsigned char	lelo ;
	char	islinenumber;


	/*
	 * UWAGA 
	 */
	/*
	 * char obuf[129]="\x02Pn\x03\n"; 
	 */
	char            obuf[129] = "\x11\x02Pn\x03\n";
	char            rbuf[5] = "\x11P\x0a";
	char            wbuf[8] = "E0\r";
	char            linedev[30];

	libpar_read_cmdline(&argc, argv);
	if (argc < 2)
		Usage();
	Diagno = (argv[1][0] == 'd');
	Simple = (argv[1][0] == 's');
	Watch  = (argv[1][0] == 'w');
	UWatch  = (argv[1][0] == 'u');
	Verbose = (argv[1][0] == 'v');
	Quiet = (argv[1][0] == 'q');

	if (Verbose && argc!=3) Usage();

//	fprintf(stderr,"%d",argc);
	/* Kolejny etap parsera - sprawdzanie czy podano numer linii czy nie */
	islinenumber = FOUND;
	if (Simple || Watch || Diagno || UWatch){
		/* W tym trybie trzba mieæ co najmniej 3 parametry np rsdmn s /dev/ttyS0 */
		if (argc<3) Usage();
		if (!CheckString(argv[2])) islinenumber = NOT_FOUND ;
	}

	for (i = 1;i< (unsigned int)argc;i++ ){ /* Po wszystkich parametrach */
		/* Na poczatku wylapuje tylko te parametry co maja znak == */
		memset(buffer,0,sizeof(buffer));
		j = 0;
		lelo = 0;
		while(j<sizeof(buffer) && (j<strlen(argv[i])) ){
			if (argv[i][j] =='='){
				lelo = 1;
				j++;
				break ;
			}
			buffer[j] = argv[i][j];
			buffer[j + 1] = 0;
			j++;
		}


		if ( (lelo) && ((!strcmp(buffer ,(char*)askdelay_str)) || (!strcmp(buffer ,(char*)askradiodelay_str))) ){
			if (strcmp(buffer ,(char*)askdelay_str) == 0 ){
				lelo = ASK_DELAY ;
			}

			if (strcmp(buffer ,(char*)askradiodelay_str) == 0 ){
				lelo = ASK_RADIO_DELAY ;
			}

			memset(buffer,0,sizeof(buffer));
			k = j;
			while(k<strlen(argv[i])&&(k-j < sizeof(buffer))){
				buffer[k-j] = argv[i][k];
				buffer[(k - j) + 1] = 0;
				k++;
			}

			switch (lelo){
				case ASK_DELAY:
					askdelay = atoi(buffer);
					break;
			
				case ASK_RADIO_DELAY:
					askradiodelay = atoi(buffer);
					break;

			}

		}
	}


	if (Simple)
		Diagno = 1;
	if (Diagno)
		parind = 2;
	else
		if (Watch || UWatch || Verbose || Quiet){
			parind = 2;
			ReadBuf = (unsigned char *)malloc(sizeof(unsigned char)*READ_BUF_SIZE) ;

		}
		else
		parind = 1;
	if (Verbose || Quiet){
		islinenumber = NOT_FOUND; 
		LineNum = 0;
	}
	if (islinenumber == NOT_FOUND){ //przesuwanie g³ównego wska¼nika parsera
		parind = 1;
	}

	if (argc < (int) parind + 1)
		Usage();
	baud = 9600;
	stopb = 1;
	if (islinenumber == FOUND)
	LineNum = atoi(argv[parind]);

	sprintf(linedev, DEFAULT_TTY, LineNum - 1); 



	if (argc > (int) parind + 1)
		strcpy(linedev, argv[parind + 1]);
		else if (islinenumber == NOT_FOUND){
			Usage();
		}
	if (argc > (int) parind + 2)
		baud = atoi(argv[parind + 2]);
	if (argc > (int) parind + 3)
		stopb = atoi(argv[parind + 3]);

	if (Verbose) {
		while (1==1){
			LineDes = OpenLine(linedev, baud, stopb);
			if (strlen(argv[1])==1)
			VerboseMode('1', LineDes);
			else
			VerboseMode(argv[1][1], LineDes);
			close(LineDes);
		}
		return 0;
	}

	if (Quiet) {
		printf("Quiet mode started on: %s \n", GetLocalDateTime());
		while (1==1){
			LineDes = OpenLine(linedev, baud, stopb);
			QuietMode(LineDes);
			close(LineDes);
		}
		return 0;
	}

	InitPaths();
	
	if (g_no17) {
		sprintf(obuf, "\x02Pn\x03\n");
	}


	if (islinenumber == NOT_FOUND){
		LineNum = GetNumberOfLine(linedev);
		if (LineNum == NOT_FOUND){
			printf("ERROR: %s not found in configuration\n",linedev);
			exit (1);
		}
	}


	Initialize(LineNum);
	

	/*
	 * if ((LineDes=OpenLine(linedev))<0) ErrMessage(2,linedev); 
	 */
	/*
	 * if ((memdes=open("/dev/mem",O_RDONLY))==-1)
	 * ErrMessage(2,"/dev/mem"); 
	 */
	LineNum--;
	/*
	 * ctrmemaddr=(LineNum<8?SI_MOD1:LineNum<16?SI_MOD2:LineNum<24?SI_MOD3:
	 * LineNum<32?SI_MOD4:LineNum<40?SI_MOD5:LineNum<48?SI_MOD6:
	 * LineNum<56?SI_MOD7:SI_MOD8)+0x300*(LineNum<32?LineNum:
	 * LineNum-32)+0x0c; lseek(memdes,ctrmemaddr,SEEK_SET); 
	 */
	if (!Simple && !Watch) {
		if ((ValTab =
		     (short *) shmat(ShmDes, (void *) 0, 0)) == (void *) -1) {
			printf("Can not attach shared segment\n");
			exit(-1);
		}
	}

	while (1) {
		if (RadiosOnLine) {
			ConnectRadio(curradio);
			curradio =
			    (unsigned char) (((int) curradio + 1) %
					     (int) RadiosOnLine);
		}
		for (i = 0; i < UnitsOnLine; i++) {
			if ((LineDes = OpenLine(linedev, baud, stopb)) < 0)
				ErrMessage(2, linedev);

			/*
			 * if (!RadiosOnLine) { bread=TIOCM_RTS;
			 * ioctl(LineDes,TIOCMBIS,&bread); } 
			 */
			if (SpecialUnit == 1) {
				if (Simple)
					printf("Probing:%s\n", wbuf);
				write(LineDes, wbuf, strlen(wbuf));
			} else if (!DirectLine) {
				/*
				 * UWAGA 
				 */
				/*
				 * obuf[2]=UnitsInfo[i].UnitCode; 
				 */
				if (g_no17) {
					obuf[2] = UnitsInfo[i].UnitCode;
				} else {
					obuf[3] = UnitsInfo[i].UnitCode;
				}

				if (Watch || UWatch){
					printf("On %s sent data: \n",GetLocalDateTime());
					DrawHex((unsigned char*)obuf);
				}

				if (Simple)
					printf("Sending:%s\n", obuf);
				write(LineDes, obuf, strlen(obuf));
			} else {
				write(LineDes, rbuf, 3);
				if (Simple)
					printf("Sending directly:%s\n", rbuf);
			}
			/*
			 * do { read(memdes,mtab,2);
			 * lseek(memdes,ctrmemaddr,SEEK_SET); }
			 * while(mtab[1]!=mtab[0]); 
			 */
			/*
			 * if (!RadiosOnLine) {
			 * ioctl(LineDes,TIOCMBIC,&bread);
			 * ioctl(LineDes,TCFLSH,0); } 
			 */
			if (RadiosOnLine)
				sleep(askradiodelay);
			else
				sleep(askdelay);
			if (SpecialUnit)
				ReadSpecial(i);
			else
				ReadReport(i, obuf);

			close(LineDes);
		}
	}
	if (Watch || UWatch || Verbose || Quiet)
		free (ReadBuf);
}
