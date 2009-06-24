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
 * alstmn.c
 * Demon odczytujacy dane z licznikow Alstom M301*
 */

#define _IPCTOOLS_H_

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
#include "msgerror.h"
#include "ipcdefines.h"

#define WRITE_MESSAGE_SIZE 8

#define READ_MESSAGE_SIZE 256


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


/* Numer rejestru z danymi MEASUREMENTS*/

#define MEASUREMENTS 454
#define NUM_OF_PARAM_MEAS 22

/* Numer rejestru z danymi POWER/ENERGY MEASUREMENTS*/

#define POWER_MEASUREMENTS 483
#define NUM_OF_PARAM_POW 24 

#define TOTAL_POWER_MEASUREMENTS 507
#define NUM_OF_PARAM_TOTAL_POW 1 

/* Ilosc urzadzen M301 do odczytu danych*/
#define NUMBER_OF_DEVICES 5

/* Ilosc parametrow odczytywanych bezposrednio z urzadzenia */
#define NUMBER_OF_PARAMETERS 23

/* Adresy urzadzen M301 firmy Alstom */
unsigned char DeviceAdresses[NUMBER_OF_DEVICES] = { 0x01, 0x02, 0x03, 0x04, 0x05 };

/*Postaæ ramki wysy³ana do licznika energii - MODBUS RTU*/
unsigned char outbuf[WRITE_MESSAGE_SIZE] = { 0x0A, 0x04, 0x00, 0x26, 0x00, 0x10, 0xA4, 0xB6 } ;


#define NUMBER_OF_VALS  (NUMBER_OF_PARAMETERS*NUMBER_OF_DEVICES)

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

static char auchCRCLo[] = {
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
  libpar_readpar("", "parcook_path", parcookpat, sizeof(parcookpat), 1);
  libpar_readpar("", "linex_cfg", linedmnpat, sizeof(linedmnpat), 1);

  VTlen=NUMBER_OF_VALS;
  if (Diagno)
    printf("Cooking deamon for line %d - CVMk deamon\n",linenum);
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
  linedes=open(line,O_RDWR|O_NDELAY|O_NONBLOCK);
  if (linedes<0)
    return(-1);
  ioctl(linedes,TCGETA,&rsconf);
  rsconf.c_iflag=0;
  rsconf.c_oflag=0;
  rsconf.c_cflag=B9600|CS8|CSTOPB|CLOCAL|CREAD;
  rsconf.c_lflag=0;
  rsconf.c_cc[4]=100;
  rsconf.c_cc[5]=100;
  ioctl(linedes,TCSETA,&rsconf);
  return(linedes);
 }

/* Funkcja ustawiaj±ca adres urz±dzenia */
void SetAddress(char address)
{
 outbuf[0] = address ;
}

/* Ustawia zawartosc wiadomosci wysylanej do licznika*/
/*buf - bufor z */
void SetMessageContents(unsigned char *buf,unsigned int adrp, unsigned char number )
{
 unsigned short CRC ;
 buf[1] = 0x04 ;
 buf[2] = adrp / 256;
 //buf[2] = ((adrp >> 8) /10 ) * 16 + ((adrp >> 8) % 10) ;
 //buf[3] = ((adrp % 100) / 10) * 16 + ((adrp % 100) % 10) ;
 buf[3] = adrp % 256 ;
 buf[4] = number / 256 ;
 buf[5] = number % 256 ;
 CRC =  CRC16(buf,6) ;
 buf[6] = (char )(CRC >> 8)  ;
 buf[7] = (char)CRC ;
}

/* Umieszczenie w buforze buf  danych z licznika energii zawartych w komunikacie inbuf*/
void PartitionMessage(unsigned char *inbuf,unsigned short lenght,long *buf,unsigned short *size)
{

  int i,j;
  for( i = 0 ; i < lenght ; i++ ) {
   if (Diagno) printf("%d,",inbuf[i]);
  }
  if (Diagno)  printf("\n");
  lenght -= 5 ;
  *size=0;
  if (lenght==2) {

	buf[0] = (inbuf[3] << 8) + inbuf[4] ;
    *size = 1;
    return ;
  }
  for (i = 0 , j = 0  ; i  < lenght   ; i+=4 , j++ ) {

    buf[j] = (inbuf[i+3] << 24 ) |(inbuf[i+4] << 16) | (inbuf[i+5] << 8) | inbuf[i+6]  ;
    *size = j+1;
  }
}

 /* zwraca dlugosc wczytanej komendy */
int ReadOutput(int sock,long *buf,unsigned short *size,unsigned int adrp, unsigned char number)
{
  unsigned short lenght,index;
  unsigned char inbuf[READ_MESSAGE_SIZE] ;
  /* ustalenie dok³adnej postaci ramki - ilo¶æ odczytywanych rejestrów (number) oraz adres pocz±tkowego rejestru
  odczytywanych danych*/
  SetMessageContents(outbuf, adrp, number);
  errno = 0;
  memset(inbuf, 0, READ_MESSAGE_SIZE);
  if (Diagno) printf("%d,%d,%d,%d,%d,%d,%d,%d\n",outbuf[0],outbuf[1],outbuf[2],outbuf[3],outbuf[4],outbuf[5],outbuf[6],outbuf[7]) ;
/* wys³anie komunikatu do urz±dzenia */
/*  i = 0;
  do {
    write(sock, &outbuf[i], 1) ;
    i++ ;
  } 
  while (i<WRITE_MESSAGE_SIZE) ;*/
  write(sock, outbuf, WRITE_MESSAGE_SIZE) ;
  //if (i<0)  return -1 ;
  /* d³ugo¶æ odczytywanej ramki - wynika z tego jak± wysy³amy ramkê do licznika */
  lenght = 2*number + 5 ;
  if (lenght > READ_MESSAGE_SIZE) lenght = READ_MESSAGE_SIZE;
/* odczyt odpowiedzi urz±dzenia */
//  usleep(1000000) ;
  sleep(1);
/*  index = 0;
  do {
     i = read(sock, &inbuf[index], 1);
     if (i>0) index++ ;
     else inbuf[index] = '\0' ;
  }
  while (i>0) ;*/
  index = read(sock, inbuf, lenght);
  if (index != lenght) {
    if (Diagno) printf("%d bytes was received!\n",index) ;
	  return -1;
  }
/* sprawdzenie poprawno¶ci CRC*/
  if (CheckCRC(inbuf, lenght )==-1) {

    if (Diagno) printf("Wrong CRC in received message!\n") ;
    return -1;
  }
  PartitionMessage(inbuf,lenght,buf,size);
  return 1 ;
}


void ReadData(char *linedev, unsigned int index)
 {
  long vals[NUMBER_OF_PARAMETERS];
  long ss[NUMBER_OF_PARAMETERS];
  unsigned short size ;
  int i,j,res;

  for ( i = 0 ; i < NUMBER_OF_PARAMETERS ; i++ ) 
    vals[i] = SZARP_NO_DATA;

  /*if ((LineDes=OpenLine(linedev))<0) {
      ErrMessage(2,linedev);
      perror("");
      exit(1);
    }*/
  j=0;
  res = -1;
  while (j<3 && res<0) {
   
   if ((res=ReadOutput(LineDes, ss, &size, MEASUREMENTS, NUM_OF_PARAM_MEAS)) > 0)

     {
       vals[0] = ss[0] / 100 ;
	     vals[1] = ss[1] / 100;
	     vals[2] = ss[2] / 100;
	     vals[3] = ss[3] / 100;
	     vals[4] = ss[4] / 100;
	     vals[5] = ss[5] / 100;
	     vals[6] = ss[6] ;
	     vals[7] = ss[7] ;
	     vals[8] = ss[8] ;
	     vals[9] = ss[9] ;
	     vals[10] = ss[10] /100;
       if (Diagno)  printf("Odczyt danych: ") ;
       if (Diagno)
           printf("vals[0]=%ld,vals[1]=%ld,vals[2]=%ld,vals[3]=%ld,vals[4]=%ld,vals[5]=%ld,vals[6]=%ld,vals[7]=%ld,vals[8]=%ld,vals[9]=%ld,vals[10]=%ld\n",
						vals[0], vals[1], vals[2], vals[3], vals[4], vals[5], vals[6], vals[7], vals[8], vals[9], vals[10]);

     }
     else
     {
       vals[0] = vals[1] = vals[2] = vals[3] = vals[4] = vals[5] = vals[6] = vals[7] = vals[8] = SZARP_NO_DATA ;
  	   vals[9] = vals[10] = SZARP_NO_DATA ;
       if (Diagno)
          printf("Brak danych: vals[0] = .. =  vals[10] = SZARP_NO_DATA \n");
     }
     usleep(200000) ;
     j++ ;
  }

/*  close(LineDes);
  if ((LineDes=OpenLine(linedev))<0) {
      ErrMessage(2,linedev);
      perror("");
      exit(1);
  }*/
  j=0;
  res = -1;
  while (j<3 && res<0) {
	  
   if ((res=ReadOutput(LineDes, ss, &size, POWER_MEASUREMENTS, NUM_OF_PARAM_POW)) > 0)
     {
       if (Diagno) printf("Odczyt danych: ") ;
       
       vals[11] = ss[0] / 10;
       vals[12] = ss[1] / 10;
       vals[13] = ss[2] / 10;
       vals[14] = ss[3] / 10;
       vals[15] = ss[4] / 10;
       vals[16] = ss[5] / 10;
       vals[17] = ss[6] / 10;
       vals[18] = ss[7] / 10;
       vals[19] = ss[8] / 10;
       vals[20] = ss[9] / 10;
       vals[21] = ss[10] / 10;
       vals[22] = ss[11] / 10;

       if (Diagno)
            printf("vals[11]=%ld,vals[12]=%ld,vals[13]=%ld,val[14]=%ld,val[15]=%ld,val[16]=%ld,val[17]=%ld,val[18]=%ld,val[19]=%ld,val[20]=%ld,val[21]=%ld,val[22]=%ld\n",
                   vals[11], vals[12], vals[13],vals[14],vals[15],vals[16],vals[17],vals[18],vals[19],vals[20],vals[21],vals[22]);
     }
     else
     {
       vals[11] = vals[12] = vals[13] = vals[14] = vals[15] = vals[16] = vals[17] = vals[18] = vals[19] = SZARP_NO_DATA ;
	     vals[20] = vals[21] = vals[22] = SZARP_NO_DATA ;
       if (Diagno)
            printf("Brak danych: vals[11] = .. =  vals[22] = SZARP_NO_DATA \n");
     }
    usleep(200000) ;
    j++;  
  }

 /* close(LineDes);
  if ((LineDes=OpenLine(linedev))<0) {
      ErrMessage(2,linedev);
      perror("");
      exit(1);
  }*/

  /*j=0;
  res = -1;
  while (j<3 && res<0) {
	  
   if ((res=ReadOutput(LineDes, ss, &size, TOTAL_POWER_MEASUREMENTS, NUM_OF_PARAM_TOTAL_POW)) > 0)
     {
       if (Diagno) printf("Odczyt danych: ") ;
       vals[23] = ss[0] ;
       if (Diagno)
                printf("vals[23] = %d\n" , vals[23]);
     }
     else
     {
       vals[23] = SZARP_NO_DATA;
       if (Diagno)
            printf("Brak danych: vals[23] = SZARP_NO_DATA \n");

     }
    usleep(200000) ;
    j++;
  }*/
  
  /*close(LineDes);*/
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
  printf("Alstom M301 device driver; can be used on both COM and Specialix ports\n");
  printf("Usage:\n");
  printf("      %s [s|d] <n> <port>\n", name);
  printf("Where:\n");
  printf("[s|d] - optional simple (s) or diagnostic (d) modes\n");
  printf("<n> - line number (necessary to read line<n>.cfg file)\n");
  printf("<port> - full port name, eg. /dev/term/01 or /dev/term/a12\n");
  printf("Comments:\n");
  printf("No parameters are taken from line<n>.cfg (base period = 1)\n");
  printf("Port parameters: 9600 baud, 8N2; signals required: TxD, RxD, GND\n");
  exit(1);
 }


int main(int argc,char *argv[])
 {
  ushort i;
  unsigned char parind;
  char linedev[30];

  libpar_read_cmdline(&argc, argv);
  libpar_init();
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
      exit(1);
    }

// sleep(5) ;
   while(1)
   {
//       sleep(5) ;
	  for (i = 0 ; i < NUMBER_OF_DEVICES ; i++ ) {
	    SetAddress(DeviceAdresses[i]) ; 

	    ReadData(linedev,i * NUMBER_OF_PARAMETERS);
	  }
   }

   close(LineDes) ;
 }
