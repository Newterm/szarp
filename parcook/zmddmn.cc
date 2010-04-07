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
 * zmddmn.c
 * Driver licznika LandisGyr ZMD405CT44.2407
 * $Id$
 */
/*
 @description_start
 @class 2
 @devices LandisGyr ZMD405CT44.2407 energy meter.
 @devices.pl Licznik energii elektrycznej LandisGyr ZMD405CT44.2407.
 @config.pl Demon moze na zadany okres czasu zostac zdezaktywowany (zamyka port, wstawia do 
 segmentu pamieci parcook'a wartosci SZARP_NO_DATA ¶pi). Okresy deaktywacji demona okre¶la siê
 za pomoca parametrów lini komend. Np. wywo³anie programu w sposob nastepujacy: 

 zmddmn 3 /dev/ttyS0 -t 12:00 10 -t 18:05 13

 spowoduje, ¿e w godzinach 12:00-12:10 oraz 18:05-18:18 demon nie bedzie aktywny. Ilo¶æ
 okresów deaktywacji jest dowolna.
 @description_end
*/

#define _IPCTOOLS_H_

#include<sys/types.h>
#include<sys/time.h>
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
#include<time.h>
#include "libpar.h"
#include "msgerror.h"
#include "ipcdefines.h"
#include <ctype.h>

#define WRITE_MESSAGE_SIZE 5

#define READ_MESSAGE_SIZE 15000

#define NUMBER_OF_VALS 100

#define DAYSECONDS ( 24 * 60 * 60 ) /* ilosc sekund w dniu */
/*
 * 1 + 1 + 4 + 3 + 4 + 1 + 1
 */

/** element listy przechowywujcej informacje o okresach niekatywnosci demona*/
typedef struct in_period {
	int start_time; /**<poczatek okresu (w sekundach dnia) */
	int length;	/**<dlugosc okresu (w sekundach) */
	struct in_period* next; /**<nastepny element na liscie*/
} in_period_t;

void SetNoData();
void DataToParcook();

static int LineDes = -1;

static int LineNum;

static int SemDes;

static int ShmDes;

static unsigned char Diagno = 0;

static unsigned char Simple = 0;

static int VTlen;

static short *ValTab;

static struct sembuf Sem[2];

static char parcookpat[81];

static char linedmnpat[81];

static unsigned char UnitsOnLine;

#define SIZE_OF_BUF 8

unsigned char outbuf[WRITE_MESSAGE_SIZE] = { '/', '?', '!', '\r', '\n' };

unsigned char DeviceVariableCodes[500][50];
unsigned char DeviceVariableValues[500][50];
unsigned short DeviceVariableCodesCount;
unsigned short DeviceVariableValuesCount;
char AdditionalData[500];

unsigned char ConfigVariableCodes[100][10];
unsigned int ConfigVariableCommas[100];
unsigned int VarCount;
unsigned int CommaCount;

int vals[NUMBER_OF_VALS];

/*
 * Numwery parametrow z pliku konfiguracyjnego landiszmd.cfg 
 */
int buf[5];

void Initialize(unsigned char linenum)
{

	char linedefname[81];
	FILE *linedef;
	int val, val1, val2, val3, val4, val5, val6, i;

	libpar_init();
	libpar_readpar("", "parcook_path", parcookpat, sizeof(parcookpat), 1);
	libpar_readpar("", "linex_cfg", linedmnpat, sizeof(linedmnpat), 1);
	libpar_done();

	sprintf(linedefname, "%s/line%d.cfg", linedmnpat, linenum);
	if ((linedef = fopen(linedefname, "r")) == NULL)
		ErrMessage(2, linedefname);

	if (fscanf(linedef, "%d\n", &val) > 0) {
		if (val > 0)
			UnitsOnLine = val;
		else
			UnitsOnLine = 1;
	} else
		exit(-1);

	VTlen = 0;
	for (i = 0; i < UnitsOnLine; i++) {
		fscanf(linedef, "%u %u %u %u %u %u\n", &val1,
		       &val2, &val3, &val4, &val5, &val6);
		VTlen += val4;
	}

	fclose(linedef);

	if (Diagno)
		printf("Cooking deamon for line %d - Pafal 2EC8 deamon\n",
		       linenum);
	if (Diagno)
		printf
		    ("Unit 1: no code/rapid/subid, parameters: in=2, no out, period=1\n");
	if (Simple)
		return;
	if ((ShmDes =
	     shmget(ftok(linedmnpat, linenum), sizeof(short) * VTlen,
		    00600)) < 0)
		ErrMessage(3, "linedmn");
	if ((SemDes =
	     semget(ftok(parcookpat, SEM_PARCOOK), SEM_LINE + 2 * linenum, 00600)) < 0)
		ErrMessage(5, "parcook sem");
}

int SetIOTimeout(int linedes, int time, int min)
{
	struct termio rsconf;

	ioctl(linedes, TCGETA, &rsconf);
	rsconf.c_cc[VTIME] = time;	/* czekaj co najmniej 5 sekund */
	rsconf.c_cc[VMIN] = min;
	ioctl(linedes, TCSETA, &rsconf);
	return 0;
}

static int OpenLine(char *line)
{
	int linedes;
	struct termio rsconf;
	linedes = open(line, O_RDWR | O_NDELAY | O_NONBLOCK);
	if (linedes < 0)
		return (-1);
	ioctl(linedes, TCGETA, &rsconf);
	rsconf.c_iflag = 0;
	rsconf.c_oflag = 0;
	rsconf.c_cflag = B2400 | CS7 | CLOCAL | CREAD | PARENB;
	rsconf.c_lflag = 0;
	rsconf.c_cc[4] = 0;
	rsconf.c_cc[5] = 0;
	ioctl(linedes, TCSETA, &rsconf);
	return (linedes);
}

void ChangeParB300(int linedes)
{
	struct termio rsconf;
	if (linedes < 0)
		return;
	ioctl(linedes, TCGETA, &rsconf);
	rsconf.c_iflag = 0;
	rsconf.c_oflag = 0;
	rsconf.c_cflag = B2400 | CS7 | CLOCAL | CREAD | PARENB;
	rsconf.c_lflag = 0;
	rsconf.c_cc[4] = 0;
	rsconf.c_cc[5] = 0;
	ioctl(linedes, TCSETA, &rsconf);
	// return(linedes);
}

void ChangeParB9600(int linedes)
{
	struct termio rsconf;
	if (linedes < 0)
		return;
	ioctl(linedes, TCGETA, &rsconf);
	rsconf.c_iflag = 0;
	rsconf.c_oflag = 0;
	rsconf.c_cflag = B2400 | CS7 | CLOCAL | CREAD | PARENB;
	rsconf.c_lflag = 0;
	rsconf.c_cc[4] = 0;
	rsconf.c_cc[5] = 0;
	ioctl(linedes, TCSETA, &rsconf);
	// return(linedes);
}

void
ReadVariable(unsigned char *inbuf, unsigned short length,
	     unsigned short *pos, unsigned char *codebuf,
	     unsigned short *codesize, unsigned char *valuebuf,
	     unsigned short *valuesize, char *AddData)
{
	int start, startc, i;
	memset(codebuf, 0, 50);
	memset(valuebuf, 0, 50);
	*codesize = 0;
	char min_period=0; /* okres minutowy */

	char codebuf_s[] =".4.0";
	char is_codebuf_equal=1; /* czy kody funkcji sa rowne */
	char _dot=0; /* kropka kodu funkcji do parsera */
	char _bracket=0; /*Licznik nawiasow w parserze*/
	char _digit; /* licznik cyferek */
	short i_s ; /* wskaznik pomocniczy */
	short i_s_s;
	start = 0;
	startc = 1;
	i = 0;
	*AddData=0;
	while (*pos < length) {
		if (inbuf[*pos] == ')' || inbuf[*pos] == '\r'
		    || inbuf[*pos] == '\n') {
			start = 0;
			startc = 0;
		}
		if (inbuf[*pos] == '*')
			start = 0;
		if (inbuf[*pos] == '(') {
			start = 1;
			startc = 0;
			*valuesize = 0;
		}
		if (start && inbuf[*pos] != '(')
			valuebuf[(*valuesize)++] = inbuf[*pos];
		if (startc)
			codebuf[(*codesize)++] = inbuf[*pos];
		(*pos)++;
		i++;
		if (inbuf[(*pos) - 1] == '\n') {
			valuebuf[*valuesize] = '\0';
			codebuf[*codesize] = '\0';
			i_s = (short)strlen((char*)codebuf)	;
			i_s_s = (short)strlen(codebuf_s);
			_dot=0;		
			while (i_s>=0){
				if (codebuf[i_s]=='.') _dot++;
				if (_dot>2) _dot=2;
				if ((codebuf[i_s]!=codebuf_s[i_s_s])&&(_dot<2)){
					is_codebuf_equal = 0;  
				}
				i_s--;
				i_s_s--;
				
			}
		
			i_s = (short)*pos;
			min_period=0;
			*AddData=0;
			_digit=1;
			_bracket=0;
			if (is_codebuf_equal){
				while (i_s>=0){
					if (inbuf[i_s]==')') _bracket++;
					if (_bracket>3) _bracket=3;
					if (_bracket==2){
						if (inbuf[i_s]=='(') _bracket=3;
						if ( isdigit(inbuf[i_s]) ){
							min_period+= _digit*(inbuf [i_s] - 0x30 );
							_digit*=10;
						}
					}
					i_s--;
				}
			}
			*AddData=min_period;
			return;
		}
	}
}

/*
 * Umieszczenie w buforze buf danych z licznika energii zawartych w
 * komunikacie inbuf
 */
void
PartitionMessage(unsigned char *inbuf, unsigned short length, int *buf,
		 unsigned short *size)
{
	int i, j, k, l;
	unsigned short pos;
	pos = 0;
	i = 0;
	while (pos < length && inbuf[pos] != '!') {
		ReadVariable(inbuf, length, &pos, DeviceVariableCodes[i],
			     &DeviceVariableCodesCount, DeviceVariableValues[i],
			     &DeviceVariableValuesCount,&AdditionalData[i]);
		i++;
	}
	*size = i;
	k = 0;
	for (i = 0; i < *size; i++) {
		for (j = 0; j < VarCount; j++) {
			if (strcmp
			    ((char*)DeviceVariableCodes[i],
			     (char*)ConfigVariableCodes[j]) == 0) {
				buf[k] = atoi((char*)DeviceVariableValues[i]);
				if (ConfigVariableCommas[j] != 0)
					for (l = 0; l < ConfigVariableCommas[j];
					     l++) {
						buf[k] = buf[k] / 10;
					}
					if (AdditionalData[i])
					buf[k]=(int)((float)buf[k]*15.0/(float)AdditionalData[i]);
				k++;
			}
		}
	}
	*size = k;
	if (Diagno)
		printf(" %s,%s,%s,%s,%s,%s,%s,%s\n", DeviceVariableCodes[4],
		       DeviceVariableValues[4], DeviceVariableCodes[5],
		       DeviceVariableValues[5], DeviceVariableCodes[6],
		       DeviceVariableValues[6], DeviceVariableCodes[7],
		       DeviceVariableValues[7]);
}

 /*
  * zwraca dlugosc wczytanej komendy 
  */
int ReadOutput(int sock, int *buf, unsigned short *size)
{
	int i, j, res;
	unsigned short lenght;
	unsigned char inbuf[READ_MESSAGE_SIZE];
	/*
	 * ustalenie dok³adnej postaci ramki - ilo¶æ odczytywanych rejestrów
	 * (number) oraz adres pocz±tkowego rejestru odczytywanych danych
	 */
	errno = 0;
	lenght = 180;
	memset(inbuf, 0, READ_MESSAGE_SIZE);
	/*
	 * wys³anie komunikatu do urz±dzenia 
	 */
	ChangeParB300(sock);
	i = write(sock, outbuf, WRITE_MESSAGE_SIZE);
	if (Diagno)
		printf("frame %c%c%c%c%c was sended \n", outbuf[0], outbuf[1],
		       outbuf[2], outbuf[3], outbuf[4]);
	if (i < 0)
		return -1;
	sleep(3);
	i = 0;
	do {
		res = read(sock, &inbuf[i], 1);
		usleep(3000);	/* pkolega zwiekszam 3krotnie z 1000 na
				 * 3000 */
		if (res > 0)
			i++;
	}
	while (res > 0);
	if (i == 0) {
		if (Diagno)
			printf("Error %d bytes was received!\n", i);
		return -1;
	}
	for (j = 0; j < i; j++)
		if (Diagno)
			printf("%c", inbuf[j]);
	if (Diagno)
		printf(" was received \n");
	// sleep(2) ; 
	PartitionMessage(inbuf, i, buf, size);
	// if (errno) return -1;
	return i;
}

void ReadData(char *linedev)
{
	unsigned short size;
	int j, res;

	SetNoData();
	j = 0;
	res = -1;

	while (j < 5 && res < 0) {

		if (ReadOutput(LineDes, vals, &size) > 0) {
			res = 1;
			if (Diagno)
				printf("Odczyt danych: ");
			if (Diagno) {
				printf
				    ("vals[0] = %d , vals[1] = %d , vals[2] = %d , vals[3] = %d, vals[4] = %d, vals[5] = %d, vals[6] = %d, vals[7] = %d, vals[8] = %d\n",
				     vals[0], vals[1], vals[2], vals[3],
				     vals[4], vals[5], vals[6], vals[7],
				     vals[8]);
			}
		}

		else {
			vals[0] = vals[1] = vals[2] = vals[3] = vals[4] =
			    vals[5] = vals[6] = vals[7] = vals[8] = SZARP_NO_DATA;
			if (Diagno)
				printf
				    ("Brak danych: vals[0] = vals[1] = vals[2] = vals[3] = vals[4] = SZARP_NO_DATA \n");
		}
		sleep(2);
		j++;
	}

	/*
	 * UWAGA - usrednianie 
	 */
	/*
	 * oryginalnie wszystko fajnie pod warunkiem, ze sterownik nie
	 * przesyla dla jakiegokolwiek parametru wartosci SZARP_NO_DATA - jesli tak, 
	 * to trzeba trzymac takze tablice SumCnt dla kazdego parametru 
	 */
	/*
	 * pkolega Ze wzgledu na to ze wartosci Val[2], val[3] sa wieksze od
	 * 32767 sa dzielone przez 1000 
	 */
	vals[2] = vals[2] / 1000;
	vals[3] = vals[3] / 1000;
	if (Simple)
		return;
	DataToParcook();
}

/** copies localy stored data to parcook's shared segment */
void DataToParcook()
{
	int i;
	Sem[0].sem_num = SEM_LINE + 2 * (LineNum - 1) + 1;
	Sem[0].sem_op = 1;
	Sem[1].sem_num = SEM_LINE + 2 * (LineNum - 1);
	Sem[1].sem_op = 0;
	Sem[0].sem_flg = Sem[1].sem_flg = SEM_UNDO;
	semop(SemDes, Sem, 2);
	for (i = 0; i < VTlen; i++)
		ValTab[i] = (short)vals[i];
	Sem[0].sem_num = SEM_LINE + 2 * (LineNum - 1) + 1;
	Sem[0].sem_op = -1;
	Sem[0].sem_flg = SEM_UNDO;
	semop(SemDes, Sem, 1);
}

void SetNoData()
{
	vals[0] = vals[1] = vals[2] = vals[3] = vals[4] = SZARP_NO_DATA;
}

void ReadCfgFile(char *conf_path)
{
	char buf[1000];
	unsigned char *Ptr;
	int i;

	libpar_init_with_filename(conf_path, 1);
	/*
	 * Cody zmiennych 
	 */

	libpar_readpar("", "VariableCodes", buf, sizeof(buf), 1);
	Ptr = (unsigned char *)strtok(buf, ",");
	i = 0;
	do {
		strcpy((char*)ConfigVariableCodes[i], (char*)Ptr);
		Ptr = (unsigned char*)strtok(NULL, ",");
		i++;
	}
	while (Ptr != NULL);
	VarCount = i;

	libpar_readpar("", "Commas", buf, sizeof(buf), 1);
	Ptr = (unsigned char *)strtok((char*)buf, ",");
	i = 0;
	do {
		ConfigVariableCommas[i] = atoi((char*)Ptr);
		Ptr = (unsigned char*)strtok(NULL, ",");
		i++;
	}
	while (Ptr != NULL);
	CommaCount = i;
	if (VarCount != CommaCount) {
		printf
		    ("Number of definied variabes and commas' posiotons are not equal in config file\n");
		exit(-1);
	}
}

void Usage(char *name)
{
	printf
	    ("Pafal 2EC8 driver; can be used on both COM and Specialix ports\n");
	printf("Usage:\n");
	printf("      %s [s|d] <n> <port> (-t hh:mm len)*\n", name);
	printf("Where:\n");
	printf("[s|d] - optional simple (s) or diagnostic (d) modes\n");
	printf("<n> - line number (necessary to read line<n>.cfg file)\n");
	printf("<port> - full port name, eg. /dev/term/01 or /dev/term/a12\n");
	printf
	    ("hh:mm len - optional parameters specifying daemon's inactivity period\n");
	printf
	    ("	hh:mm - indicates start of daemon's inactivity period (hh-hour,mm-minute)\n");
	printf("	len - length of inactivity period (in minutes)\n");
	printf("Comments:\n");
	printf("No parameters are taken from line<n>.cfg (base period = 1)\n");
	printf
	    ("Port parameters: 300 baud, 7E1; signals required: TxD, RxD, GND\n");
	exit(1);
}

/** Przeglada zadana tabele napisow w poszukiwaniu trojek argumentow 
 * wygladajacych nastepujaco: pierwszy to -t, drugi postaci hh:mm (hh-godzina,
 * mm - minuta), trzeci to liczba calkowita z zakresu [0;24*60), drugi argument
 * spefcyfikuje start okresu nieaktywnosci demona, trzeci - dlugosc tego okresu
 * w minutach 
 * @param argl - lista napisow
 * @param argc - ilosc elementow na liscie
 * @param result - argument wyjsciowy, zawiera liste znalezionych okresow nieaktywnosci
 * @return 0 jezeli 2 argumenty wystepujace bezposrednio po -t nie pasuja do powyzszej specyfikacji*/
int GrabPeriods(char* argl[], int argc, in_period_t** result)
{
	int hour;
	int minute;
	int length_in_minutes;
	*result = NULL;
	in_period_t* period;
	int i = 1;
	while (i < argc) {
		if (strcmp(argl[i],"-t")) {
			i++;
			continue;
		}	
		if (i + 2 >= argc) /*sprawdz czy jest wystarczajaca liczba parametrow*/
			goto bad;

		if ((sscanf(argl[i + 1], "%d:%d", &hour, &minute) != 2)
		    || (hour < 0) || (hour > 23)
		    || (minute < 0) || (minute > 59)) 
			goto bad;

		if (sscanf(argl[i + 2], "%d", &length_in_minutes) != 1) 
			goto bad;

		if ((length_in_minutes < 0) || (length_in_minutes >= 24 * 60)) {
			goto bad;
		}
		if (Diagno)
			printf("Inactivity period start time: %d:%d length: %d\n", hour,
			       minute, length_in_minutes);

		in_period_t* period = (in_period_t*) malloc(sizeof(in_period_t));
		period->start_time = ((hour * 60) + minute) * 60;
		period->length = length_in_minutes * 60;
		period->next = *result;
		*result = period;

		i+=3;
	}
	return 0;
bad:
	while (*result) {
		period = *result;
		*result = (*result)->next;
		free(period);
	}
	return -1;
}

/**Przegl±da podan± listê okresów niekatywno¶ci porównuj±c jej elementy z obecnym czasem.
 * Zwraca ilo¶æ sekund na jak± demon powinien siê zdezaktywowaæ.
 * @param list - lista okresów niekatyno¶æi
 * @return ilo¶æ sekund jak± demon powinien pozostaæ niekatywny*/
int CalcWaitTime(in_period_t *list) {

	in_period_t* period = list;

	while (period) {
		int p_start = period->start_time;
		int p_length = period->length;
		time_t cur_time;
		struct tm *cur_time_tm;
		int t;
		int secdiff;
		cur_time = time(NULL);
		if (cur_time != (time_t) - 1) {
			cur_time_tm = localtime(&cur_time);
			t = (cur_time_tm->tm_hour * 60 +
			     cur_time_tm->tm_min) * 60 +
			    cur_time_tm->tm_sec;

			secdiff = (t - p_start) % DAYSECONDS;
			if (secdiff < 0)
				secdiff += DAYSECONDS;
			if (secdiff < p_length) 
				return p_length - secdiff;
		} else {
			if (Diagno) 
				printf("Cannot get current time");
			return 0;
		}
		period = period->next;
	}
	return 0;
}


int main(int argc, char *argv[])
{
	in_period_t* periods = NULL;	/*lista okresów niekatywno¶ci*/
	int parind;
	char linedev[30];
	char conf_path[80];
	/*
	 * FILE *plik;
	 */

	/*
	 * libpar_read_cmdline(&argc, argv);
	 */

	libpar_init();
	libpar_readpar("landiszmd", "cfgfile", conf_path, sizeof(conf_path), 1);

	ReadCfgFile(conf_path);

	if (argc < 3)
		Usage(argv[0]);
	Diagno = (argv[1][0] == 'd');
	Simple = (argv[1][0] == 's');
	if (Simple)
		Diagno = 1;
	if (Diagno)
		parind = 2;
	else
		parind = 1;
	if (argc < (int)parind + 2)
		Usage(argv[0]);
	LineNum = atoi(argv[parind]);
	strcpy(linedev, argv[parind + 1]);
	if (GrabPeriods(argv, argc, &periods)) {
		printf("Invalid inactivity time spefication\n");
		exit(1);
	}

	Initialize(LineNum);
	LineNum--;
	if (!Simple) {
		if ((ValTab =
		     (short *)shmat(ShmDes, (void *)0, 0)) == (void *)-1) {
			printf("Can not attach shared segment\n");
			exit(-1);
		}
	}

	while (1) {
		int wait_time;
		while ((wait_time = CalcWaitTime(periods))) {
			if (!Simple) {
				SetNoData();
				DataToParcook();
			}	
			if ( LineDes >= 0) {
				close(LineDes);
				LineDes = -1;
			}
			if (Diagno)
				printf("Sleeping for %d second(s)\n", wait_time);
			sleep(wait_time);
		}
		if (LineDes < 0) {
			LineDes = OpenLine(linedev);
			if (LineDes < 0) {
				ErrMessage(2, linedev);
				perror("");
				exit(1);
			}
			SetIOTimeout(LineDes, 10, 0);
		}	
		ReadData(linedev);
		sleep(5);
	}
	close(LineDes);
	return 0;
}
