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
 * linedmn.c
 */

/*
 * Modified by Codematic 13.05.1998
 * obsluga libpar
 */

/*
 * main() przyjmuje 3 parametry:
 * 	1: numer linii (integer)
 * 	2: nazwa urzadzenia (string)
 * 	3(opcjonalnie): znak ('d' lub 's')
 */ 	

/*
 @description_start
 @class 2
 @devices Z-Elektronik PLC using radio modems.
 @devices.pl Sterowniki PLC Z-Elektronik po radioliniach.
 @protocol Z-Elektronik.
 @comment This daemon is obsolete, not used anymore and scheduled for removing.
 @comment.pl Jest to przestarza³y, nieu¿ywany demon, przeznaczony do usuniêcia.
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
#include <termio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "ipcdefines.h"

#include <config.h>
#include "szarp.h"

#define MAXLINES 48 

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

#define MAX_PARS 100

static int LineDes;

static int LineNum;

static int SemDes;

static int ShmDes;

static int CheckSum;

static unsigned char UnitsOnLine;

static unsigned char RadiosOnLine;
 
static int DirectLine;

static unsigned char Diagno=0;

static unsigned char Simple=0;

static int VTlen;

static short *ValTab;

static struct sembuf Sem[2];

static char parcookpat[81];

static char linedmnpat[81];

static char MyName[]="Hostxx";

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
  long *rtype;			/* tablica ident komunikatów potwierdzeñ */
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
      if ((UnitsInfo[i].rtype=(long *)
           calloc(UnitsInfo[i].ParOut,sizeof(long)))==NULL)
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
  if ((ShmDes=shmget(ftok(linedmnpat,linenum),
       sizeof(short)*VTlen,00600))<0)
    ErrMessage(3,"linedmn");
  if ((SemDes=semget(ftok(parcookpat,SEM_PARCOOK),SEM_LINE+2*linenum,00600))<0)
    ErrMessage(5,"parcook sem");
  if ((MsgSetDes=msgget(ftok(parcookpat,MSG_SET), 00666))<0)
    ErrMessage(6, "parcook set");
  if ((MsgRplyDes=msgget(ftok(parcookpat,MSG_RPLY), 00666))<0)
    ErrMessage(6, "parcook rply");
 }
   

static int OpenLine(char *line)
 {
  int linedes;
  struct termio rsconf;
  linedes=open(line,O_RDWR|O_NDELAY|O_NONBLOCK);
  if (linedes<0)
    return(-1);
  ioctl(linedes,TCGETA,&rsconf);
  rsconf.c_iflag=0;
  rsconf.c_oflag=0;
  if (DirectLine)
    rsconf.c_cflag=B2400|CS8|CSTOPB|CLOCAL|CREAD;
  else
    rsconf.c_cflag=B9600|CS8|CLOCAL|CREAD;
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
    if (ss[i]<32) 
     {
      ss[i]=0;
      return(i);
     }
   }
  ss[i]=0;
  if (i==0)
    return(-1);
  else 
    return(i);
 }

void PrepareCommand(unsigned char unit, char *chptr)
 {
  char ss[129];
  tMsgSetParam msg;
  unsigned char parset;
  ushort i;

  sprintf(chptr,"\x02Pn");
  parset=0;
  msg.type=(long)LineNum*256L+(long)UnitsInfo[unit].UnitCode;
  if (Diagno||Simple) 
    printf("searching for message:%ld\n", msg.type);
  while(msgrcv(MsgSetDes, &msg, sizeof(tSetParam), msg.type, IPC_NOWAIT)==
        sizeof(tSetParam))
    if (msg.cont.param<UnitsInfo[unit].ParOut)
     {
      UnitsInfo[unit].Sending[msg.cont.param]=msg.cont.retry;
      UnitsInfo[unit].Pars[msg.cont.param]=msg.cont.value;
      UnitsInfo[unit].rtype[msg.cont.param]=msg.cont.rtype;
     }
  for(i=0;i<(ushort)UnitsInfo[unit].ParOut;i++)
    if (UnitsInfo[unit].Sending[i])
     {
      sprintf(ss, "%d,%d,", i, UnitsInfo[unit].Pars[i]);
      if (strlen(ss)+strlen(chptr)<120)
        strcat(chptr, ss);
      parset=1;
     }
  chptr[strlen(chptr)-parset]=0;
  strcat(chptr, "\x03");                        /* mo¿e dodaæ NL */
 } 

void ReadReport(unsigned char unit)  
 {
  char ss[129];
  short chks,vals[MAX_PARS];
  ushort i;
  unsigned char ok,torm;
  tMsgSetParam msg;
  int res;

  CheckSum=0;
  ok=(ReadLine(ss)>=0);
  if (Simple)
    printf("Got: %s\n", ss);
  if (UnitsInfo[unit].RapId!=255)
   {
    while(atoi(ss)!=(int)UnitsInfo[unit].RapId&&ok) 
     {
      CheckSum=0;
      ok=(ok&&ReadLine(ss)>=0);
      if (Simple&&ok)
        printf("Got: %s\n", ss);
     }
    ok=(ok&&ReadLine(ss)>=0);
    if (Simple&&ok)
      printf("Got: %s\n", ss);
    ok=(ok&&atoi(ss)==(int)UnitsInfo[unit].SubId);
    ok=(ok&&ReadLine(ss)>0); 
    if (Simple&&ok)
      printf("Got: %s\n", ss);
    ok=(ok&&ss[0]==UnitsInfo[unit].UnitCode); 
    ok=(ok&&ReadLine(ss)>0);
    if (Simple&&ok)
      printf("Got: %s\n", ss);
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
     {
      vals[i]=vals[i]==SZARP_NO_DATA?vals[i]=-32767:vals[i];
      UnitsInfo[unit].AvgSum[i]+=vals[i];
     }
    else
      vals[i]=SZARP_NO_DATA;
    UnitsInfo[unit].AvgBuf[i][UnitsInfo[unit].PutPtr]=vals[i];
   }
  UnitsInfo[unit].PutPtr=(unsigned char)(UnitsInfo[unit].PutPtr+1)%
                         UnitsInfo[unit].SumPeriod;

  while(ReadLine(ss)>=0)
    if (Simple)
      printf("Garbage: %s\n", ss);
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
  semop(SemDes,Sem,1);

  for(i=0;i<(ushort)UnitsInfo[unit].ParOut;i++)
    if (UnitsInfo[unit].Sending[i])
     {
      if (ok)
       {
        if (UnitsInfo[unit].Sending[i]!=255)
         {
          msg.type=UnitsInfo[unit].rtype[i];
          msg.cont.param=i;
	  msg.cont.value=UnitsInfo[unit].Pars[i];
          msg.cont.rtype=(long)LineNum*256L+(long)UnitsInfo[unit].UnitCode;
          msg.cont.retry=UnitsInfo[unit].Sending[i];

          res=msgsnd(MsgRplyDes, &msg, sizeof(tSetParam), IPC_NOWAIT);
          if (Diagno)
           {
            printf("Sending message %ld confirming %ld setting param %d to %d\n", msg.type, msg.cont.rtype, msg.cont.param, msg.cont.value);
            printf("Send result %d with errno %d\n", res, errno);
           }
         }
        UnitsInfo[unit].Sending[i]=0;
        UnitsInfo[unit].Pars[i]=SZARP_NO_DATA;
       }
      else
       {
        if (--UnitsInfo[unit].Sending[i]==0)
         {
          msg.type=UnitsInfo[unit].rtype[i];
          msg.cont.param=i;
          msg.cont.value=UnitsInfo[unit].Pars[i];
          msg.cont.rtype=(long)LineNum*256L+(long)UnitsInfo[unit].UnitCode;
          msg.cont.retry=UnitsInfo[unit].Sending[i];
          if (Diagno)
             {
          printf("Sending message %ld rejecting %ld setting param %d to %d\n",
               msg.type, msg.cont.rtype, msg.cont.param, msg.cont.value);
          printf("Send result %d with errno %d\n",
               msgsnd(MsgRplyDes, &msg, sizeof(tSetParam), IPC_NOWAIT),
               errno);
             }
         }
       }
     }
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

unsigned char ConnectRadio(unsigned char unit)
 {
  unsigned char state=0;
  unsigned char retry=0;
  char cmdbuf[256];

  while(1)
   {
    if (Simple)
      printf("state: %d\n", state);
    switch(state)
     { 
      case 0:if (++retry>1)
	       return(0);
             sprintf(cmdbuf,"");
             write(LineDes, cmdbuf, strlen(cmdbuf));
             sleep(5);
             sprintf(cmdbuf,"\r");
	     write(LineDes, cmdbuf, strlen(cmdbuf));
	     sleep(1);
             state=1;
             break;
      case 1:while(ReadLine(cmdbuf)>=0)
              {
	       if (Simple)
                 printf("cmdbuf->%s\n", cmdbuf);
               if (strfind(cmdbuf, "cmd:")!=0)
 		{
		 state=2;
		 break;
		}
              }
 	     if (state!=2)
	       state=0;
	     break;
      case 2:sprintf(cmdbuf,"disconnect\r");
	     write(LineDes, cmdbuf, strlen(cmdbuf));
	     sleep(5);
	     state=3;
	     break;
      case 3:while(ReadLine(cmdbuf)>=0)
              {
	       if (Simple)
                 printf("cmdbuf->%s\n", cmdbuf);
	       if (strfind(cmdbuf, "cmd:")!=0)
  		{
		 state=4;	 
		 break;
		}
              }
	     if (state!=4)
	       state=0;
	     break;
      case 4:sprintf(cmdbuf,"restart\r");
	     write(LineDes, cmdbuf, strlen(cmdbuf));
	     sleep(3);
	     state=5;
	     break;
      case 5:while(ReadLine(cmdbuf)>=0)
              {
	       if (Simple)	
                 printf("cmdbuf->%s\n", cmdbuf);
               if (strfind(cmdbuf, "cmd:")!=0)
                {
                 state=6;
                 break;
                }
              }
             if (state!=6)
               state=0;
             break;
      case 6:sprintf(cmdbuf,"mycall %s\r", MyName);
	     write(LineDes, cmdbuf, strlen(cmdbuf));
	     sleep(1);
	     state=7;
	     break;
      case 7:while(ReadLine(cmdbuf)>=0)
	      {
	       if (Simple)
	         printf("cmdbuf->%s\n", cmdbuf);
               if (strfind(cmdbuf, "cmd:")!=0)
                {
                 state=8;
                 break;
                }
   	      }
             if (state!=8)
               state=0;
             break;
      case 8:sprintf(cmdbuf, "connect %s\r", RadioInfo[unit].RadioName);
	     write(LineDes, cmdbuf, strlen(cmdbuf));
             sleep(9);
             state=9;
             break;
      case 9:while(ReadLine(cmdbuf)>=0)
	      {
	       if (Simple)
                 printf("cmdbuf->%s\n", cmdbuf);
               if (strfind(cmdbuf, "CONNECTED to")!=0)
                {
                 state=10;
                 break;
                }
	      }
             if (state!=10)
               state=0;
             break;
      case 10:sprintf(cmdbuf, "trans\r");
	      write(LineDes, cmdbuf, strlen(cmdbuf));
              sleep(1);
 	      while(ReadLine(cmdbuf)>=0);
	      return(1); 
              break; 
      default:break;
     }
   }
 }

int main(int argc,char *argv[])
 {
  int bread,memdes;
  unsigned int z;
  ushort i;
  unsigned char curradio=0; 
  char obuf[129]="\x02Pn\n";  
  char rbuf[4]="\x11P\x0a";
  char linedev[30];
  ulong ctrmemaddr;
  
  libpar_read_cmdline(&argc, argv);

  if (argc > 3) {
   Diagno=(argv[3][0]=='d'); 
   Simple=(argv[3][0]=='s');
  }

  if (Simple)
    Diagno=1;
  if (argc<=2)
    ErrMessage(0,NULL);
  else 
   {
    LineNum=atoi(argv[1]);
    if (LineNum<1||LineNum>MAXLINES)
      ErrMessage(0,NULL);
   }
  Initialize(LineNum);
//  sprintf(linedev,"/dev/ttyX%d",atoi(argv[1]) - 1); 
//  sprintf(linedev,"/dev/ttyA%02d",atoi(argv[1]) - 1); 
  snprintf(linedev, sizeof(linedev), "%s", argv[2]); 
  	// teraz moze byc uzywane i ze specjaliksem i z moksa 
  linedev[sizeof(linedev)-1] = '\0';

  if ((LineDes=OpenLine(linedev))<0)
    ErrMessage(2,linedev);
  if ((memdes=open("/dev/mem",O_RDONLY))==-1)
    ErrMessage(2,"/dev/mem");
  LineNum--;
  ctrmemaddr=(LineNum<8?SI_MOD1:LineNum<16?SI_MOD2:LineNum<24?SI_MOD3:
              LineNum<32?SI_MOD4:LineNum<40?SI_MOD5:LineNum<48?SI_MOD6:
              LineNum<56?SI_MOD7:SI_MOD8)+0x300*(LineNum<32?LineNum:
              LineNum-32)+0x0c;  
  lseek(memdes,ctrmemaddr,SEEK_SET);
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
    if (RadiosOnLine)
     {
      curradio=(unsigned char)(((int)curradio+1)%(int)RadiosOnLine);
      UnitsOnLine=RadioInfo[curradio].NumberOfUnits; 
      UnitsInfo=RadioInfo[curradio].UnitsInfo;
      if (!ConnectRadio(curradio))
       {
        for(i=0;i<UnitsOnLine;i++)
         {
          PrepareCommand(i, obuf);
          ReadReport(i);
	 }
        continue;
       } 
     }
    for(i=0;i<UnitsOnLine;i++)
     {
      PrepareCommand(i, obuf);
      if (!RadiosOnLine)
       {
        bread=TIOCM_RTS;
        ioctl(LineDes,TIOCMBIS,&bread); 
       }
      if (!DirectLine)
       {
        obuf[2]=UnitsInfo[i].UnitCode;
        write(LineDes,"\x11", 1);
        for(z=0;z<strlen(obuf);z++)
         {
          usleep(1000);
          write(LineDes,&obuf[z],1);
         }
	if (Simple)
          printf("sending -> %s\n", obuf);
       }
      else
        write(LineDes,rbuf,3); 
/*      do
       {
        read(memdes,mtab,2);
        lseek(memdes,ctrmemaddr,SEEK_SET);
       }
      while(mtab[1]!=mtab[0]); */
      if (!RadiosOnLine)
       {
       	sleep(1);
        ioctl(LineDes,TIOCMBIC,&bread);
        ioctl(LineDes,TCFLSH,0); 
       }
      if (RadiosOnLine) 
        sleep(20);
      else
        sleep(10);
      ReadReport(i);  
     } 
   }
 }

