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
 * rsdmn.c
 * $Id$
 */

/*
 @description_start
 @class 2
 @devices Pozyton KWMS Energy Meters.
 @devices.pl Liczniki energii Pozyton KWMS.
 @description_end
 */

#define _IPCTOOLS_H_ /* Nie dolaczaj ipctools.h z szarp.h */
#define _HELP_H_
#define _ICON_ICON_

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <time.h>
#include <termio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <config.h>

#include "szarp.h"
#include "ipcdefines.h"

#define USAGE "kwmsdmn [s|d] conf_entry [device] [baud_rate] [stop_bits]"
#define VERSION1 "Internal PC COM daemon ver:2.1 rev:98.05.27"
#define DESCR1 "Daemon for communication with KWMS count device"
#define DESCR2 "send & reply queues"  
#define DESCR3 "rev 1.x cfg file protocol rules preserved"

#define MAXLINES 32 
#define MAX_PARS 100

#define DEFAULT_TTY "/dev/ttyS%02d"

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

static int LineDes;

static int LineNum;

static int SemDes;

static int ShmDes;

static int CheckSum;

static unsigned char UnitsOnLine;

static unsigned char RadiosOnLine;
 
static unsigned char DirectLine;

static unsigned char SpecialUnit;

static unsigned char Diagno=0;

static unsigned char Simple=0;

static int VTlen;

static short *ValTab;

static struct sembuf Sem[2];

static char parcookpat[81];

static char linedmnpat[81];

struct phUnit
 {
  unsigned char UnitCode;	/* kod sterownika */
  unsigned char RapId;          /* kod raportu */
  unsigned char SubId;          /* subkod raportu */
  unsigned char ParIn;		/* liczba zbieranych parametrow */
  unsigned char ParOut;		/* liczba ustawianych parametrów */
  unsigned short ParBase;	/* adres bazowy parametrow w tablicy linii */
  unsigned char SumPeriod;	/* okres podstawowego usredniania */
  unsigned char SumCnt;         /* ilosc pomiarow w buforze usredniania */
  unsigned char PutPtr;         /* wskaznik na biezacy element w buforze */
  short *Pars;			/* tablica parametrów do wys³ania */
  unsigned char *Sending;	/* znaczniki potwierdzeñ wys³ania parametru */ 
  int *AvgSum;			/* sumy buforow usrednien */
  short **AvgBuf;		/* bufory usrednien */
 };

typedef struct phUnit tUnit;

struct phRadio
 {
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
  unsigned char i,ii,iii,j,maxj;
  int val,val1,val2,val3,val4;
  char linedefname[81];
  FILE *linedef;

  libpar_init();
  libpar_readpar("", "parcook_path", parcookpat, sizeof(parcookpat), 1);
  libpar_readpar("", "linex_cfg", linedmnpat, sizeof(linedmnpat), 1);
  libpar_done();


  VTlen=0;
  sprintf(linedefname,"%s/line%d.cfg",linedmnpat, linenum);
  if ((linedef=fopen(linedefname,"r"))==NULL)
    ErrMessage(2,linedefname);
    
  if (fscanf(linedef,"%d\n",&val)>0)
   {
    RadiosOnLine=0;
    if (val>0)
     {
      UnitsOnLine=val;
      DirectLine=0;
     }
    else if (val<0)
     {
      UnitsOnLine=1;
      DirectLine=1;
      SpecialUnit=abs(val);
     }
    else
     {
      UnitsOnLine=1;
      DirectLine=1;
     }
   }
  else
   {
    fscanf(linedef,"%c%d\n", &ch, &val);
    if (ch!='R')
      exit(-1);
    else
      RadiosOnLine=(unsigned char)val;
   }
  if (Diagno)
   {
    printf("Cooking deamon for line %d\n",linenum);
    if (RadiosOnLine)
      printf("Allocating structures for %d radios\n", RadiosOnLine);
    else if (SpecialUnit)
      printf("Allocating structures for Special Unit code: %d\n", SpecialUnit);
    else
      printf("Allocating structures for %d units\n",UnitsOnLine);
   }
  maxj=RadiosOnLine?RadiosOnLine:1;
  if ((RadioInfo=(tRadio *)calloc(maxj,sizeof(tRadio)))==NULL)
    ErrMessage(1,NULL);
  for(j=0;j<maxj;j++)
   {
    if (RadiosOnLine)
     {
      fgets(RadioInfo[j].RadioName, 40, linedef);
      if ((chptr=strrchr(RadioInfo[j].RadioName, '\n'))!=NULL)
        *chptr=0;
      fscanf(linedef, "%d\n", &val);
      RadioInfo[j].NumberOfUnits=UnitsOnLine=(unsigned char)val;
      if (Diagno)
        printf("Radio <%s> drives %d units\n", RadioInfo[j].RadioName,
	        UnitsOnLine);
     }  
    if ((UnitsInfo=(tUnit *)calloc(UnitsOnLine,sizeof(tUnit)))==NULL)
      ErrMessage(1,NULL);
    for(i=0;i<UnitsOnLine;i++)
     { 
      fscanf(linedef,"%c %u %u %u %u %u\n",&UnitsInfo[i].UnitCode,
             &val,&val1,&val2,&val3,&val4);
      UnitsInfo[i].RapId=(unsigned char)val;
      UnitsInfo[i].SubId=(unsigned char)val1;
      UnitsInfo[i].ParIn=(unsigned char)val2;
      UnitsInfo[i].ParOut=(unsigned char)val3;
      UnitsInfo[i].SumPeriod=(unsigned char)val4;
      UnitsInfo[i].SumCnt=0;
      UnitsInfo[i].PutPtr=0;
      UnitsInfo[i].ParBase=VTlen;
      VTlen+=val2;
      if (Diagno)
        printf("Unit %d: code=%c %d %d, parameters: in=%d, out=%d, base=%d, period=%d\n",
               i,UnitsInfo[i].UnitCode, UnitsInfo[i].RapId, UnitsInfo[i].SubId,
               UnitsInfo[i].ParIn,UnitsInfo[i].ParOut,
               UnitsInfo[i].ParBase, UnitsInfo[i].SumPeriod);
      if ((UnitsInfo[i].Pars=(short *)
           calloc(UnitsInfo[i].ParOut,sizeof(short)))==NULL)
        ErrMessage(1,NULL);
      if ((UnitsInfo[i].Sending=(unsigned char *)
           calloc(UnitsInfo[i].ParOut,sizeof(unsigned char)))==NULL)
        ErrMessage(1,NULL);
      if ((UnitsInfo[i].AvgSum=(int *)
           calloc(UnitsInfo[i].ParIn,sizeof(int)))==NULL)
        ErrMessage(1,NULL);
      if ((UnitsInfo[i].AvgBuf=(short **)
           calloc(UnitsInfo[i].ParIn,sizeof(short *)))==NULL)
        ErrMessage(1,NULL);
      for(ii=0;ii<UnitsInfo[i].ParOut;ii++)
       {
        UnitsInfo[i].Pars[ii]=SZARP_NO_DATA;
        UnitsInfo[i].Sending[ii]=0;
       }
      for(ii=0;ii<UnitsInfo[i].ParIn;ii++)
       {
        if ((UnitsInfo[i].AvgBuf[ii]=(short *)
             calloc(UnitsInfo[i].SumPeriod,sizeof(short)))==NULL)
          ErrMessage(1,NULL);
        for(iii=0;iii<UnitsInfo[i].SumPeriod;iii++)
          UnitsInfo[i].AvgBuf[ii][iii]=SZARP_NO_DATA;
       }
     }
    RadioInfo[j].UnitsInfo=UnitsInfo;
   } 
  if (Simple)
    return;

  printf("Looking for segment: key %d\n", ftok(linedmnpat,linenum));
  if ((ShmDes=shmget(ftok(linedmnpat,linenum),
       sizeof(short)*VTlen,00600))<0)
    ErrMessage(3,"rsdmn");
  if ((SemDes=semget(ftok(parcookpat,SEM_PARCOOK),SEM_LINE+2*linenum,00600))<0)
    ErrMessage(5,"parcook sem");
  if ((MsgSetDes=msgget(ftok(parcookpat,MSG_SET), 00666))<0)
    ErrMessage(6, "parcook set");
  if ((MsgRplyDes=msgget(ftok(parcookpat,MSG_RPLY), 00666))<0)
    ErrMessage(6, "parcook rply");
 }
   

static int OpenLine(char *line, int baud, int stopb)
 {
  int linedes;
  struct termio rsconf;
  linedes=open(line,O_RDWR|O_NDELAY|O_NONBLOCK);
  if (linedes<0)
    return(-1);
  ioctl(linedes,TCGETA,&rsconf);
  rsconf.c_iflag=0;
  rsconf.c_oflag=0;
  switch(baud)
   {
    case 2400:rsconf.c_cflag=B2400;
              break;
    case 9600:rsconf.c_cflag=B9600;
              break;
    case 19200:
      default:rsconf.c_cflag=B19200;
   }
  switch(stopb)
   {
    case 2:rsconf.c_cflag|=CSTOPB;
	   break;
    case 1:
   default:break;
   }
  rsconf.c_cflag|=CS8|CLOCAL|CREAD;
  rsconf.c_lflag=0;
  rsconf.c_cc[4]=0;
  rsconf.c_cc[5]=0;
  ioctl(linedes,TCSETA,&rsconf);
  return(linedes);
 }

static int ReadLine(char *ss)
 {
  short i;
  for(i=0;read(LineDes,&ss[i],1)==1;i++)
   {
    CheckSum+=(uint)ss[i];
    if (ss[i]<32 && ss[i]!=1) 
     {
      ss[i]=0;
      return(i);
     }
    else
      if (ss[i]==1)
           ss[i] += 48 ; /* konwersja znaku z 1 na '1'*/
   }
  ss[i]=0;
  if (i==0)
    return(-1);
  else 
    return(i);
 }

void ReadSpecial(unsigned char unit)
 {
  static float Counters[13][3];
  static unsigned char blinker;
  struct tm at;
  static time_t ltim;
  time_t atim, diftim;
  short ind, vals[MAX_PARS];
  float fval;
  char ss[129];
  unsigned char i, ok, torm;
  atim = 0;
  diftim = 0;
  while(ReadLine(ss)>=0)
   {
    at.tm_mon=15;
    sscanf(ss, "Stan wagi w dniu %d-%d-%d o godz.%d:%d:%d", &at.tm_year,
           &at.tm_mon, &at.tm_mday, &at.tm_hour, &at.tm_min, &at.tm_sec);
    if (at.tm_mon!=15)
     {
      if (Simple)
        printf("Current date %02d-%02d-%02d and time %02d:%02d:%02d captured\n",
                at.tm_year, at.tm_mon, at.tm_mday, at.tm_hour, at.tm_min,
                at.tm_sec);
      at.tm_mon-=1;
      atim=mktime(&at);
      diftim=atim-ltim;
      if (Simple)
        printf("Counting window %ld seconds\n", diftim);
      break;
     }
   }
  for(i=0;i<13;i++)
    Counters[i][blinker]=-1.0;
  while(ReadLine(ss)>=0)
   {
    if (Simple&&strlen(ss)>0) 
      printf("%s\n", ss); 
    ind=0;
    sscanf(ss, "    -  Licznik  nr %hd %f", &ind, &fval);
    if (ind>0&&ind<14)
     {
/*      printf("Couter %d captured value %0.2f\n", ind, fval); */
      Counters[ind-1][blinker]=fval;  
/*      Counters[ind-1][blinker]=Counters[ind-1][!blinker]+(float)ind/100.0; */ 
     }
   }
  ok=1;
  for(i=0;i<13;i++)
   {
/*    printf("Counters[%d][%d] - Counters[%d][%d] = %0.2f - %0.2f \n",
           i, blinker, i, !blinker, Counters[i][blinker], 
           Counters[i][!blinker]);  */
    Counters[i][2]=ltim==0?0.0:
                   (Counters[i][blinker]-Counters[i][!blinker])/diftim*3600.0;
    if (Simple)
      printf("Flow[%d]=%0.2f T/h\n", i, Counters[i][2]);
    ok&=(Counters[i][blinker]>-0.5);
   }
  if (ok)
   {
    if (Simple)
      printf("Ok.\n");
    blinker=!blinker;
    ltim=atim;
   }
  torm=UnitsInfo[unit].AvgBuf[0][UnitsInfo[unit].PutPtr]!=SZARP_NO_DATA; 
  for(i=0;i<UnitsInfo[unit].ParIn;i++)
   {
    if (torm)
      UnitsInfo[unit].AvgSum[i]-=
        UnitsInfo[unit].AvgBuf[i][UnitsInfo[unit].PutPtr];
    if (ok)
     {
      vals[i]=rint(Counters[i][2]*100.0);
      UnitsInfo[unit].AvgSum[i]+=vals[i];
     }
    else
      vals[i]=SZARP_NO_DATA;
    UnitsInfo[unit].AvgBuf[i][UnitsInfo[unit].PutPtr]=vals[i];
   }
  UnitsInfo[unit].PutPtr=(unsigned char)(UnitsInfo[unit].PutPtr+1)%
                         UnitsInfo[unit].SumPeriod;
  if (torm)
    UnitsInfo[unit].SumCnt--;
  if (ok)
    UnitsInfo[unit].SumCnt++;
  if (Simple)
    return;
  Sem[0].sem_num=SEM_LINE+2*(LineNum-1)+1;
  Sem[0].sem_op=1;
  Sem[1].sem_num=SEM_LINE+2*(LineNum-1);
  Sem[1].sem_op=0;
  Sem[0].sem_flg=Sem[1].sem_flg=SEM_UNDO;
  semop(SemDes,Sem,2); 
  if (UnitsInfo[unit].SumCnt>0)
    for(i=0;i<UnitsInfo[unit].ParIn;i++)
      ValTab[UnitsInfo[unit].ParBase+i]=(short)
        (UnitsInfo[unit].AvgSum[i]/(int)UnitsInfo[unit].SumCnt);
  else
    for(i=0;i<UnitsInfo[unit].ParIn;i++)
      ValTab[UnitsInfo[unit].ParBase+i]=SZARP_NO_DATA;
  if (Diagno)
    for(i=0;i<(ushort)VTlen;i++)
     {
      printf("ValTab[%d]=%d",i,ValTab[i]);
      if (i>=UnitsInfo[unit].ParBase&&i<(ushort)(UnitsInfo[unit].ParBase+
          UnitsInfo[unit].ParIn))
        printf("\t\tval[%d]=%d",i-UnitsInfo[unit].ParBase,
               vals[i-UnitsInfo[unit].ParBase]);
      printf("\n");
     }
  Sem[0].sem_num=SEM_LINE+2*(LineNum-1)+1;
  Sem[0].sem_op=-1;
  Sem[0].sem_flg=SEM_UNDO;
  semop(SemDes,Sem,1);
  if (Diagno)
   {
    for(i=0;i<UnitsOnLine;i++)
     {
      printf("Cnt%d:%d\t",i,UnitsInfo[i].SumCnt);
      printf("Put%d:%d\t",i,UnitsInfo[i].PutPtr);
     }
    printf("\n++++++++++++++++++++++++\n\n");
   }
 }

void ReadReport(unsigned char unit, char *chptr)  
 {
  char ss[129];
  short chks,vals[MAX_PARS];
  ushort i;
  unsigned char ok,torm,parset;
  tMsgSetParam msg;

  CheckSum=0;
  ok=(ReadLine(ss)>0);
  if (Simple)
    printf("ok1=%d\n", ok);
/* UWAGA */
/*  if (UnitsInfo[unit].RapId!=255) */
   {
/* UWAGA - pominiecie RapId */
/*    ok=(ok&&atoi(ss)==(int)UnitsInfo[unit].RapId); */
    if (Simple)
      printf("RapId: %s\n", ss);
    ok=(ok&&ReadLine(ss)>0);
/* UWAGA - pominiecie Subid */
/*    ok=(ok&&atoi(ss)==(int)UnitsInfo[unit].SubId); */
    if (Simple)
      printf("SubId: %s\n", ss);
    ok=(ok&&ReadLine(ss)>0); 
    ok=(ok&&ss[0]==UnitsInfo[unit].UnitCode);
    if (Simple)
      printf("FunId: %s\n", ss);
    ok=(ok&&ReadLine(ss)>0);    
   }
  for(i=0;i<UnitsInfo[unit].ParIn;i++)
   {
    ok=(ok&&ReadLine(ss)>0);
    vals[i]=atoi(ss);
    if (Simple)
      printf("vals[%d]=%d\n", i, vals[i]);
   }
  chks=CheckSum;
  ok=(ok&&ReadLine(ss)>0);
  ok=(ok&&chks==atoi(ss));
  if (Simple)
      printf("ok=%d\n\n", ok);
      
  torm=UnitsInfo[unit].AvgBuf[0][UnitsInfo[unit].PutPtr]!=SZARP_NO_DATA; 
  for(i=0;i<UnitsInfo[unit].ParIn;i++)
   {
    if (torm)
      UnitsInfo[unit].AvgSum[i]-=
        UnitsInfo[unit].AvgBuf[i][UnitsInfo[unit].PutPtr];
    if (ok)
      UnitsInfo[unit].AvgSum[i]+=vals[i];
    else
      vals[i]=SZARP_NO_DATA;
    UnitsInfo[unit].AvgBuf[i][UnitsInfo[unit].PutPtr]=vals[i];
   }
  UnitsInfo[unit].PutPtr=(unsigned char)(UnitsInfo[unit].PutPtr+1)%
                         UnitsInfo[unit].SumPeriod;
  if (torm)
    UnitsInfo[unit].SumCnt--;
  if (ok)
    UnitsInfo[unit].SumCnt++;
  if (Simple)
    return;
  Sem[0].sem_num=SEM_LINE+2*(LineNum-1)+1;
  Sem[0].sem_op=1;
  Sem[1].sem_num=SEM_LINE+2*(LineNum-1);
  Sem[1].sem_op=0;
  Sem[0].sem_flg=Sem[1].sem_flg=SEM_UNDO;
  semop(SemDes,Sem,2); 
  if (UnitsInfo[unit].SumCnt>0)
    for(i=0;i<UnitsInfo[unit].ParIn;i++)
      ValTab[UnitsInfo[unit].ParBase+i]=(short)
        (UnitsInfo[unit].AvgSum[i]/(int)UnitsInfo[unit].SumCnt);
  else
    for(i=0;i<UnitsInfo[unit].ParIn;i++)
      ValTab[UnitsInfo[unit].ParBase+i]=SZARP_NO_DATA;
  while(ReadLine(ss)>=0);
  if (Diagno)
    for(i=0;i<(ushort)VTlen;i++)
     {
      printf("ValTab[%d]=%d",i,ValTab[i]);
      if (i>=UnitsInfo[unit].ParBase&&i<(ushort)(UnitsInfo[unit].ParBase+
          UnitsInfo[unit].ParIn))
        printf("\t\tval[%d]=%d",i-UnitsInfo[unit].ParBase,
               vals[i-UnitsInfo[unit].ParBase]);
      printf("\n");
     }
  for(i=0;i<(ushort)UnitsInfo[unit].ParOut;i++)
    if (UnitsInfo[unit].Sending[i])
     {
      UnitsInfo[unit].Sending[i]=0;
      if (ok)
       {
        msg.type=(long)LineNum*256L+(long)UnitsInfo[unit].UnitCode; 
        msg.cont.param=i;
	msg.cont.value=UnitsInfo[unit].Pars[i];
        msgsnd(MsgRplyDes, &msg, sizeof(tSetParam), IPC_NOWAIT);
       }
     }
  unit=(unsigned char)(unit+1)%UnitsOnLine;
/* UWAGA */
/*  sprintf(chptr,"\x02Pn"); */
  sprintf(chptr,"\x11\x02Pn");
  parset=0;
  msg.type=(long)LineNum*256L+(long)UnitsInfo[unit].UnitCode;
  while(msgrcv(MsgSetDes, &msg, sizeof(tSetParam), msg.type, IPC_NOWAIT)==
        sizeof(tSetParam))
    if (msg.cont.param<UnitsInfo[unit].ParOut)
     {
      sprintf(ss, "%d,%d,", msg.cont.param, msg.cont.value);
      strcat(chptr, ss);
      UnitsInfo[unit].Sending[i]=1;
      parset=1; 
     }
  chptr[strlen(chptr)-parset]=0;
  strcat(chptr, "\x03");  			/* mo¿e dodaæ NL */
  Sem[0].sem_num=SEM_LINE+2*(LineNum-1)+1;
  Sem[0].sem_op=-1;
  Sem[0].sem_flg=SEM_UNDO;
  semop(SemDes,Sem,1);
  if (Diagno)
   {
    for(i=0;i<(int)UnitsOnLine;i++)
     {
      printf("Cnt%d:%d\t",i,UnitsInfo[i].SumCnt);
      printf("Put%d:%d\t",i,UnitsInfo[i].PutPtr);
     }
    printf("\n++++++++++++++++++++++++\n\n");
   }
 }

void Usage()
 {
  printf("\n%s\n", VERSION1);
  printf("Usage:    %s\n", USAGE);
  printf("Features: %s\n", DESCR1);
  printf("          %s\n", DESCR2);
  printf("          %s\n", DESCR3);
  exit(1);
 }


int main(int argc,char *argv[])
 {
  int baud,stopb;
  ushort i;
  unsigned char parind;
/* UWAGA */
/*  char obuf[129]="\x02Pn\x03\n";   */
  char obuf[129]="\x11\x02Pn\x03\n";  
  char rbuf[5]="\x11P\x0a";
  char wbuf[8]="E0\r";
  char linedev[30];

  libpar_read_cmdline(&argc, argv);
  if (argc<2)
    Usage();
  Diagno=(argv[1][0]=='d');
  Simple=(argv[1][0]=='s');
  if (Simple)
    Diagno=1;
  if (Diagno)
    parind=2; 
  else
    parind=1;
  if (argc<(int)parind+1)
    Usage();
  baud=19200;
  stopb=1;
  LineNum=atoi(argv[parind]);
  sprintf(linedev, DEFAULT_TTY, LineNum-1);
  if (argc>(int)parind+1)
    strcpy(linedev, argv[parind+1]);
  if (argc>(int)parind+2)
    baud=atoi(argv[parind+2]);
  if (argc>(int)parind+3)
    stopb=atoi(argv[parind+3]);
  Initialize(LineNum);
/*  if ((LineDes=OpenLine(linedev))<0)
    ErrMessage(2,linedev); */
/*  if ((memdes=open("/dev/mem",O_RDONLY))==-1)
    ErrMessage(2,"/dev/mem"); */
  LineNum--;
/*  ctrmemaddr=(LineNum<8?SI_MOD1:LineNum<16?SI_MOD2:LineNum<24?SI_MOD3:
              LineNum<32?SI_MOD4:LineNum<40?SI_MOD5:LineNum<48?SI_MOD6:
              LineNum<56?SI_MOD7:SI_MOD8)+0x300*(LineNum<32?LineNum:
              LineNum-32)+0x0c;  
  lseek(memdes,ctrmemaddr,SEEK_SET); */
  if (!Simple)
   {
    if ((ValTab=(short *)shmat(ShmDes,(void *)0,0))==(void *)-1)
     {
      printf("Can not attach shared segment\n");
      exit(-1);
     }
   }

  while(1)
   {
    for(i=0;i<UnitsOnLine;i++)
     {
      if ((LineDes=OpenLine(linedev, baud, stopb))<0)
       ErrMessage(2,linedev);

/*      if (!RadiosOnLine)
       {
        bread=TIOCM_RTS;
        ioctl(LineDes,TIOCMBIS,&bread); 
       }					*/
      if (SpecialUnit==1)
       {
        if (Simple)
          printf("Probing:%s\n", wbuf);
        write(LineDes,wbuf,strlen(wbuf));
       } 
      else if (!DirectLine)
       {
/* UWAGA */
/*        obuf[2]=UnitsInfo[i].UnitCode; */
        obuf[3]=UnitsInfo[i].UnitCode;
        if (Simple)
          printf("Sending:%s\n", obuf);
        write(LineDes,obuf,strlen(obuf));
       }
      else
       {
        write(LineDes,rbuf,3); 
        if (Simple)
          printf("Sending directly:%s\n", rbuf);
       }
/*      do
       {
        read(memdes,mtab,2);
        lseek(memdes,ctrmemaddr,SEEK_SET);
       }
      while(mtab[1]!=mtab[0]);	*/
/*      if (!RadiosOnLine)
       {
        ioctl(LineDes,TIOCMBIC,&bread);
        ioctl(LineDes,TCFLSH,0); 
       }				*/
      if (RadiosOnLine)
        sleep(20);
       else
        sleep(5);
      if (SpecialUnit)
        ReadSpecial(i);
      else
        ReadReport(i,obuf);  

      close(LineDes);
     } 
   }
 }

