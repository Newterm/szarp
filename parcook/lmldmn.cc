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
 @description_start
 @class 2
 @devices Lumel RE-31 PID regulator.
 @devices.pl Regulator PID Lumel RE-31.
 @config_example
 	Numery parametrów z sekcji "OPTIONS" params.xml:
	Czas wytrzymania #DataCode = 1 #Comma = 1
	Prêdko¶æ narostu warto¶ci zadanej #DataCode = 2 #Comma = 1
	Kompensacja odchylenia sygna³u wej¶ciowego #DataCode = 3 #Comma = 2
	Przesuniêcie warto¶ci regulowanej #DataCode = 4 #Comma = 1
	Nastawa zakresu proporcjonalno¶ci PID-a #DataCode = 5 #Comma = 0
	Nastawa czasu ca³kowania PID-a #DataCode = 6 #Comma = 0
	Nastawa czasu ró¿niczkowania PID-a #DataCode = 7 #Comma = 0
	Histereza alarmu 1 #DataCode = 8 #Comma = 1
	Histereza dla regulacji dwustanowej #DataCode = 9 #Comma = 1
	Adres urz±dzenia w sieci #DataCode = 10 #Comma = 0
	Dolna granica zakresu #DataCode = 11 #Comma = 1
	Górna granica zakresu #DataCode = 12 #Comma = 1
	Ograniczenie mocy wyj¶cia 1 #DataCode = 13 #Comma = 0
	Ograniczenie mocy wyj¶cia 2 #DataCode = 14 #Comma = 0
	Rodzaj sygna³u wej¶ciowego #DataCode = 15 #Comma = 0
	Typ jednostk #DataCode = 16 #Comma = 0
	Rozdzielczo¶ #DataCode = 17 #Comma = 0
	Rodzaj regulacj #DataCode = 18 #Comma = 0
	Typ alarmu #DataCode = 19 #Comma = 0
	Funkcje alarmu 2 #DataCode = 20 #Comma = 0
	Okres impulsowania dla wyj¶cia 1 #DataCode = 21 #Comma = 0
	Okres impulsowania dla wyj¶cia ch³odzeni #DataCode = 22 #Comma = 0
	Zakres proporcjonalno¶ci dla wyj¶cia ch³odzenia #DataCode = 23 #Comma = 1
	Strefa nieczu³o¶ci #DataCode = 24 #Comma = 1
	Warto¶æ mierzona #DataCode = 25 #Comma = 0
	Warto¶æ zadana #DataCode = 26 #Comma = 1 
	Warto¶æ sygna³u na wyj¶ciu 1 # DataCode = 27 #Comma = 0
	Warto¶æ sygna³u na wyj¶ciu 2 #DataCode = 28 #Comma = 0
	Warto¶æ alarmu 2 lub czas wytrzymania #DataCode = 29 #Comma = 1
	Histereza alarmu 2 #DataCode = 30 #Comma = 1
	Typ alarmu 2 #DataCode = 31 #Comma = 0
	Funkcje alarmu 2 #DataCode = 32 #Comma = 0
 @description_end
*/

#define _IPCTOOLS_H_

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
#include<libgen.h>
#include<math.h>
#include <ctype.h>

#include "libpar.h"
#include "liblog.h"
#include "ipcdefines.h"
#define READ_FUN 65

#define MAXLINES 32
#define ERROR -1

#undef DEBUG			/* debug wewnêtrzny */
/*
*/

static int DeviceId;

static int SemDes;

static int ShmDes;

static unsigned char Diagno = 0;

static unsigned char Simple = 0;

static unsigned VTlen;

static char parcookpat[81];

static char linedmnpat[81];

static unsigned char UnitsOnLine;

static short *ValTab;

static struct sembuf Sem[2];

#define TRUE 1
#define FALSE 0

#define MAX_DATA 32		/* Maksymalna liczba parametrow przychodzaca z LUMELA */

/* Zwraca ID urz±dzenia, inicjalizuje segmenty pamieci dzielonej */
void
Initialize (unsigned char linenum)
{
  char linedefname[81];
  FILE *linedef;
  int val, val1, val2, val3, val4, val5, val6, i;

  libpar_init ();
  libpar_readpar ("", "parcook_path", parcookpat, sizeof (parcookpat), 1);
  libpar_readpar ("", "linex_cfg", linedmnpat, sizeof (linedmnpat), 1);
  libpar_done ();

  sprintf (linedefname, "%s/line%d.cfg", linedmnpat, linenum);
  if ((linedef = fopen (linedefname, "r")) == NULL)
    {
      sz_log (2, "testdmn: cannot open file '%s'", linedefname);
      exit (1);
    }

  if (fscanf (linedef, "%d\n", &val) > 0)
    {
      if (val > 0)
	UnitsOnLine = val;
      else
	UnitsOnLine = 1;
    }
  else
    exit (-1);


  VTlen = 0;
  for (i = 0; i < UnitsOnLine; i++)
    {
      fscanf (linedef, "%u %u %u %u %u %u\n", &val1,
	      &val2, &val3, &val4, &val5, &val6);
      VTlen += val4;
    }

  fclose (linedef);
  DeviceId = val1;

  if (Diagno)
    printf ("Cooking deamon for line %d -  deamon\n", linenum);
  if (Diagno)
    printf
      ("Unit 1: no code/rapid/subid, parameters: in=2, no out, period=1\n");
  if (Simple)
    return;
  if ((ShmDes =
       shmget (ftok (linedmnpat, linenum), sizeof (short) * VTlen,
	       00600)) < 0)
    {
      sz_log (0, "lmldmn: cannot get shared memory segment descriptor");
      exit (1);
    }

  if ((SemDes =
       semget (ftok (parcookpat, SEM_PARCOOK), SEM_LINE + 2 * linenum, 00600)) < 0)
    {
      sz_log (0, "lmldmn: cannot get parcook semaphor descriptor");
      exit (1);
    }
}

/* Liczy sumê CRC do zapytañ */
char *
CRC (char *inSTR)
{
  const int CRC_LEN = 3;	/* CRC ma 2 bajty +\0 */
  unsigned int tmpCRC;		/* Zmienna przechowujaca CRC z przeniesieniem */
  char *oCRC;			/* wskaznik do sumy CRC */
  int initi;
  tmpCRC = 0;
  if (inSTR[0] == ':')
    {
      initi = 1;
    }
  else
    {
      initi = 0;
    }

  for (unsigned i = initi; i < strlen (inSTR); i++)
    {
      tmpCRC += inSTR[i];
    }
  tmpCRC &= 0x00ff;		/* konwersja na sume bez przeniesienia */
  tmpCRC = ~tmpCRC;		/* negacja */
  tmpCRC &= 0x00ff;		/* konwersja na sume bez przeniesienia */
  tmpCRC++;			/* Dodanie 1 */
  oCRC = (char *) malloc (CRC_LEN);
  sprintf (oCRC, "%02X", tmpCRC);
  return oCRC;
}

char *
MakeQuery (char ADD, char CMD, char PARA)
{
  const char QUERY_LEN = 12;	/* 1x: 2xPARA + 2xCMD + 2xPARA + 2xCRC +1x\r +1x\n +1xNULL */
  char *tmpMakeQuery;
  char buf[20];
  char sbuf[3];
  char *strCRC;
  tmpMakeQuery = (char *) malloc (QUERY_LEN);
  strcpy (buf, "");
  sprintf (sbuf, "%02d", ADD);
  strcat (buf, sbuf);
  sprintf (sbuf, "%02d", CMD);
  strcat (buf, sbuf);
  sprintf (sbuf, "%02d", PARA);
  strcat (buf, sbuf);
  strCRC = CRC (buf);
  sprintf (tmpMakeQuery, ":%s%s\r\n", buf, strCRC);
  free (strCRC);
  return (tmpMakeQuery);
}

static int
OpenLine (char *line)
{
  int linedes;
  struct termio rsconf;
  linedes = open (line, O_RDWR | O_NDELAY | O_NONBLOCK);
  if (linedes < 0)
    return (-1);
  ioctl (linedes, TCGETA, &rsconf);
  rsconf.c_iflag = 0;
  rsconf.c_oflag = 0;
  rsconf.c_cflag = B9600 | CRTSCTS | CS7 | PARENB | CLOCAL | CREAD;
  rsconf.c_lflag = 0;
  rsconf.c_cc[4] = 0;
  rsconf.c_cc[5] = 0;
  ioctl (linedes, TCSETA, &rsconf);
  tcflush (linedes, TCIFLUSH);
  tcflush (linedes, TCOFLUSH);
  return (linedes);
}

void
SendQuery (int linedes, char *Query)
{
  for (unsigned i = 0; i < strlen (Query); i++)
    {
      write (linedes, &Query[i], 1);
      usleep (1000 * 10);
    }
}

int
ReceiveData (int linedes, int *Param, int HowCodes, int *Codes, int *Commas)
{
  int res;
  const int Offset = 9;
#define LENGTH_DATA  7		/* 6xznak + \0 */
  int i;
  char Codebuf[3];		/* kody funkcji */
  int Code;
  int dbufPTR;
  char buf[255];
  char dbuf[LENGTH_DATA];


  strcpy (buf, "");
  res = read (linedes, buf, sizeof (buf));
  if (!res)
    {
      return SZARP_NO_DATA;
    }
  Codebuf[0] = buf[Offset - 2];
  Codebuf[1] = buf[Offset - 1];
  Codebuf[2] = '\0';
  *Param = 0;
  Code = atoi (Codebuf);
  for (i = 0; i < HowCodes; i++)
    {
      if (Code == Codes[i])
	{
	  *Param = i;
	  break;
	}
    }
  dbufPTR = 0;
  for (i = 0; i < LENGTH_DATA - 1; i++)
    {
      if (Commas == 0)
	{
	  dbuf[dbufPTR] = buf[i + Offset];
	  dbufPTR++;
	  dbuf[dbufPTR] = '\0';
	}
      if ((Commas > 0) && (i != ((LENGTH_DATA - 2) - Commas[(*Param)])))
	{
	  dbuf[dbufPTR] = buf[i + Offset];
	  dbufPTR++;
	  dbuf[dbufPTR] = '\0';
	}
    }

  if (Diagno)
    {
      printf ("%s\n", buf);
    }
  return atoi (dbuf);
}

void
Usage (char *name)
{
  printf ("LUMEL RE-31 driver\n");
  printf ("Usage:\n");
  printf ("      %s [s|d] <n> <port> <--DataCodes=> <--Commas=>\n", name);
  printf ("Where:\n");
  printf ("[s|d] - optional simple (s) or diagnostic (d) modes\n");
  printf ("n - line number (necessary to read line<n>.cfg file)\n");
  printf ("port - full path to port\n");
  printf
    ("--DataCodes= - Number-s of function-s to read. Numbers must be separated commas(,),without space\n");
  printf ("--Commas= - Comma position (from right side) for read value\n");
  exit (0);
}

int
main (int argc, char *argv[])
{
  const char CodeName[] = "--DataCodes";
  const char CommaName[] = "--Commas";
  char linedev[100];
  int parind;
  char StrCodes[100];
  char StrCommas[100];
  int Param;
  int Codes[100];
  int Commas[100];
  int linedes;
  int LineNum;
  unsigned i, j;
  int Data;
  int DataBuffer[MAX_DATA];
  char FirstTime;
  unsigned HowCodes;
  unsigned HowCommas;
  char *p;
  char *Query;
  char ArgName[100];		/* Nazwa argumentu */
  char ArgData[1000];		/* parametry argumentu */
  char Ps;			/* Flaga poprawnosci DataCodes */
  FirstTime = TRUE;
  if ((argc < 4 + 1) || (argc > 5 + 1))	/* tylko 4 lub parametrow */
    Usage (argv[0]);
  Diagno = (argv[1][0] == 'd');
  Simple = (argv[1][0] == 's');
  if (Simple)
    Diagno = 1;
  if (Diagno)
    parind = 2;
  else
    parind = 1;

  if (isdigit (argv[parind][0]))
    {
      LineNum = atoi (argv[parind]);
    }
  else
    {
      LineNum = ERROR;
    }
  if (LineNum == ERROR)
    {
      sz_log (0, "n - missing parametr\n");
      exit (1);
    }
  strcpy (linedev, argv[parind + 1]);

  /* Sprawdzenie czy w parametrach danych znajduje siê znak = */
  Ps = FALSE;
  for (i = 0; i < strlen (argv[parind + 2]); i++)
    {
      if (argv[parind + 2][i] == '=')
	{
	  Ps = TRUE;
	}
    }
  if (Ps == FALSE)
    {
      sz_log (0, "Parametr %s/%s has no '=' argument \n", CodeName,
	      CommaName);
      exit (-3);
    }

  for (i = 0; i < strlen (argv[parind + 3]); i++)
    {
      if (argv[parind + 3][i] == '=')
	{
	  Ps = TRUE;
	}
    }

  if (Ps == FALSE)
    {
      sz_log (0, "Parametr %s/%s has no '=' argument \n", CodeName,
	      CommaName);
      exit (-3);
    }
  i = 0;
  while (argv[parind + 2][i] != '=')
    {
      ArgName[i] = argv[parind + 2][i];
      i++;
    }
  ArgName[i] = '\0';
  i++;
  j = 0;
  while (argv[parind + 2][i] != '\0')
    {
      ArgData[j] = argv[parind + 2][i];
      i++;
      j++;
    }
  ArgData[j] = '\0';
  if (strcasecmp (ArgName, CodeName) == 0)
    {
      strcpy (StrCodes, ArgData);
    }
  else
    {
      if (strcasecmp (ArgName, CommaName) == 0)
	{
	  strcpy (StrCommas, ArgData);
	}
      else
	{
	  sz_log (0, "Invalid %s/%s parametr\n", CodeName, CommaName);
	  exit (1);
	}
    }

  i = 0;
  while (argv[parind + 3][i] != '=')
    {
      ArgName[i] = argv[parind + 3][i];
      i++;
    }
  ArgName[i] = '\0';
  i++;
  j = 0;
  while (argv[parind + 3][i] != '\0')
    {
      ArgData[j] = argv[parind + 3][i];
      i++;
      j++;
    }
  ArgData[j] = '\0';

  if (strcasecmp (ArgName, CodeName) == 0)
    {
      strcpy (StrCodes, ArgData);
    }
  else
    {
      if (strcasecmp (ArgName, CommaName) == 0)
	{
	  strcpy (StrCommas, ArgData);
	}
      else
	{
	  sz_log (0, "Invalid %s/%s parametr\n", CodeName, CommaName);
	  exit (1);
	}
    }

#ifndef DEBUG
  Initialize (LineNum);
#endif
  LineNum--;
#ifndef DEBUG
  if (!Simple)
    {
      if ((ValTab = (short *) shmat (ShmDes, (void *) 0, 0)) == (void *) -1)
	{
	  sz_log (0, "lmldmn: Can not attach shared segment");
	  exit (1);
	}
    }
#endif
  HowCodes = 0;
  p = strtok (StrCodes, ",");
  while (p)
    {
      Codes[HowCodes] = atoi (p);
      p = strtok (NULL, ",");
      HowCodes++;
    }

  HowCommas = 0;
  p = strtok (StrCommas, ",");
  while (p)
    {
      Commas[HowCommas] = atoi (p);
      p = strtok (NULL, ",");
      HowCommas++;
    }

  if (HowCodes != HowCommas)
    {
      sz_log (0, "lmldmn: mismatch number of params in %s or %s", CodeName,
	      CommaName);
      exit (1);
    }

  if ((linedes = OpenLine (linedev)) < 0)
    {
      sz_log (0, "lmldmn: invalid device");
      exit (1);
    }

  while (1 == 1)
    {
      sleep (5);		/* Odczytuj dane co 5 sekund */
      for (i = 0; i < HowCodes; i++)
	{
	  Query = MakeQuery (DeviceId, READ_FUN, Codes[i]);

	  SendQuery (linedes, Query);
	  free (Query);
	  Data = ReceiveData (linedes, &Param, HowCodes, Codes, Commas);	/* Odczytane dane ze sterownika INT */
	  DataBuffer[Param] = Data;	/* Dane musza byc buforowane bo czasami LUMELOWi cos odbija */
	  /* Opoznienie dobrane eksperymentalnie */
	  usleep (1000 * 300);
	}
      if (FirstTime == TRUE)
	{
	  FirstTime = FALSE;
	}
      else
	{
#ifndef DEBUG
	  if (!Simple)
	    {
	      Sem[0].sem_num = SEM_LINE + 2 * (LineNum - 1) + 1;
	      Sem[0].sem_op = 1;
	      Sem[1].sem_num = SEM_LINE + 2 * (LineNum - 1);
	      Sem[1].sem_op = 0;
	      Sem[0].sem_flg = Sem[1].sem_flg = SEM_UNDO;
	      semop (SemDes, Sem, 2);
	      for (i = 0; i < VTlen; i++)
		ValTab[i] = (short) DataBuffer[i];
	      Sem[0].sem_num = SEM_LINE + 2 * (LineNum - 1) + 1;
	      Sem[0].sem_op = -1;
	      Sem[0].sem_flg = SEM_UNDO;
	      semop (SemDes, Sem, 1);
	    }
#endif
	}
    }
  return 0;
}
