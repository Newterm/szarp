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
 * SZARP 2.0
 * parcook
 * lbdmn.c
 * Wersja $Id$
 */
/*
 @description_start
 @class 2
 @devices Label LB-746 wind meter.
 @devices.pl Wiatromierz Label LB-746.
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
#include "libpar.h"
#include "msgerror.h"
#include "ipcdefines.h"

#define LB_MESSAGE_SIZE 15
 /* 1 + 1 + 4 + 3 + 4 + 1 + 1*/

static int LineDes;

static int LineNum;

static int SemDes;

static int ShmDes;

static unsigned char Diagno=0;

static unsigned char Simple=0;

static int VTlen;

static short *ValTab;

static struct sembuf Sem[2];

static char parcookpat[81];

static char linedmnpat[81];

/*
void ErrMessage(int i, char *s)
{
	fprintf(stderr, "%d - %s\n", i, s);
}
*/

void Initialize(unsigned char linenum)
 {
  libpar_init();
  libpar_readpar("", "parcook_path", parcookpat, sizeof(parcookpat), 1);
  libpar_readpar("", "linex_cfg", linedmnpat, sizeof(linedmnpat), 1);
  libpar_done();

  VTlen=2;
  if (Diagno)
    printf("Cooking deamon for line %d - LB-746 deamon\n",linenum);
  if (Diagno)
    printf("Unit 1: no code/rapid/subid, parameters: in=2, no out, period=1\n");
  if (Simple)
    return;
  if ((ShmDes=shmget(ftok(linedmnpat,linenum),sizeof(short)*VTlen,00600))<0)
    ErrMessage(3,"linedmn");
  if ((SemDes=semget(ftok(parcookpat,SEM_PARCOOK),SEM_LINE+2*linenum,00600))<0)
    ErrMessage(5,"parcook sem");
 }

int SetIOTimeout(int linedes, int time, int min)
{
 struct termio rsconf;

  ioctl(linedes,TCGETA,&rsconf);
  rsconf.c_cc[VTIME]=time; /* czekaj co najmniej 5 sekund */
  rsconf.c_cc[VMIN]=min;
  ioctl(linedes,TCSETA,&rsconf);
  return 0;
}   

static int OpenLine(char *line)
 {
  int linedes;
  struct termio rsconf;
  linedes=open(line, O_RDONLY|O_NOCTTY );
  if (linedes<0)
    return(-1);
  ioctl(linedes,TCGETA,&rsconf);
  rsconf.c_iflag=0;
  rsconf.c_oflag=0;
  rsconf.c_cflag=B300|CS7|CLOCAL|CREAD;
  rsconf.c_lflag=0;
  ioctl(linedes,TCSETA,&rsconf);
  return(linedes);
 }

 /* zwraca dlugosc wczytanej komendy */
/* BUG? Tak naprawde to nie wiem co ma zwracaæ, zwraca³ losowo... */
int ReadLBOutput(int sock, char *buf)
{
  int i,j;

  errno = 0;
  memset(buf, 0, LB_MESSAGE_SIZE);
  SetIOTimeout(sock, 50, 0); /* ustaw 5 sekund oczekiwania */
  j = i = read(sock, buf, 1);
  if (i <= 0) return -1;
  SetIOTimeout(sock, 1, LB_MESSAGE_SIZE-1); /* jak juz przeczyta 0 to nie czekaj */
  i = read(sock, buf, LB_MESSAGE_SIZE-1);
  if (i <= 0) return -1;
  for (i = 0 ; i < LB_MESSAGE_SIZE ; i++)
  	buf[i] &= 0x3f; /* bit parzystosci */
  buf[LB_MESSAGE_SIZE - 1] = 0; /* obetnij na znaku #13 */
  if ((buf[0] < '0') || (buf[0] > '7'))
   buf[0] = '7'; /* ustaw control-char na bledy */
  if (errno) return -1;
  return j;
}

void ParseLBMessage(char *buf, 
	unsigned char *control, int *_serial, int *_azimuth, int *_speed)
{
 char control_char = 0;
 char serial[5] = "";
 char speed[5] = "";
 char azimuth[4] = "";
 
 control_char = buf[0] - '0';
 strncpy(serial, buf + 1, 4); serial[4] = 0;
 strncpy(azimuth, buf + 1 + 4, 3); azimuth[3] = 0;
 strncpy(speed, buf + 1 + 4 + 3, 4); speed[4] = 0;
 
 *control = control_char;
 *_serial = (int) buf[2] - (int)'0' + 
 	    16 * ((int)buf[1] - (int)'0') +
 	    256 * ((int)buf[4] - (int)'0') + 
 	    4096 * ((int)buf[3] - (int)'0');

 *_azimuth = atoi(azimuth);
 if (*_azimuth < 0) *_azimuth = 0;
 if (*_azimuth > 360) *_azimuth = 360;
 *_speed = atoi(speed); 
}

void ReadLB_746(void)  
 {
  char ss[LB_MESSAGE_SIZE];
  short vals[2];
  unsigned char ctrl_char;
  int serial, azimuth, speed, i;

  vals[0] = vals[1] = SZARP_NO_DATA;

  if (ReadLBOutput(LineDes, ss) > 0)
    {
     if (Diagno)
       printf("content: %s\n", ss);

     ParseLBMessage(ss, &ctrl_char, &serial, &azimuth, &speed);

     if ((serial > 0) && (Diagno))
       printf("numer seryjny: %d\n", serial);
       
     if (ctrl_char & 1)
       {
        azimuth = SZARP_NO_DATA;
        if (Diagno)
          printf("Blad pomiaru kierunku wiatru\n");
       }
     if (ctrl_char & 2)
       {
        speed = SZARP_NO_DATA;
        if (Diagno)
          printf("Blad pomiaru predkosci wiatru\n");
       }
     if (ctrl_char & 4)
       {
        azimuth = speed = SZARP_NO_DATA;
        if (Diagno)
          printf("Blad kalibracji\n");
       }
       
     if (Diagno && (azimuth != SZARP_NO_DATA))
       printf("Azymut = %d\n", azimuth);
     if (Diagno && (speed != SZARP_NO_DATA))
       printf("Predkosc wiatru = %d.%d m/s\n", speed / 10, speed % 10);

	vals[0] = azimuth;
	vals[1] = speed;

    }

   else
    {
     vals[0] = vals[1] = SZARP_NO_DATA;
     if (Diagno)
       printf("Brak danych\n");
    }
    
/* UWAGA - usrednianie */
/* oryginalnie wszystko fajnie pod warunkiem, ze sterownik nie przesyla
  dla jakiegokolwiek parametru wartosci SZARP_NO_DATA - jesli tak, to trzeba 
  trzymac takze tablice SumCnt dla kazdego parametru */
  if (Simple)
    return;
  Sem[0].sem_num=SEM_LINE+2*(LineNum-1)+1;
  Sem[0].sem_op=1;
  Sem[1].sem_num=SEM_LINE+2*(LineNum-1);
  Sem[1].sem_op=0;
  Sem[0].sem_flg=Sem[1].sem_flg=SEM_UNDO;
  semop(SemDes,Sem,2); 
  for(i = 0 ; i < VTlen ; i++)
    ValTab[i] = (short)vals[i];
  Sem[0].sem_num=SEM_LINE+2*(LineNum-1)+1;
  Sem[0].sem_op=-1;
  Sem[0].sem_flg=SEM_UNDO;
  semop(SemDes,Sem,1);
 }

void Usage(char *name)
 {
  printf("LB-746 driver; can be used on both COM and Specialix ports\n");
  printf("Usage:\n");
  printf("      %s [s|d] <n> <port>\n", name);
  printf("Where:\n");
  printf("[s|d] - optional simple (s) or diagnostic (d) modes\n");
  printf("<n> - line number (necessary to read line<n>.cfg file)\n");
  printf("<port> - full port name, eg. /dev/term/01 or /dev/term/a12\n");
  printf("Comments:\n");
  printf("No parameters are taken from line<n>.cfg (base period = 1)\n");
  printf("Port parameters: 300 baud, 7N1; signals required: TxD, RxD, GND\n");
  printf("Data format: records sent automatically (about 2 per second) containing:\n");
  printf("<0x00><c><i><i><i><i><a><a><a><v><v><v><v><0x13>\n");
  printf("Where <c>,..,<s> are data bytes, all with parity set on oldest bit:\n");
  printf("<c> - control byte:\n");
  printf("  <c> & 0x01 - wrong azimuth measurement\n");
  printf("  <c> & 0x02 - wrong speed measurement\n");
  printf("  <c> & 0x04 - wrong calibration\n");
  printf("<i><i><i><i> - coded device serial number\n");
  printf("<a><a><a> - azimuth in degrees\n");
  printf("<v><v><v><v> - velocity in 0.1 m/s\n");
  exit(1);
 }


int main(int argc,char *argv[])
 {
  unsigned char parind;
  char linedev[30];

  libpar_read_cmdline(&argc, argv);
  if (argc<3)
    Usage(argv[0]);
  Diagno=(argv[1][0]=='d');
  Simple=(argv[1][0]=='s');
  if (Simple)
    Diagno=1;
  if (Diagno)
    parind=2; 
  else
    parind=1;
  if (argc<(int)parind+2)
    Usage(argv[0]);
  LineNum=atoi(argv[parind]);
  strcpy(linedev, argv[parind+1]);
  Initialize(LineNum);
  LineNum--;
  if (!Simple)
   {
    if ((ValTab=(short *)shmat(ShmDes,(void *)0,0))==(void *)-1)
     {
      printf("Can not attach shared segment\n");
      exit(-1);
     }
   }

    if ((LineDes=OpenLine(linedev))<0) {
      ErrMessage(2,linedev);
      perror("");
    }

  while(1)
   {
    ReadLB_746();  
   }

    close(LineDes);

 }


