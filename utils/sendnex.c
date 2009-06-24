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
 * SendNex for Unix/Curses
 *  based on 28.10.1998 Codematic version 
 * SterKom 
 *  04.01.2005 by Marcin Czenko
 * 
 * gcc sendnex.c -o sendnex
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/termios.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

extern int errno;

/* #define DEBUG */

#define DEVICE "/dev/ttyS0"
#define ZET_SPEED 9600
#define SK_SPEED 19200

// czas oczekiwania WAIT_TIME*0.1 sekundy
#define WAIT_TIME	10	

char hexname[]="test.hex";

FILE *hexfile;

char hexrec[1024];

int toint(int x)
 {
  if (isdigit(x))
	return(x-'0');
  return(10+tolower(x)-'a');
 }

static int OpenLine(char *line, int baud, int stopb)
 {
  int linedes;
  struct termios rsconf;
  
  linedes=open(line,O_RDWR); 
  if (linedes<0)
    return(-1);

  // pobierz poprzednie ustawienia dla struktury termio
  tcgetattr(linedes,&rsconf);
  
  // zamieniaj odebrane CR na NL - to by³ tylko test - ale to naprawde dziala
  rsconf.c_iflag=0;
  //rsconf.c_iflag=ICRNL;
  
  // zadnych specjalnych ustawien dla wyjscia
  rsconf.c_oflag=0;
  
  // pracujemy w tzw <<non-canonical mode>> czyli nie odbieramy ca³ych wierszy
  // a pojedyncze znaki zgodnie z ustawieniami struktury c_cc (patrz poni¿ej).
  //rsconf.c_lflag &= (~ICANON);
  rsconf.c_lflag = 0;
  
  switch(baud){
    case ZET_SPEED:
		cfsetospeed(&rsconf,B9600);
    	break;

    case SK_SPEED:
		cfsetospeed(&rsconf,B19200);
    	break;
    default:
    	cfsetospeed(&rsconf,B9600);
  }
  switch(stopb){
    case 2:
    	rsconf.c_cflag|=CSTOPB;
		break;
    default:
    	break;
   }
  
  // operacja <<read>> dziala w sposob nastepujacy:
  // Jezeli w czasie TIME*0.10 sekundy nie odebrany zostanie zaden znak
  // funkcja <<read>> wraca. Jezeli przed uplywem tego czasu odebrany
  // zostanie jakikolwiek znak funkcja <<read>> zwroci od razu. 
  rsconf.c_cc[VTIME] = WAIT_TIME;    
  rsconf.c_cc[VMIN] = 0;
    
  // CS8 - 8 bitów danych
  // CLOCAL - je¿eli flaga CLOCAL jest ustawiona stan terminala
  //	      nie zalezy od linii sterowania modemem
  // CREAD - wlacz odbieranie
  rsconf.c_cflag|=CS8|CLOCAL|CREAD;
    
  tcsetattr(linedes,TCSANOW,&rsconf);

  return(linedes);
 }


int GetAnswer(int linedes, char * buf,int size)
{
	int i=0, tord, ret ;	
	char c ;

	tord = 1 ;
	do{		
		ret = read(linedes,&c,tord) ;		
		if(ret==-1)
			perror("GetAnswer");
		if(ret!=tord)
			return 0 ;		
		// odczytano znak		
		if(c!=0x0d && c!=0x0a)
			buf[i++] = c ;		
			
	}while((c != 0x0a) && (i<size)) ;

	buf[i] = '\0' ;

	return 1 ;
}

int main(int argc, char *argv[])
{
	char unit ;
	char Number[] = "123456\0" ;
	char Seq[15] ;
	char RdTab[15] ;
	int i, linedes, towr;
	unsigned int HexFrom, HexTo, HexBeg;
	int BaudRate ;
	int argcindex;
	char *device = (char *)malloc(strlen(DEVICE)+1); 
	strcpy (device,DEVICE);

	setbuf(stdout, 0);

	//  myinit();
	//  atexit(mydone);
  
	//clrscr();
	HexFrom=0;
	HexTo=0xFFFF;
	BaudRate = ZET_SPEED ;
	argcindex = 0;
	//delay_val = 1000; /*opoznienie czasowe miedzy wysylanymi do sterownika danymi*/
	//j = 0 ;
	//hexind=0;
	if (argc<2){
		fprintf(stderr, "Usage: sendnex hexfile [device] [BZET | {BSK}] [unit [start [end]]]\n");
		/* fprintf(stderr, "Usage: techex hexfile [device] [unit [start [end]]]\n"); */
	free (device);
    	exit(-1);
    }

	if (argc>2){
		free (device);
		device = (char *)malloc(strlen(argv[2])+1); 
		strcpy (device,argv[2]);
	}
  	unit='1';
	argcindex = 3 ;
  	if (argc>argcindex) {
		if ((!strcmp(argv[3], "BZET")) || (!strcmp(argv[3],"BSK")) ){ /* Kiedy jest BaudRate w linii polecen */
			if (!strcmp(argv[3], "BZET")){
				BaudRate = ZET_SPEED ;
			}

			if (!strcmp(argv[3], "BSK")){
				BaudRate = SK_SPEED ;
			}
			argcindex++;
		}

		if (argc>argcindex) {	
			unit=(char)argv[argcindex][0]; //3 | 4
			argcindex++;
			if (argc>argcindex){ //4 | 5
				HexFrom=atoi(argv[argcindex]); //4 | 5
				argcindex++;
        			if (argc>argcindex) //5 | 6
          				HexTo=atoi(argv[argcindex]); //5 | 6
			}
   		}
	}
	if (HexTo<HexFrom){
    	fprintf(stderr, "No data to upload, end lower than start %04x < %04x\n", HexTo,
 	   			HexFrom);
	free (device);
     	exit(-1);
  	}
  	linedes = OpenLine(device, BaudRate, 1);
  	if (!linedes) {
  		fprintf(stderr, "Cannot open device ! %s\n", device);
		free (device);
  		exit(0);
  	}

  	if ((hexfile=fopen(argv[1], "r"))==NULL) {
    	fprintf(stderr, "Unable to open input file: %s\n", argv[1]);
	free (device);
    	exit(-1);
  	}

	if (BaudRate == ZET_SPEED)
  		printf("Uploading %s to ZET unit %c in range [%04x,%04x] on device %s\n",
			argv[1], unit, HexFrom, HexTo, device);

	if (BaudRate == SK_SPEED)
  		printf("Uploading %s to SK unit %c in range [%04x,%04x] on device %s\n",
			argv[1], unit, HexFrom, HexTo, device);


	//  mvaddstr(1, 1, scrbuf);refresh();
	
	/* 
	   Nie wiem, czy w linuxie to jest potrzebne, ale na wszelik wypadek
	   wykonuje tutaj procedure o kryptonimie <<kanal>>.
	   Najpierw wysylam do sterownika znak konca ramki (0x03) po to aby
	   sterownik ustawic w stan poczatkowy, po czym odbieram wszystkie
	   znaki z bufora, po to aby nie odbierac potem smieci
	*/

	printf("Preparing communication chanel...");

	Seq[0] = 0x03 ;
	
	if (write(linedes,Seq,1)!=1){
		fprintf(stderr,"\nProblem writing to the terminal !\n");
		free (device);
		exit(-1);
	}
	
	// czytaj smiecie z bufora
	while(read(linedes,hexrec,1)>0) ;
	
	printf("OK\n");
	
	// a teraz wysylamy to co trzeba
	
	// wejscie w tryb programowania
	
	printf("Entering programming mode...");
	
	Seq[0] = 0x02 ;					//^B
	Seq[1] = 'X' ;					//opcja wejscia w tryb programowania
	Seq[2] = unit ;					// numer identyfikacyjny sterownika
	for(i=0 ; i<6 ; i++)
		Seq[i+3] = Number[i] ;		//n0n1n2n3n4n5 - kod dostepu
	i+=3 ;
	Seq[i++] = 0x03 ;				//^C
	Seq[i] = '\0' ;
	
	towr = strlen(Seq) ;			// ilosc znakow do wyslania
	
	if(write(linedes,Seq,towr)!=towr){
		fprintf(stderr,"\nError while transmitting the <<X>> command !\n");
		free (device);
		exit(-1) ;
	}

	// czekaj na odpowied¼
	if(GetAnswer(linedes,RdTab,15)==0){
		fprintf(stderr,"\nX:Oo...something went wrong - no answer! (%s)\n",RdTab);
		free (device);
		exit(-1);
	}
	
	// sprawdz co odpowiedzial
	if(strcmp(RdTab,"OK")!=0){
		fprintf(stderr,"\nX:Expected: <<OK>>\nReceived: %s\n",RdTab);
		free (device);
		exit(-1);
	}
	printf("OK\n");
	close (linedes);
	sleep(1);
	while(fgets(hexrec, 1022, hexfile)){
  		linedes = OpenLine(device, BaudRate, 1);
		HexBeg=toint(hexrec[3])*4096+
	       toint(hexrec[4])*256+
		   toint(hexrec[5])*16+
	       toint(hexrec[6]);
		if (HexBeg<HexFrom)
	  		continue ;
		printf("Address: %04x\r",HexBeg);
		strcat(hexrec, "\n\r");
		towr = strlen(hexrec);
		if(write(linedes,hexrec,towr)!=towr){
			fprintf(stderr,"\nError while transmitting a HEX record !\n");
			free (device);
			exit(-1) ;
		}

		// czekaj na odpowied¼
		if(GetAnswer(linedes,RdTab,15)==0){
			fprintf(stderr,"\nHEX:Oo...something went wrong - no answer! (%s)\n",RdTab);
			free (device);
			exit(-1);
		}
	
		// sprawdz co odpowiedzial
		if(strcmp(RdTab,"OK")!=0){
			fprintf(stderr,"\nHEX:Expected: <<OK>>\nReceived: %s\n",RdTab);
			free (device);
			exit(-1);
		}
	close (linedes);
	}
	sleep(1);
  	linedes = OpenLine(device, BaudRate, 1);
	printf("\nExiting programming mode...");
	
	Seq[1] = 'E' ;					//opcja wyjscia z trybu programowania
	towr = strlen(Seq) ;			// ilosc znakow do wyslania
	
	if(write(linedes,Seq,towr)!=towr){
		fprintf(stderr,"\nError while transmitting the <<E>> command !\n");
		free (device);
		exit(-1) ;
	}

	// czekaj na odpowied¼
	if(GetAnswer(linedes,RdTab,15)==0){
		fprintf(stderr,"\nE:Oo...something went wrong - no answer! (%s)\n",RdTab);
		free (device);
		exit(-1);
	}
	
	// sprawdz co odpowiedzial
	if(strcmp(RdTab,"OK")!=0){
		fprintf(stderr,"\nE:Expected: <<OK>>\nReceived: %s\n",RdTab);
		free (device);
		exit(-1);
	}
	
	printf("OK\n");
	close(linedes);
	free (device);
	return 1 ;
}
 
