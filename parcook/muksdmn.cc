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
 * MUKSdmn 
 * Pawe³ Kolega
 * $Id$
 * demon dla kot³a olejowego KOG6
 * dla sterownika MUKS firmy N&N oraz firmy Merpro
 * Demon powstal bardziej na podstawie Reverse Engineeringu niz dokumentacji, ktora jest niedostateczna!
 */
/*
 @description_start
 @class 4
 @devices MUKS-MERPRO PLC for KOG6 oil boiler.
 @devices.pl Sterownik MUKS-MERPRO kot³a olejowego KOG6.
 @description_end
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <termio.h>
#include <fcntl.h>

#include "ipchandler.h"
#include "liblog.h"
#include "conversion.h"
#include "custom_assert.h"

#define DAEMON_ERROR 1

#define GENERAL_ERROR -1
#define TIMEOUT_ERROR -2
#define PACKET_SIZE_ERROR -1

#define NO_PARITY 0
#define EVEN 1
#define ODD 2

/* Identyfikatory zapytañ */
#define P012_ORDER 01 /* CIS W WYLOT /MPa  */
#define T014_ORDER 02 /* TEM W ZASIL /C  */
#define P015_ORDER 03 /* CIS W ZASIL /MPa  */
#define T016_ORDER 04 /* TEM S Z KOTLA /C  */
#define P018_ORDER 05 /* CIS S PALEN /kPa  */
#define T019_ORDER 06 /* TEM W WYLOT /C  */
#define F025_ORDER 07 /* PRZ W WYLOT /m3/s  */

#define F125_ORDER 33 /* PRZEP W WYL /kg/s  */
#define U225_ORDER 34 /* MOC /MW  */
#define U325_ORDER 35 /* ENERGIA /GJ */
#define U425_ORDER 36 /* MASA /GJ */

#define HOW_QUERIES 11 /* ILOSC zapytan */

#define MAX_RESPONSE 255

#define YES 1
#define NO 0

#define OK 1
#define FAIL 0
#define EMPTY -1

/**
 * MUKS communication config.
 */
class           MUKS {
      public:
	/** info about single parameter */
				/**< array of params to read */
	int             m_params_count;
				/**< size of params array */
				  /**< virtual memory map */
	int             m_sends_count;
				/**< size of sends array */

	/**
	 * @param params number of params to read
	 * @param sends number of params to send (write)
	 */
	MUKS(int params, int sends) {
		ASSERT(params >= 0);
		ASSERT(sends >= 0);

		m_params_count = params;
		m_sends_count = sends;
	}

	~MUKS() {
	}



	/**
	 * Filling m_read structure SZARP_NO_DATA value
	 */
	void            SetNoData(IPCHandler * ipc);

	/**
	 * Function oppening serial port special for LECstat E Heat Meter
	 * @param Device Path to device np "/dev/ttyS0"
	 * @param BaudRate baud rate
	 * @param StopBits stop bits 1 od 2
	 * @param Parity parity : NO_PARITY, ODD, EVEN 
	 * @param return file descriptor
	 */

	int InitComm(const char *Device, long BaudRate, unsigned char DataBits,
	     unsigned char StopBits, unsigned char Parity);

	/**
	 * Function gets response from serial port
	 * @param fd File descriptor
	 * @param Response  Pointer to buffer
	 * @param return >0 - how bytes read, <=0 error
	 */

	int GetResponse(int fd, char *Response);

	/**
	 * Creates LEC - Asi Query
	 * @param order order
	 * @param net_id net identifier (default 0xff)
	 * @return pointer to query. User must self frees this pointer !malloc
	 */
	 char *CreateQuery(unsigned char order);
	/**
	 * Sends query
	 * @param fd file descriptor of comm port
	 * @param order order type
	 * @return OK or FAIL
	 */
	void SendQuery(int fd,unsigned char order);

	/**
	 * Calculates query size
	 * @param order order type
	 * @return size of query
	 */
	short QuerySize(unsigned char order);

	
	/**
	 * Scans incomming packet for errors
	 * @param *Packet incomming packet
	 * @param order order in query
	 * @param size size of query
	 * @param MaxResponse maximum allowed size of incomming packet
	 * @return OK or FAIL
	 */
	char CheckResponse (char *Packet, unsigned char order, unsigned short size, unsigned short MaxResponse);

	/**
	 * returns index of order in response.
	 * @param *Packet incomming packet
	 * @param order order in query
	 * @param size size of query
	 * @param MaxResponse maximum allowed size of incomming packet
	 * @return position or EMPTY
	 */
	short FindOrderInResponse(char *Packet, unsigned char order, unsigned short size);
	
	/**
	 * Scans incomming packet for errors
	 * @param *Packet incomming packet
	 * @param order order in query
	 * @param size size of query
	 * @param MaxResponse maximum allowed size of incomming packet
	 * @param status OK or FAIL
	 * @return parsed value
	 */
	 int ParsePacket (char *Packet, unsigned char order, unsigned short size, unsigned short MaxResponse, char *status) ;

	 

      private:
	short my_index(char *str1, char *str2);
};


void MUKS::SetNoData(IPCHandler * ipc)
{
	int i;

	for (i = 0; i < ipc->m_params_count; i++)
		ipc->m_read[i] = SZARP_NO_DATA;

}

int MUKS::InitComm(const char *Device, long BaudRate, unsigned char DataBits,
	     unsigned char StopBits, unsigned char Parity)
{
	int             CommId;
	long            BaudRateStatus;
	long            DataBitsStatus;
	long            StopBitsStatus;
	long            ParityStatus;
	struct termios  rsconf;
//	int serial;
	CommId = open(Device, O_RDWR | O_NDELAY |O_NONBLOCK);
	if (CommId < 0)
		return CommId;

	switch (BaudRate) {
		case 300:
			BaudRateStatus = B300;
			break;
		case 600:
			BaudRateStatus = B600;
			break;
		case 1200:
			BaudRateStatus = B1200;
			break;
		case 2400:
			BaudRateStatus = B2400;
			break;
		case 4800:
			BaudRateStatus = B4800;
			break;
		case 9600:
			BaudRateStatus = B9600;
			break;
		case 19200:
			BaudRateStatus = B19200;
			break;
		case 38400:
			BaudRateStatus = B38400;
			break;
		case 115200:
			BaudRateStatus = B115200;
			break;
		default:
			BaudRateStatus = B9600;
			break;
	}

	switch (DataBits) {
		case 7:
			DataBitsStatus = CS7;
			break;
		case 8:
			DataBitsStatus = CS8;
			break;
		default:
			DataBitsStatus = CS8;
			break;
	}

	switch (StopBits) {
		case 1:
			StopBitsStatus = 0;
			break;
		case 2:
			StopBitsStatus = CSTOPB;
			break;
		default:
			StopBitsStatus = 0;
			break;
	}

	switch (Parity) {
		case NO_PARITY:
			ParityStatus = 0;
			break;
		case EVEN:
			ParityStatus = PARENB;
			break;
		case ODD:
			ParityStatus = PARENB | PARODD;
			break;
		default:
			ParityStatus = 0;
			break;
	}
	tcgetattr(CommId, &rsconf);
	rsconf.c_cflag =
		BaudRateStatus | DataBitsStatus | StopBitsStatus |
		ParityStatus | CLOCAL | CREAD ;

	rsconf.c_iflag = 0;
	rsconf.c_oflag = 0;
	rsconf.c_lflag = 0;
	rsconf.c_cc[VMIN] = 1;
	rsconf.c_cc[VTIME] = 0;
	tcsetattr(CommId, TCSANOW, &rsconf);
//	ioctl(CommId, TIOCMGET, &serial);
//	serial |= TIOCM_DTR; //DTR is High
//	serial |= TIOCM_RTS; //RTS is High
//	ioctl(CommId, TIOCMSET, &serial);
	return CommId;
}

int MUKS::GetResponse(int fd, char *Response)
{
 fd_set rfds;
 struct timeval tv;
 int i,j;
 int retval;
 
 FD_ZERO(&rfds);
 FD_SET(fd, &rfds);
 tv.tv_sec = 5;
 tv.tv_usec = 0;
 retval = select(fd+1, &rfds, NULL, NULL, &tv);
 if (retval == -1){
 	perror("select()");
	return GENERAL_ERROR ;
 }
 else
     if (retval) {
	j = 0;
       	for (i = 0; read(fd, &Response[i], 1) == 1; i++) {
		usleep(1000);
		j++ ;
	}
	return (j);
     }
     else
     {   return TIMEOUT_ERROR ;
	    
     }

}

char *MUKS::CreateQuery(unsigned char order)
{
 char *buffer;
 if (order>100) return NULL ; //Jaki¶ babol
 asprintf(&buffer,"C%02d\r",order);
 return buffer ;
}

short MUKS::QuerySize(unsigned char order){
	short len;
	char *b;
	b = CreateQuery(order);
	len = strlen(b);
	free (b);
	return (len);
}


void MUKS::SendQuery(int fd,unsigned char order)
{
 char *buffer;
 buffer = CreateQuery(order);
 write(fd, buffer, sizeof(buffer));
 free (buffer);
}


short MUKS::my_index(char *str1, char *str2){
	char * str3;
	short i,max_offset;
	short index;
	index = EMPTY;
	if (!strcmp(str1,str2)) return 0;
	if (strlen(str1)<=strlen(str2)) return EMPTY;
	str3=(char *)malloc(sizeof(char)*strlen(str2)+1);
	max_offset = strlen(str1)-strlen(str2);
	for (i=0;i<=max_offset;i++){
		memcpy(str3, (str1+i), strlen(str2));
		str3[strlen(str2)]=0;
		if(!strcmp(str3, str2)){
			index=i;
			break;
                }														        }
	    free (str3);
	    return index;
}

short MUKS::FindOrderInResponse(char *Packet, unsigned char order, unsigned short size){
	short len;
	char *buff1;
	//Protokol wydaje sie miec bledy dlatego sa rozne wyjatki od reul
	buff1 = CreateQuery(order);
	buff1[strlen(buff1)]=0;
	if (size <= strlen(buff1)){
		free(buff1);
		return FAIL;
	}
	len = my_index(Packet, buff1);
	if (len<0){ 
		free (buff1);
		return FAIL;
	}
	free(buff1);
	return len;	

}

char MUKS::CheckResponse (char *Packet, unsigned char order, unsigned short size, unsigned short MaxResponse)
{
	if (size == 0) return FAIL;
	if  (FindOrderInResponse(Packet, order, size) == EMPTY) return FAIL;
	if ((Packet[size-1]!=0x3E) && (Packet[size-1]!=0x0A)) return FAIL;
	return OK ;
}

/* Parsuje pakiet do postaci liczbowej */
int MUKS::ParsePacket (char *Packet, unsigned char order, unsigned short size, unsigned short MaxResponse, char *status)
{
	char *buffer;
	char *tmp;
	 int  b ;
	short i,j ;
	short response_offset;
	char zero_packet; /* Czy mamy do czynienia z pakietem zerowym */
	char was_here;
	if (CheckResponse(Packet, order, size, MaxResponse)==FAIL){
		*status = FAIL;
		return 0;
	}
	*status = OK;
	/* Allokuje tyle pamiêci ile ma pakiet */
	buffer = (char *)malloc(sizeof(char)*size);
	memset(buffer,0x0,size);
	/* Przepisuje wszystkie warto¶ci pomiêdzy 0xBC które spe³niaj± isdigit  */
	response_offset = FindOrderInResponse(Packet, order,size);
	i = QuerySize(order) + response_offset + 1 ;//13/10
	zero_packet = NO;
	was_here=NO;
	if (response_offset > 0) zero_packet = YES;
	j = 0;
	/* Jest taki glupi pakiet ktory ma na poczatku 0 a potem ma jakas wartosc albo nie ma nic - wtedy trzeba szukac do pierwszego /13  */
	while ((i<size)&&(Packet[i]!=0x3E)&&(Packet[i]!=0x0A)&&(Packet[i]!=0x0D)){
//		fprintf(stderr,"lelo %d %c %x\n", Packet[i],Packet[i],Packet[i]);	
		if (!isdigit(Packet[i])&&(Packet[i]!='-')&&(Packet[i]!='.')) *status = FAIL;
		
		 if (isdigit(Packet[i]) || (Packet[i]=='-')){ /* Pomijamy kropeczki i inne znaki pomiedzy cyferkami  */
			 was_here = YES;
			 buffer[j] = Packet[i] ;
			 j++ ;
		}
		i++ ;
	}
	buffer[j] = 0 ;
//	fprintf(stderr,"lelo %s\n",buffer);
//	b = (unsigned int)atoi(buffer);
	b = (int)strtol(buffer, &tmp, 10);
	if ((zero_packet==YES) && (was_here==NO)){
		b = 0 ; /* To jest prawdopodobnie przypadek, gdy kociol nie chodzi, ale wartosc moze byc niezerowa! */
		free (buffer);
		return b;
	}
	
	if (tmp[0] != 0) *status = FAIL;
		
	free (buffer);
	return b ;
}

int main(int argc, char *argv[])
{

	DaemonConfig   *cfg;
	MUKS     *muksinfo;
	IPCHandler     *ipc;
	int             fd;
	int 		ResponseStatus;
	//int		i;
	unsigned int	bfr ;
	char status;
	char buf[MAX_RESPONSE];
	unsigned short which_order;
	unsigned char orders[HOW_QUERIES] ={
	P012_ORDER, /* CIS W WYLOT /MPa  */ 
	T014_ORDER, /* TEM W ZASIL /C  */ 
	P015_ORDER, /* CIS W ZASIL /MPa  */ 
	T016_ORDER, /* TEM S Z KOTLA /C  */ 
	P018_ORDER, /* CIS S PALEN /kPa  */ 
	T019_ORDER, /* TEM W WYLOT /C  */ 
	F025_ORDER, /* PRZ W WYLOT /m3/s  */ 

	F125_ORDER, /* PRZEP W WYL /kg/s  */ 
	U225_ORDER, /* MOC /MW  */ 
	U325_ORDER, /* ENERGIA /GJ */ 
	U425_ORDER /* MASA /GJ */ 
	};

	unsigned char orders_str1[][50] ={
	"P012_ORDER", /* CIS W WYLOT /MPa  */ 
	"T014_ORDER", /* TEM W ZASIL /C  */ 
	"P015_ORDER", /* CIS W ZASIL /MPa  */ 
	"T016_ORDER", /* TEM S Z KOTLA /C  */ 
	"P018_ORDER", /* CIS S PALEN /kPa  */ 
	"T019_ORDER", /* TEM W WYLOT /C  */ 
	"F025_ORDER", /* PRZ W WYLOT /m3/s  */ 

	"F125_ORDER", /* PRZEP W WYL /kg/s  */ 
	"U225_ORDER", /* MOC /MW  */ 
	"U325_ORDER", /* ENERGIA /GJ */ 
	"U425_ORDER" /* MASA /GJ */ 
	};

	unsigned char orders_str2[][50] ={\
	  "CIS W WYLOT /MPa", 
	  "TEM W ZASIL /C",  
	  "CIS W ZASIL /MPa",  
	  "TEM S Z KOTLA /C",
	  "CIS S PALEN /kPa",
	  "TEM W WYLOT /C",
	  "PRZ W WYLOT /m3/s",

	  "PRZEP W WYL /kg/s",
	  "MOC /MW", 
	  "ENERGIA /GJ",
	  "MASA /GJ",
	};
	
	cfg = new DaemonConfig("muksdmn");

	if (cfg->Load(&argc, argv)) {
		sz_log(0, "Error while loading configuration, exiting.");
		return 1;
	muksinfo = new MUKS(cfg->GetDevice()->
				GetFirstUnit()->GetParamsCount(),
				cfg->GetDevice()->
				GetFirstUnit()->GetSendParamsCount());

	if (cfg->GetSingle()) {
		printf("\
line number: %d\n\
device: %ls\n\
params in: %d\n", cfg->GetLineNumber(), cfg->GetDevice()->GetPath().c_str(), muksinfo->m_params_count);
	}
	
	try {
		ipc = new IPCHandler(m_cfg);
	} catch(const std::exception& e) {
		sz_log(0, "Error while initializing IPC: %s, exiting.", e.what());
		return 1;
	}

	
	sz_log(2, "starting main loop");
	muksinfo->SetNoData(ipc);
	while (true) {
		fd = muksinfo->InitComm(SC::S2A(cfg->GetDevice()->GetPath()).c_str(), cfg->GetDevice()->GetSpeed(), 8, 2, NO_PARITY);
		if (fd<0){
			sz_log(0, "file descriptor <0! crashed");
			return 0;
		}
		
		for (which_order=0;which_order<HOW_QUERIES;which_order++){
			muksinfo->SendQuery(fd, orders[which_order]);
			if (cfg->GetSingle()){
				fprintf(stderr,"Sending : %s (%s) Query\n",orders_str1[which_order], orders_str2[which_order]);
			}
			usleep(1000*10);
			memset(buf,0x0,sizeof(buf));
			ResponseStatus = muksinfo-> GetResponse(fd,buf);
			if (ResponseStatus <= 0 ) status = FAIL;
			if (ResponseStatus > MAX_RESPONSE ) status = FAIL;
			
			if (status == FAIL){
				if (cfg->GetSingle()){
					fprintf(stderr,"FAILED\n");
				}
				ipc->m_read[2 * which_order] = SZARP_NO_DATA ;
				ipc->m_read[2 * which_order + 1] = SZARP_NO_DATA ;
			}
			else{
				bfr = muksinfo->ParsePacket(buf, orders[which_order], ResponseStatus, MAX_RESPONSE,&status);
				if (cfg->GetSingle()){
					fprintf(stderr,"%s (%s) is %d\n",orders_str1[which_order], orders_str2[which_order],bfr);
				}
				ipc->m_read[2 * which_order]=(int)(bfr&0xffff);
				ipc->m_read[2 * which_order + 1]=(int)(bfr>>16);
						      
			}
			sleep(1);
		}

		ipc->GoParcook();
		sleep(10); //Wait 10s
		close(fd);
	}
//	free (ParsedData);
	return 0;
}
