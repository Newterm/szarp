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
   Driver urzadzenia DataPaf 3.X w Starachowicach,Swidniku
   WAZNE!!! W katalogu config musza byc pliki XXXXXser1.dat i XXXXser0.dat z kodami,  
   gdzie XXXX to numer seryjny rejestratora DataPaf
*/

/*
 @description_start
 @class 2
 @devices DataPAF 3.x Energy meters.
 @devices.pl Liczniki energii DataPAF 3.X
 @description_end
*/
/*
 * SZARP 2.0
 * parcook
 * dpafdmn.c
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

static int LineDes;

static int LineNum;

static int SemDes;

static int ShmDes;

static int BaudRate ;

static int Energy ;

static int EnergyDivBy ;

static int Power ;

static int ZonesCount;

static char Zones[50];

static int ChannelsCount ;
static int Channels[30];

static int Sleep ;

static unsigned char Diagno=0;

static unsigned char Simple=0;

static int VTlen;

static short *ValTab;

static struct sembuf Sem[2];

static char parcookpat[81];

static char linedmnpat[81];

int buf[5] ;

short avgwindow;
short *AVG;
int *Vals2;
short *Vctr;
short ll_ ;
char go;

/* Tablica kodow */
unsigned long int tab_code[12][31] ;

/* Numer miesiaca z urzadzenia */
unsigned short month ;

/* Numer dnia z urzadzenia */
unsigned short day ;

/* Polecenia */
unsigned char INIT[1] = { 0x1F } ;
unsigned char ACK[1] = { 0x06 } ;
unsigned char NAK[1] = { 0x15 } ;
unsigned char KAS[1] = { 0x18 } ;

/* Numer seryjny urzadzenia: */
char ser_num[] = "XXXX\0" ;
/* nazwa pliku z kodami: XXXser0.dat - rs232 nr 0 , XXXXser0.dat - rs232 nr 1*/
char code_filename[] = "XXXXserX.dat" ;

char code_path[255] ;
/* Struktura bloku rozkazu */
#define SIZE_OF_OUTBUF 40
unsigned char outbuf[SIZE_OF_OUTBUF] = { 0x01 , 'X', 'X',  0x52, 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', '\0'} ;

/* Format komend zapytan przesylanych do urzadzenia */

/* Czas */
#define SIZE_OF_TIM 6
unsigned char tim[SIZE_OF_TIM] = { 'D' , 'C', 'Z', 'K' , 'O', 'N' } ;

/* Data */
#define SIZE_OF_DAT 6
unsigned char dat[SIZE_OF_DAT] = { 'D' , 'D', 'A', 'K' , 'O', 'N' } ;

/* Energia   */
#define SIZE_OF_ENE 10
unsigned char ene[SIZE_OF_ENE] = { 'E','N','E', '0' , '1' , ',' , '0' , 'K' , 'O', 'N' } ;

/* Moc Minutowa  */
#define SIZE_OF_MMI 8
unsigned char mmi[SIZE_OF_MMI] = { 'M','M','I' , '0' , '1' , 'K' , 'O', 'N' } ;

/* Bufor dla odpowiedzi urzadzenia */
#define SIZE_OF_INBUF 60
unsigned char inbuf[SIZE_OF_INBUF] ;

void Initialize(unsigned char linenum)
 {
  libpar_readpar("", "parcook_path", parcookpat, sizeof(parcookpat), 1);
  libpar_readpar("", "linex_cfg", linedmnpat, sizeof(linedmnpat), 1);

  VTlen=ChannelsCount*Power + ZonesCount*Energy*ChannelsCount;
  if (Diagno)
    printf("Cooking deamon for line %d - DataPaf 3.X daemon\n",linenum);
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

static int OpenLine(char *line,int baudRate)
 {
  int linedes;
  struct termio rsconf;
  linedes=open(line,O_RDWR|O_NDELAY|O_NONBLOCK);
  if (linedes<0)
    return(-1);
  ioctl(linedes,TCGETA,&rsconf);
  rsconf.c_iflag=0;
  rsconf.c_oflag=0;
	switch(baudRate)
	{
  	case 50:
  	{
  		rsconf.c_cflag=B50|CS8|CLOCAL|CREAD;
  		Sleep = 1;
  		break ;
  	}
  	case 300:
  	{
  		rsconf.c_cflag=B300|CS8|CLOCAL|CREAD;
  		Sleep = 2;  					
  		break ;
  	}
  	case 600:
  	{
  		rsconf.c_cflag=B600|CS8|CLOCAL|CREAD;
  		Sleep = 3;  					  					
  		break ;
  	}
  	case 1200:
  	{
  		rsconf.c_cflag=B1200|CS8|CLOCAL|CREAD;
  		Sleep = 4;  					  					  					
  		break ;
  	}
  	case 2400:
  	{	
  		rsconf.c_cflag=B2400|CS8|CLOCAL|CREAD;
  		Sleep = 5;  					  					  					  					
  		break ;
  	}
  	case 4800:
  	{
  		rsconf.c_cflag=B4800|CS8|CLOCAL|CREAD;
  		Sleep = 6;  					  					  					  					  					
  		break ;
  	}
		default:
		{
			rsconf.c_cflag=B50|CS8|CLOCAL|CREAD;
  		Sleep = 1;  					  					  					  					  					  					
  		break ;				
  	}
  }
  rsconf.c_lflag=0;
  rsconf.c_cc[4]=0;
  rsconf.c_cc[5]=0;
  ioctl(linedes,TCSETA,&rsconf);
  return(linedes);
 }

/* Umieszczenie w buforze buf  danych z licznika energii zawartych w komunikacie inbuf*/
int PartitionMessage(unsigned char *inbuf,unsigned short lenght,long *value)
{

  char *endptr;
  char buf[25] ;
  long lnumber ;
  int index,ok ;
  unsigned char len[3] ;
  unsigned char err[3] ;
  unsigned char mon[3] ;
  unsigned char da[3] ;
  int i;

  ok = 0;
  /*!!! CheckSum!!!*/
  len[0] = inbuf[1] ;
  len[1] = inbuf[2] ;
  len[2] = '\0' ;

  lnumber = strtol((char*)len, &endptr, 16);

  if (inbuf[4] == 's' && inbuf[5] == 'e' && inbuf[6] == 'c') return 1;
  if (inbuf[4] == 'd' && inbuf[5] == 'd' && inbuf[6] == 'a') {
      /* odczytanie miesiaca z odpowiedzi urzadzenia */
      i = 0 ;
      while (inbuf[7+i]!='/' && i<7) {
        da[i] = inbuf[7+i] ;
        i++ ;
      }
      if (i == 7) return -1 ;
      da[i] = '\0';
      i++;
      mon[0] =  inbuf[7+i] ;
      mon[1] =  '\0' ;
      if (inbuf[8+i]!='\\') {

        mon[1] =  inbuf[8+i] ;
        mon[2] =  '\0' ;
      }
      month = atoi((char*)mon) ;
      day   = atoi((char*)da) ;
      return 1;
  }
  if (inbuf[4] == 'e' && inbuf[5] == 'r' && inbuf[6] == 'r') {

      err[0] = inbuf[7] ;
      err[1] = inbuf[8] ;
      err[2] = '\0' ;
      return  (-atoi((char*)err)) ;
  }

  index = 0 ;
  if (Diagno) printf ("lnumber = %s , %ld\n", len , lnumber );
  for (i = 0 ; i < lnumber-3 ; i++ )
  {
	  if (inbuf[4+i] == ',' ) 
	  {
 	    index = 0 ;
	  }
	  else if (inbuf[4+i] != 'k' && inbuf[4+i] != 'o' && inbuf[4+i] != 'n')
	  {
 		    buf[index] = inbuf[4+i] ;
	      index++;
    }
    else if (inbuf[4+i] == 'k' && inbuf[5+i] == 'o' && inbuf[6+i] == 'n') {
      ok = 1;
      break ;
    }
  }
  buf[index] = '\0' ;
  if (ok==0) {
//    printf("\nDupa\n");
    return -1;
  }
  *value = atol(buf) ;
  return 1 ;
}

/*Wyliczanie sumy kontrolnej*/
unsigned char CheckSum(unsigned char *buf,unsigned short len )
{
 int i ;
 int sum ;
 sum = 0 ;
 for (i = 0 ; i < len   ; i++ )
 {
   sum+=buf[i] ;
 }
 sum = sum % 256 ;
 return (unsigned char)sum;
}

/* Tworzy komende wysylana do urzadzenia */
unsigned short MakeCommandSec(unsigned char *outbuf, unsigned short size_of_outbuf )
{
  unsigned char buf[3] ;
  unsigned char code[25] ;
  unsigned short len,size_of_command;
  int i;

  sprintf((char*)code, "%ld", tab_code[month-1][day-1]);

  size_of_command = strlen((char*)code) + 8;
  len = 6 + size_of_command;

  sprintf((char*)buf, "%02x", (int)size_of_command);

  for ( i = 0 ; i < 2 ; i++) {

     if (buf[i]=='a') buf[i] = 'A' ;
     if (buf[i]=='b') buf[i] = 'B' ;
     if (buf[i]=='c') buf[i] = 'C' ;
     if (buf[i]=='d') buf[i] = 'D' ;
     if (buf[i]=='e') buf[i] = 'E' ;
     if (buf[i]=='f') buf[i] = 'F' ;
     outbuf[i+1] = buf[i] ;

  }

  outbuf[4] = 'S' ;
  outbuf[5] = 'E' ;
  outbuf[6] = 'C' ;

  if (Diagno) printf("\n%zd \n", strlen((char*)code)) ;
  for ( i = 0 ; i<strlen((char*)code) ; i++ )
    outbuf[7+i] = code[i] ;

  outbuf[7+i] = ',' ;
  outbuf[8+i] = '0' ;
  outbuf[9+i] = 'K' ;
  outbuf[10+i] = 'O' ;
  outbuf[11+i] = 'N' ;
  sprintf((char*)buf, "%02x", (int)CheckSum(outbuf,len-2 ));

  for (i = 0 ; i < 2 ; i++) {
     if (buf[i]=='a') buf[i] = 'A' ;
     if (buf[i]=='b') buf[i] = 'B' ;
     if (buf[i]=='c') buf[i] = 'C' ;
     if (buf[i]=='d') buf[i] = 'D' ;
     if (buf[i]=='e') buf[i] = 'E' ;
     if (buf[i]=='f') buf[i] = 'F' ;
     outbuf[len-2+i] = buf[i] ;

  }
  outbuf[len] = '\0' ;
  if (Diagno) printf("%s\n" , outbuf) ;
  return len;

}

/* Tworzy komende wysylana do urzadzenia */
unsigned short MakeCommand(unsigned char *outbuf, unsigned short size_of_outbuf, unsigned char *command, unsigned short size_of_command ) 
{
  unsigned char buf[3] ;
  unsigned short len; 
  int i;
  len = 6 + size_of_command;
  sprintf((char*)buf, "%02x", (int)size_of_command);
   
  for ( i = 0 ; i < 2 ; i++) {

     if (buf[i]=='a') buf[i] = 'A' ;
     if (buf[i]=='b') buf[i] = 'B' ;
     if (buf[i]=='c') buf[i] = 'C' ;
     if (buf[i]=='d') buf[i] = 'D' ;
     if (buf[i]=='e') buf[i] = 'E' ;
     if (buf[i]=='f') buf[i] = 'F' ;
     outbuf[i+1] = buf[i] ;
  
  } 
  for ( i = 0 ; i<size_of_command ; i++ )
    outbuf[4+i] = command[i] ;
  sprintf((char*)buf, "%02x", (int)CheckSum(outbuf,len-2 ));
  
  for (i = 0 ; i < 2 ; i++) {
     if (buf[i]=='a') buf[i] = 'A' ;
     if (buf[i]=='b') buf[i] = 'B' ;
     if (buf[i]=='c') buf[i] = 'C' ;
     if (buf[i]=='d') buf[i] = 'D' ;
     if (buf[i]=='e') buf[i] = 'E' ;
     if (buf[i]=='f') buf[i] = 'F' ;
     outbuf[len-2+i] = buf[i] ;
 
  }
  outbuf[len] = '\0' ;
  return len;
}

/* zwraca dlugosc wczytanej komendy */
int ReadOutput(int sock,unsigned char *outbuf, unsigned short size_of_outbuf, long *value )
{
  int i,j;
  unsigned short index;
  unsigned char resp[1] ;

  /* ustalenie dok³adnej postaci ramki - ilo¶æ odczytywanych rejestrów (number) oraz adres pocz±tkowego rejestru
  odczytywanych danych*/
  errno = 0;
  memset(inbuf, 0, 25);
  i = write(sock, INIT, 1) ;
  if (i<0) return -1;
  usleep(4000000/Sleep) ;
//  sleep(3) ;
  i = read(sock,&resp[0], 1) ;
  if (i<0) {
      i = write(sock, KAS, 1) ;
      if (i<0) return -1;
  		usleep(4000000/Sleep) ;
//      sleep(3) ;
      i = read(sock,&resp[0], 1) ;
      return -1;
  }
  if (i<0 || resp[0]!=INIT[0]) {
      if (Diagno) printf("No response\n") ;
      return -1;
  }
  if (Diagno) printf(" INIT/KAS received \n") ;
//  sleep(6) ;
	usleep(2000000/Sleep) ;
  i = write(sock, outbuf, size_of_outbuf) ;
  if (Diagno)
  // printf("frame %c%c%c%c%c%c%c was sended \n", outbuf[0],outbuf[1],outbuf[2],outbuf[3],outbuf[4],outbuf[5],outbuf[6]) ;
     printf(" Frame %s was sended, length %d \n" , outbuf, size_of_outbuf) ;
  if (i<0)  return -1 ;
	usleep(4000000/Sleep) ;
 // sleep(6) ;
  i = read(sock, &resp[0], 1);
  if (i<0) {
    if (Diagno) printf(" Nothing received!\n") ;
    return -1;
  }
  if (resp[0]==NAK[0]) {
     if (Diagno) printf(" NAK received!\n");
     return -1 ;
  }   
  if (Diagno) printf(" ACK received!\n") ;
  
  index = 0;
//  sleep(3) ;
	usleep(1000000/Sleep) ;
  do {
     i = read(sock, &inbuf[index], 1);
     usleep(200000/Sleep) ;
     if (i>0) index++ ;
     else inbuf[index] = '\0' ;
  }
  while (i>0); 

  if (Diagno) printf(" %s\n", inbuf) ;
  if (Diagno) printf(" response %s received, length %d \n", inbuf, index) ;
 
  j =  PartitionMessage(inbuf,index,value) ;
  if (j==-1) {
    write(sock, NAK, 1) ;	  
    if (Diagno) printf(" NAK sended \n");
  }
  else
	  write(sock, ACK, 1) ;
  return j ;
}


int ReadEnergy(long *value, unsigned char strefa, char *kanal)
{
   int res;
   unsigned short size_of_outbuf ;
   ene[3] = kanal[0] ;
   ene[4] = kanal[1] ;  			
   ene[6] = (unsigned char)(strefa+48) ;
   size_of_outbuf =  MakeCommand(outbuf, SIZE_OF_OUTBUF, ene, SIZE_OF_ENE )  ;
   res = ReadOutput(LineDes, outbuf, size_of_outbuf, value ) ;
   if (res==-15) { /* Trzeba podac kod dostepu*/

    size_of_outbuf =  MakeCommand(outbuf, SIZE_OF_OUTBUF, dat, SIZE_OF_DAT )  ;
    res = ReadOutput(LineDes, outbuf, size_of_outbuf, value ) ;
    if (res<0)   return -1;
    if (Diagno) printf("Data %d\%d \n", day, month) ;
    size_of_outbuf =  MakeCommandSec(outbuf, SIZE_OF_OUTBUF)  ;
    res = ReadOutput(LineDes, outbuf, size_of_outbuf, value ) ;
   }
   return res;
}

int ReadPower(long *value, char *kanal)
{
   int res;
   unsigned short size_of_outbuf ;
   mmi[3] = kanal[0] ;
   mmi[4] = kanal[1] ;
   size_of_outbuf =  MakeCommand(outbuf, SIZE_OF_OUTBUF, mmi, SIZE_OF_MMI )  ;
   res = ReadOutput(LineDes, outbuf, size_of_outbuf, value ) ;
   if (res==-15) { /* Trzeba podac kod dostepu*/

    size_of_outbuf =  MakeCommand(outbuf, SIZE_OF_OUTBUF, dat, SIZE_OF_DAT )  ;
    res = ReadOutput(LineDes, outbuf, size_of_outbuf, value ) ;
    if (res<0) return -1;
    size_of_outbuf =  MakeCommandSec(outbuf, SIZE_OF_OUTBUF)  ;
    res = ReadOutput(LineDes, outbuf, size_of_outbuf, value ) ;
   }
   return res;
}


void ReadData(char *linedev)
 {
	int NumberOfVals ;
	int *vals;
	long value ;
	int i,j,res,k,zz;
	char kanal[] = "00\0";
	NumberOfVals = ChannelsCount*Power + ZonesCount*Energy*ChannelsCount ;
	vals = (int*)malloc(NumberOfVals*sizeof(int)) ;
	for ( i = 0 ; i < NumberOfVals ; i++ )
		vals[i] = SZARP_NO_DATA;

	j=0;
	res = -1;

	if (Energy)
	{
		for (i = 0 ; i< ChannelsCount ; i++ )
		{
  			if (Channels[i]<10)
  			{
  				kanal[0] = '0' ;
  				kanal[1] = (unsigned char)(Channels[i]+48);  		
  			}
  			else
  				if (Channels[i]<20)
  				{
  					kanal[0] = '1' ;
  					kanal[1] = (unsigned char)(Channels[i]+38);  		
  				}
  				else  			
  				{
  					kanal[0] = '2' ;
  					kanal[1] = (unsigned char)(Channels[i]+38);  		
  				}
    			if (Diagno) printf("Energy, Kanal %s \n", kanal) ;
  				
  				for (k = 0 ; k < ZonesCount ; k++, j++)
  				{
  					if (ReadEnergy(&value,Zones[k],kanal)>0)
  					{
       					    res = 1 ;
       					    if (Diagno) printf("Energy, zone %c, channel %s = %ld\n", Zones[k], kanal, value  );
       					    vals[j] = value / EnergyDivBy;
  					}
  					else
      					    if (Diagno) printf("Brak danych \n") ;
      			}
  		}
	}
	if (Power)
	{
		for (i = 0 ; i< ChannelsCount ; i++, j++)
		{
  			if (Channels[i]<10)
  			{
  				kanal[0] = '0' ;
  				kanal[1] = (unsigned char)(Channels[i]+48);  		
  			}
  			else
  			if (Channels[i]<20)
  			{
  				kanal[0] = '1' ;
  				kanal[1] = (unsigned char)(Channels[i]+38);  		
  			}
  			else  			
  				{
  					kanal[0] = '2' ;
  					kanal[1] = (unsigned char)(Channels[i]+38);  		
  				}
    		if (Diagno) printf("Power, Kanal %s \n", kanal) ;
  			if (ReadPower(&value,kanal)>0)
  			{
       			res = 1 ;
       			if (Diagno) printf("Power %s = %ld\n", kanal, value  );
       			vals[j] = value ; /* Power_1 */
  			}
  			else
      			if (Diagno) printf("Brak danych \n") ;
  		}
	}
	if (Simple)
	{
	    free(vals);
		return;
	}

	/* Tablica usredniajaca */
	if (AVG!=NULL){
//		printf("\n\n\njestem %d %d l %d\n\n\n",go,avgwindow,ll_);
		for (i=0;i<VTlen;i++){
				AVG[i * avgwindow + ll_] = vals [i] ;
				vals[i] = 0;
		}
		
		for (i = 0; i < VTlen;i++){
			Vals2[i] = 0;
			Vctr[i] = 0;
		}
	
		for (i=0;i<VTlen;i++){
			for (zz = 0; zz < avgwindow;zz++){
				Vals2[i] += (int)AVG[i * avgwindow + zz] ;
			 	if ((int)AVG[i * avgwindow + zz] != SZARP_NO_DATA) Vctr[i]++;
			}
		}	
	
		for (i = 0; i < VTlen;i++){
			if (Vctr[i] == 0){
				vals[i] = SZARP_NO_DATA;
			}else{
				vals[i] = (short)Vals2[i] / (short)Vctr[i];
				if ((vals[i]<0)&&(vals[i]!=SZARP_NO_DATA)) vals[i] = SZARP_NO_DATA ;
			}
		}
		
		if (go==0){
			for (i=0;i<VTlen;i++) vals[i] = SZARP_NO_DATA;
		}
		 
		
		ll_++;
		if (ll_>=avgwindow){
			ll_ = 0;
			go = 1;
		}
	}
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
	free(vals);
}

void Usage(char *name)
 {
  printf("DataPaf 3.X  driver; can be used with both COM and Specialix ports\n");
  printf("Usage:\n");
  printf("      %s [s|d] <n> <port>\n", name);
  printf("Where:\n");
  printf("[s|d] - optional simple (s) or diagnostic (d) modes\n");
  printf("<n> - line number (necessary to read line<n>.cfg file)\n");
  printf("<port> - full port name, eg. /dev/term/01 or /dev/term/a12\n");
  printf("Comments:\n");
  printf("No parameters are taken from line<n>.cfg (base period = 1)\n");
  printf("Port parameters: 300 baud, 7E1; signals required: TxD, RxD, GND\n");
  exit(1);
 }


void MakeConfFileName()
{
  code_filename[0] = ser_num[0];
  code_filename[1] = ser_num[1];
  code_filename[2] = ser_num[2];
  code_filename[3] = ser_num[3];
  strcpy(code_path, linedmnpat) ;
  strcat(code_path,"/") ;
  strcat(code_path,code_filename) ;
}

void ReadCfgFile(char *conf_path)
{
	char buf[255] ;
	char *Ptr ;
	int i ;

 	libpar_init_with_filename(conf_path,1);
	/* dodatkowe usrednianie parametrow */
	libpar_readpar("","AVG",buf,sizeof(buf),1);
	avgwindow = atoi(buf) ;
	/* Predkosc urzadzenia */
	libpar_readpar("","BaudRate",buf,sizeof(buf),1);
	BaudRate = atoi(buf) ;
	/* Numer seryjny urzadzenia */
	libpar_readpar("","SerialNumber",ser_num,sizeof(ser_num),1);
	/* Numer portu rs w rejestratorze ( 0 lub 1 ) */	
	libpar_readpar("","RSNumber",buf,sizeof(buf),1);
	code_filename[7] = buf[0] ;
	/* Odczyt energii (1 - tak , 0 - nie) */
	libpar_readpar("","Energy",buf,sizeof(buf),1);
	Energy = atoi(buf) ;
  /* Dzielnik energii ( przez ile ma byc podzielona energia) */
	libpar_readpar("","EnergyDivBy",buf,sizeof(buf),1);
	EnergyDivBy = atoi(buf) ;
  if (EnergyDivBy<=0)
    EnergyDivBy = 1 ;
  /* Odczyt mocy (1 - tak, 0 - nie) */
	libpar_readpar("","Power",buf,sizeof(buf),1);
	Power = atoi(buf) ; 	
 	/* Numery stref czasowych( dotycza tylko energii), energia sumaryczna zawsze znajduje sie w strefie nr 8	*/
	libpar_readpar("","Zones",buf,sizeof(buf),1);
	/* Numery kanalow ( dla energii i mocy ) */
	Ptr = strtok(buf, ",");
	i = 0 ;
  do
  {
      Zones[i] = (char )atoi(Ptr) ;
      Ptr = strtok(NULL, ",");
      i++ ;
  }
  while (Ptr != NULL);
  ZonesCount = i ;
	libpar_readpar("","Channels",buf,sizeof(buf),1);					
	Ptr = strtok(buf, ",");
	i = 0 ;
  do
  {
      Channels[i] = atoi(Ptr) ;
      Ptr = strtok(NULL, ",");
      i++ ;
  }
  while (Ptr != NULL);
  ChannelsCount = i ;
}

int main(int argc,char *argv[])
 {
  unsigned char parind;
  char linedev[30];
  char conf_path[80] ;
  FILE *plik;
  avgwindow = 0;
  ll_ = 0;
  go = 0;
/*  libpar_read_cmdline(&argc, argv);*/
 	libpar_init();
 	libpar_readpar("datapaf", "cfgfile", conf_path, sizeof(conf_path), 1);

  ReadCfgFile(conf_path) ;

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
      printf("Cannot attach shared segment\n");
      exit(-1);
     }
   }

  if ((LineDes=OpenLine(linedev,BaudRate))<0) {
      ErrMessage(2,linedev);

      perror("");
      exit(1);
  }

  SetIOTimeout(LineDes, 10, 0); /* ustaw 1 sekunde oczekiwania */

  MakeConfFileName() ;
  plik=fopen(code_path,"rb");
  if (plik==NULL) {
    printf("Nie moge otworzyc pliku %s \n",code_path) ;
    exit(1) ;
  }
  fread(&tab_code[0][0],sizeof(unsigned long int),12 * 31,plik);
  fclose(plik);
  AVG = NULL;
  Vals2 = NULL;
  if (avgwindow > 0){
 	 AVG = (short *)malloc(VTlen * avgwindow*sizeof(short));
 	 Vals2 = (int *)malloc(VTlen * sizeof(int));
 	 Vctr = (short *)malloc(VTlen *sizeof(short));
  }
  while(1)
  {
       ReadData(linedev);
  }
  if (Vctr!=NULL)
	  free (Vctr);
  if (Vals2!=NULL)
	  free (Vals2);
  if (AVG !=NULL)
 	 free(AVG);
  close(LineDes) ;
  return 0;
 }
