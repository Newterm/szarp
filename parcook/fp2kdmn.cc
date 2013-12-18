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
 * fp2kdmn.c
 * $Id$
 */
 
/* Driver przelicznikow metronic FP-2001 i Fp-2001W */
/* Odczytuje kolejno dane z dwoch licznikow: FP-2001(para) , FP-2001W(woda)*/
/* czyta parametry z pliku konfiguracyjnego fp2001.cfg */

/*
 @description_start
 @class 2
 @devices Metronic FP-2001, Metronic FP-2001W.
 @protocol Modbus RTU.
 @config Reads configuration from fp2001.cfg file.
 @config.pl Czyta konfiguracjê z pliku fp2001.cfg file.
 @comment This driver is obsolete, use general Modbus driver (mbdmn).
 @comment.pl Ten demon jest przestarza³y, zamiast niego nale¿y u¿ywaæ ogólnego demona 
 do obs³ugi protoko³u Modbus, czyli mbdmn.
 @description_end
*/

#define _IPCTOOLS_H_
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
#include <libgen.h>
#include <math.h>

#include "libpar.h"
#include "ipcdefines.h"
#include "msgerror.h"

#define WRITE_MESSAGE_SIZE 8

#define READ_MESSAGE_SIZE 256

#define DEFAULT_TTY "/dev/ttyS%02d"

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

static unsigned char UnitsOnLine;

short *EntalpiaParyDivBy ;/* Tablica do dzielenia entalpii pary dla ka¿dego Unitu oddzielnie */
short *EntalpiaWodyDivBy ;/* Tablica do dzielenia entalpii wody dla ka¿dego Unitu oddzielnie */

/* Ilosc parametrow odczytywanych bezposrednio z urzadzenia */
#define NUMBER_OF_PARAMETERS 12

/* Adresy urzadzen FP-2001/W */
unsigned int *DeviceAdresses;
/* Typy urzadzen*/
/* 1 - FP-2001*/
/* 2 - FP-2001W*/
unsigned int *DeviceType;


/*Postaæ ramki wysy³ana do licznika energii - MODBUS RTU*/
unsigned char outbuf[WRITE_MESSAGE_SIZE] = { 0x02, 0x04, 0x00, 0x00, 0x00, 0x0C, 0xA4, 0xB6 } ;

/* Tabela warto¶ci CRC dla bardziej znacz±cego bajtu */

static unsigned char auchCRCHi[] = {
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40
} ;


/* Tabela warto¶ci CRC dla mniej znacz±cego bajtu */

static char unsigned auchCRCLo[] = {
0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06,
0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD,
0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A,
0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4,
0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3,
0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29,
0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED,
0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60,
0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67,
0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,
0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E,
0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71,
0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92,
0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B,
0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B,
0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42,
0x43, 0x83, 0x41, 0x81, 0x80, 0x40
} ;

/* Funkcja licz±ca CRC dla protoko³u MODBUS*/

unsigned short CRC16(unsigned char *buf,unsigned short lenght)
{
	unsigned char CRCHi = 0xFF ;	/* high CRC  */
	unsigned char CRCLo = 0xFF ;	/* low CRC  */
	unsigned index ;				
										
	while (lenght--)		
		{
		 index = CRCHi ^ *(buf++) ;	/* liczenie CRC */
		 CRCHi = CRCLo ^ auchCRCHi[index] ;
		 CRCLo = auchCRCLo[index] ;
		}

	return (CRCHi << 8 | CRCLo) ;
}

/* Funkcja sprawdzaj±ca poprawno¶æ CRC danych w buf */
int CheckCRC(unsigned char *buf, unsigned short lenght )
{
 unsigned short receivedCRC , calculatedCRC ;

 calculatedCRC = CRC16(buf, lenght - 2);

 receivedCRC = buf[lenght-2] << 8 | buf[lenght-1] ;

/* wyliczone CRC nie zgadza siê z CRC otrzymanej wiadomo¶ci */
 if (calculatedCRC!=receivedCRC) return -1;
 return 1 ;

}

void Initialize(unsigned char linenum)
 {

  char linedefname[81];
  FILE *linedef;
  int val,val1,val2,val3,val4,i;
    
  libpar_init();
  libpar_readpar("", "parcook_path", parcookpat, sizeof(parcookpat), 1);
  libpar_readpar("", "linex_cfg", linedmnpat, sizeof(linedmnpat), 1);
  //libpar_done();

  VTlen=0;
  sprintf(linedefname,"%s/line%d.cfg",linedmnpat, linenum);
  if ((linedef=fopen(linedefname,"r"))==NULL)
    ErrMessage(2,linedefname);

  if (fscanf(linedef,"%d\n",&val)>0)
  {
    if (val>0)
      UnitsOnLine=val;
    else
      UnitsOnLine=1;
  }
  else
      exit(-1);

 
  VTlen = UnitsOnLine*NUMBER_OF_PARAMETERS ;
  
  DeviceAdresses  = (unsigned int *)calloc(UnitsOnLine,sizeof(int)) ;
  DeviceType  = (unsigned int *)calloc(UnitsOnLine,sizeof(int)) ;
       
  /*UWAGA! UWAGA! UWAGA! - drugi parametr z pliku lineX.cfg to typ urzadzenia:*/
  /* 1 - FP-2001 (para) */
  /* 2 - FP-2001W (woda) */
  /* 3 - FP-2001W (woda) */
  
  for (i = 0 ; i < UnitsOnLine ; i++)
  {
      fscanf(linedef,"%u %u %u %u %u %u\n",&DeviceAdresses[i],
          &DeviceType[i],&val1,&val2,&val3,&val4);
      if ((DeviceType[i]<1) || (DeviceType[i]>0xff))
      {
          if (Diagno)
            printf("Wrong device type in lineX.cfg file (second parameter)! id is modbus identifier and must be  between 1 255. Typically may be 1 for FP-2001 or 2 for FP-2001W\n but not %u\n",DeviceType[i]) ;
        
          exit(-1) ;
      }
  }
  
  fclose(linedef) ;
  
  if (Diagno)
    printf("Cooking deamon for line %d - FP-2001/W deamon\n",linenum);
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
    case 19200:rsconf.c_cflag=B19200;
              break;
    case 9600:
      default:rsconf.c_cflag=B9600;
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

 
/* Funkcja ustawiaj±ca adres urz±dzenia */
void SetAddress(int address)
{
 outbuf[0] = (char )address ;
}

/* Ustawia zawartosc wiadomosci wysylanej do licznika*/
/*buf - bufor z */
void SetMessageContents(unsigned char *buf,unsigned char adrp, unsigned char number )
{
 unsigned short CRC ;
 buf[1] = 0x04 ;
 buf[2] = 0x00 ;
 buf[3] = adrp ;
 buf[4] = 0x00 ;
 buf[5] = number ;
 CRC =  CRC16(buf,6) ;
 buf[6] = CRC >> 8  ;
 buf[7] = CRC ;
}

/* Umieszczenie w buforze buf  danych z licznika energii zawartych w komunikacie inbuf*/
void ParitionMessage(unsigned char *inbuf,unsigned short lenght,unsigned int *buf,unsigned short *size)
{
  int i,j;
  for( i = 0 ; i < lenght ; i++ )
  {
   if (Diagno) printf("%x,",inbuf[i]);
  }
  if (Diagno)  printf("\n");
  lenght -= 5 ;
  for (i = 0 , j = 0  ; i < lenght ; i+=2 , j++ )
  {
    buf[j] = inbuf[i+3] << 8 | inbuf[i+4] ;
    *size = j+1;
  }
}

 /* zwraca dlugosc wczytanej komendy */
int ReadOutput(int sock,unsigned int *buf,unsigned short *size,unsigned char adrp, unsigned char number)
{
  int i;
  unsigned short lenght;
  unsigned char inbuf[READ_MESSAGE_SIZE] ;
  /* ustalenie dok³adnej postaci ramki - ilo¶æ odczytywanych rejestrów (number) oraz adres pocz±tkowego rejestru
  odczytywanych danych*/
  SetMessageContents(outbuf, adrp, number);
  errno = 0;
  memset(inbuf, 0, READ_MESSAGE_SIZE);
/* wys³anie komunikatu do urz±dzenia */
  i = 0;
  do {
    write(sock, &outbuf[i], 1) ;
    i++ ;
  } 
  while (i<WRITE_MESSAGE_SIZE) ;
  if (i<0)  return -1 ;
  if (Diagno)  printf("%s\n",outbuf);
  /* d³ugo¶æ odczytywanej ramki - wynika z tego jak± wysy³amy ramkê do licznika */
  lenght = 2*number + 5 ;
  if (lenght > READ_MESSAGE_SIZE) lenght = READ_MESSAGE_SIZE;
  sleep(2) ;
  i = read(sock, inbuf, lenght); 
  if (i != lenght) {
    if (Diagno) printf("%d bytes was received!\n",i) ;
	  return -1;
  }
  if (errno) return -1;
/* sprawdzenie poprawno¶ci CRC*/
  if (CheckCRC(inbuf, lenght )==-1) {

    if (Diagno) printf("Wrong CRC in received message!\n") ;
    return -1;
  }
  ParitionMessage(inbuf,lenght,buf,size);
  return i ;
}


void ReadData(int linedes, int index, int devtype, int Uon, short *EntalpiaParyDivBy, short *EntalpiaWodyDivBy)
 {
    unsigned int ss[NUMBER_OF_PARAMETERS];
    int vals[NUMBER_OF_PARAMETERS];
    unsigned short size ;
    int i,j;

    vals[0] = vals[1] = vals[2] = vals[3] = vals[4] = vals[5] = vals[6] = vals[7] = vals[8] = vals[9] = vals[10] = vals[11] = SZARP_NO_DATA;

    if (ReadOutput(linedes, ss, &size, 0, 12) > 0)
    {
        if (Diagno)  printf("Odczyt danych: ") ;
        vals[0] = ss[0] - 10000;
        vals[1] = ss[1] - 10000;
        vals[2] = ss[2] - 10000;
        vals[3] = ss[3] - 10000;
        vals[4] = ss[4] - 10000;
        vals[5] = ss[5] - 10000;
        vals[6] = ss[6] - 10000;
        vals[7] = ss[7] - 10000;
        vals[8] = ss[8] - 10000;
        vals[9] = ss[9] - 10000;
        if (devtype==1) /* FP-2001 ( para)*/ 
          vals[10] = (ss[10] - 10000)/(int)EntalpiaParyDivBy[Uon];
        else
          vals[10] = (ss[10] - 10000);
 
        vals[11] = (ss[11] - 10000)/(int)EntalpiaWodyDivBy[Uon];
        if (Diagno)
            printf("vals[0] = %d , vals[1] = %d , vals[2] = %d , vals[3] = %d , vals[4] = %d , vals[5] = %d , vals[6] = %d , vals[7] = %d, vals[8] = %d\n , vals[9] = %d , vals[10] = %d , vals[11] = %d", vals[0], vals[1], vals[2], vals[3], vals[4], vals[5], vals[6], vals[7], vals[8], vals[9], vals[10], vals[11]);
    }
    else
      if (Diagno)
        printf("Brak danych: vals[0] = vals[1] = vals[2] = vals[3] = ... = vals[11] = SZARP_NO_DATA \n");

    sleep(2) ;

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
    for(i = index , j = 0 ; (i < VTlen) && ( j < NUMBER_OF_PARAMETERS ); i++ , j++)
      ValTab[i] = (short)vals[j];
    Sem[0].sem_num=SEM_LINE+2*(LineNum-1)+1;
    Sem[0].sem_op=-1;
    Sem[0].sem_flg=SEM_UNDO;
    semop(SemDes,Sem,1);
}


void Usage(char *name)
 {
  printf("FP-2001/W driver; can be used on both COM and Specialix ports\n");
  printf("Usage:\n");
  printf("      %s [s|d] <n> <port>\n", name);
  printf("Where:\n");
  printf("[s|d] - optional simple (s) or diagnostic (d) modes\n");
  printf("<n> - line number (necessary to read line<n>.cfg file)\n");
  printf("<port> - full port name, eg. /dev/term/01 or /dev/term/a12\n");
  printf("Comments:\n");
  printf("No parameters are taken from line<n>.cfg (base period = 1)\n");
  printf("Port parameters: 9600 baud, 8N1; signals required: TxD, RxD, GND\n");
  exit(1);
 }


void ReadCfgFile(char *conf_path, int LN, int U, unsigned int *DT)
{
	char buf[255] ;
	char ziut[255];
	int ii;
	libpar_init_with_filename(conf_path,1);
	short EntalpiaParyDivBy_;
	short EntalpiaWodyDivBy_;
	libpar_readpar("","EntalpiaParyDivBy",buf,sizeof(buf),0);
	EntalpiaParyDivBy_ = atoi(buf) ;
	memset(buf,0,sizeof(buf));
	if (EntalpiaParyDivBy_<=0) EntalpiaParyDivBy_=1;  
	libpar_readpar("","EntalpiaWodyDivBy",buf,sizeof(buf),0);
	EntalpiaWodyDivBy_ = atoi(buf) ;	
	if (EntalpiaWodyDivBy_<=0) EntalpiaWodyDivBy_=1;  
  
	/* Dzielnik entalpii pary EntalpiaParyDivBy_line_unit */
	for (ii=0;ii<U;ii++){
		sprintf(ziut,"EntalpiaParyDivBy_%d_%d",LN, DT[ii]);
		memset(buf,0,sizeof(buf));
		libpar_readpar("",ziut,buf,sizeof(buf),0);
		EntalpiaParyDivBy[ii] = atoi(buf) ;
  		if (EntalpiaParyDivBy[ii]<=0) EntalpiaParyDivBy[ii] = EntalpiaParyDivBy_;  
		
	}
	for (ii=0;ii<U;ii++){

		sprintf(ziut,"EntalpiaWodyDivBy_%d_%d",LN, DT[ii]);
		memset(buf,0,sizeof(buf));
		libpar_readpar("",ziut,buf,sizeof(buf),0);
		EntalpiaWodyDivBy[ii] = atoi(buf) ;
  		if (EntalpiaWodyDivBy[ii]<=0) EntalpiaWodyDivBy[ii] = EntalpiaWodyDivBy_;  
		
	}

}

int main(int argc,char *argv[])
 {
  int baud,stopb;
  unsigned char parind;
  char linedev[30];
  char conf_path[80] ;
  
  int i ;
  

  if (argc<2)
    Usage(argv[0]);
  Diagno=(argv[1][0]=='d');
  Simple=(argv[1][0]=='s');
  if (Simple)
    Diagno=1;
  if (Diagno)
    parind=2;
  else
    parind=1;
  if (argc<(int)parind+1)
    Usage(argv[0]);
  baud=9600;
  stopb=1;
  LineNum=atoi(argv[parind]);

   
  sprintf(linedev, DEFAULT_TTY, LineNum-1);
  if (argc>(int)parind+1)
    strcpy(linedev, argv[parind+1]);
  if (argc>(int)parind+2)
    baud=atoi(argv[parind+2]);
  if (argc>(int)parind+3)
    stopb=atoi(argv[parind+3]);
  libpar_init();

  Initialize(LineNum);
  LineNum--;
  /* Inicjalizujemy tablicê dzielników */
  EntalpiaParyDivBy = (short *)malloc(sizeof (short) * UnitsOnLine ) ;
  EntalpiaWodyDivBy = (short *)malloc(sizeof (short) * UnitsOnLine );
  for (i = 0 ; i < UnitsOnLine ; i++ ){
 	EntalpiaParyDivBy[i] = 1;
 	EntalpiaWodyDivBy[i] = 1;
  } 
  libpar_readpar("fp2001", "cfgfile", conf_path, sizeof(conf_path), 1);
  ReadCfgFile(conf_path, LineNum + 1, UnitsOnLine, DeviceAdresses) ;
  libpar_done();
  
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
	  for (i = 0 ; i < UnitsOnLine ; i++ )
    {
      if ((LineDes=OpenLine(linedev, baud, stopb))<0)
      {
        ErrMessage(2,linedev);
        exit(1) ;
      }
      SetAddress(DeviceAdresses[i]) ;
	    ReadData(LineDes,i * NUMBER_OF_PARAMETERS,DeviceType[i], i, EntalpiaParyDivBy,EntalpiaWodyDivBy );
      close(LineDes);
	  }
   }
 free (EntalpiaParyDivBy) ;
 free (EntalpiaWodyDivBy) ;
   
 }
