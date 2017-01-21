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
 * AQTDMN
 * $Id$
 * demon do odczytu parametrów z przelicznika SuperCal Aquatherm 431 lub 432 
 * Pawe³ Kolega 2005
 */

/*
 @descrption_start

 @class 4

 @devices SuperCal Aquatherm 431 and 432 heatmeters
 @devices.pl Ciep³omierze SuperCal Aquatherm 431/432

 @comment.pl Sczytywane parametry: Energia MSB, Energia LSB, Przep³yw zliczony MSB, Przep³yw zliczony LSB,
 Temperatura zasilania MSB, Temperatura zasilania LSB, Temperatura powrotu MSB, Temperatura powrotu LSB,
 Przep³yw aktualny MSB, Przep³yw aktualny LSB  

 @description_end
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <errno.h>
#include <assert.h>
#include <libxml/tree.h>
#include <termio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>

#define _IPCTOOLS_H_
#define _HELP_H_
#define _ICON_ICON_
#include "szarp.h"
#include "msgtypes.h"

#include "ipchandler.h"
#include "liblog.h"
#include "conversion.h"

#define DAEMON_ERROR 1

#define GENERAL_ERROR -1
#define TIMEOUT_ERROR -2
#define PACKET_SIZE_ERROR -1

#define ENERGY_CALCULATED 1
#define ENERGY_READ 0

#define N_OF_PARAMS 12 /* Bardzo wazne! liczba parametrow */

/* Zapytanie do ciep³omierza ma zawsze 8 bajtów + budz±cy znak 0xFF je¿eli odpowied¼ przyjdzie te¿ 8 lub bajtów tzn ¿e mamy do czynienia z echem i trzeba to zignorowaæ */
#define PACKET_SIZE_8 8
#define PACKET_SIZE_9 9

/* Domy¶lne urz±dzenie ma zawsze adres 0xfe - przy budowie sieci ma to znaczenie, w przypadku jednego cieplomierza nie ma to znaczenia */
#define DEFAULT_DEVICE 0xfe
#define AQT_OFFSET 0x03 /* Offset od którego maj± byæ pobierane dane */

/* Bloki i adresy odczytywanych danych */

#define NO_PARITY 0
#define EVEN 1
#define ODD 2

#define RESPONSE_OK 1
#define RESPONSE_BAD 0

/* Data Offset */
#define DATA_OFFSET 18

/* Data */
#define AQT_DATE_B 	319 
#define AQT_DATE_P 	112
#define AQT_DATE_S	16

/* Licznik energii  */
#define AQT_ENERGY_B	255
#define AQT_ENERGY_P	96
#define AQT_ENERGY_S	32
#define AQT_ENERGY_O	0


/* Kod b³êdu  */
#define AQT_ERRORS_B 	287
#define AQT_ERRORS_P 	0
#define AQT_ERRORS_S 	16

/* jednostki GJ lub MJ */
#define AQT_GJ_MJ_FL_B	63
#define AQT_GJ_MJ_FL_P	42
#define AQT_GJ_MJ_FL_S	1

/* godziny pracy */
#define AQT_HOURS_B 	351
#define AQT_HOURS_P 	12
#define AQT_HOURS_S 	20

/* numer identyfikacyjny  */
#define AQT_IDNUM_B	255
#define AQT_IDNUM_P	4
#define AQT_IDNUM_S	28

/* miejsce monta¿u przetwornika przep³ywu  */
#define AQT_KPOSFL_B 	63
#define AQT_KPOSFL_P 	56
#define AQT_KPOSFL_S 	1

/* wspó³czynnik impulsowania  */
#define AQT_REG09_B 	63
#define AQT_REG09_P	44
#define AQT_REG09_S	4

/* typ i wersja przelicznika  */
#define AQT_RUBNUM_B	1559
#define AQT_RUBNUM_P	64
#define AQT_RUBNUM_S	16

//#define AQT_TEMPH_B 	287
//#define AQT_TEMPH_P 	64
//#define AQT_TEMPH_S 	16
//#define AQT_TEMPH_O 	4

/* temperatura zasilania */
#define AQT_TEMPH_B 	287
#define AQT_TEMPH_P 	64	
#define AQT_TEMPH_S 	16
#define AQT_TEMPH_O 	4


/* temperatura powrotu  */
#define AQT_TEMPL_B 	287
#define AQT_TEMPL_P 	80
#define AQT_TEMPL_S 	16
#define AQT_TEMPL_O 	6

/* przep³yw aktualny  */
#define AQT_FLOW_B 	287
#define AQT_FLOW_P 	96
#define AQT_FLOW_S 	31 
#define AQT_FLOW_O 	8

/* moc aktualna  */
#define AQT_POWER_B 	319
#define AQT_POWER_P 	0
#define AQT_POWER_S 	16
#define AQT_POWER_O 	10


/* czas */
#define AQT_TIME_B	351
#define AQT_TIME_P	0
#define AQT_TIME_S	12

 /* licznik  */
#define AQT_VOLUME_B	287
#define AQT_VOLUME_P	32
#define AQT_VOLUME_S	32
#define AQT_VOLUME_O	2


/* jednostki Wh lub J  */
#define AQT_WH_J_FL_B	63
#define AQT_WH_J_FL_P 	43
#define AQT_WH_J_FL_S 	1


/************************************************************************************************************/
/* KLASY ZAIMPORTOWANE Z SRE2DMN */
/************************************************************************************************************/
#define MAX_TIME 24*3600
#define TIME_OK 1
#define TIME_ERROR -1

#define YES 1
#define NO 0
typedef struct
{
	signed char t_hour;
	signed char t_min ;
	signed char t_sec ;
} ttime;

typedef struct{
	unsigned int t_en;
	unsigned long t_sec;
} tenergy ;

class		MyTime{
	public:
	MyTime () {};
	ttime CreateTime(signed char _t_hour, signed char _t_min, signed char _t_sec) ;
	unsigned long Time2Sec (ttime time);
	unsigned long TimeDiffSec (ttime time_1, ttime time_2);
	unsigned long DiffSec (unsigned long time_1, unsigned long time_2);
	
	signed char CheckTimeError(ttime time);
};

ttime CreateTime(signed char _t_hour, signed char _t_min, signed char _t_sec){
	ttime time ;
	if ((_t_hour < 0 )|| (_t_hour > 23))
	time.t_hour = TIME_ERROR ;
	else
	time.t_hour = _t_hour;
	if ((_t_min < 0 )|| (_t_min > 59))
	time.t_min =  TIME_ERROR ;
	else
	time.t_min = _t_min;
	if ((_t_sec < 0 )|| (_t_sec > 59))
	time.t_sec = TIME_ERROR;
	time.t_sec = _t_sec;
	return time;
}

unsigned long MyTime::Time2Sec (ttime time)
{
 return (unsigned long)time.t_hour*3600 + (unsigned long)time.t_min * 60 + (unsigned long) time.t_sec * 1;
}

/* time_2 must be older than time_1 */
unsigned long MyTime::TimeDiffSec (ttime time_1, ttime time_2)
{
 unsigned long time1, time2, time;
 time1 = Time2Sec(time_1);
 time2 = Time2Sec(time_2);
 time = 0;
 if (time2 > time1){
	 time = time2 - time1;
 }
 if (time2<time1){
	 time = (MAX_TIME-time1) + time2 ;
 }
 return time;
}

unsigned long MyTime::DiffSec (unsigned long time_1, unsigned long time_2)
{
 unsigned long time = 0;
 if (time_2 > time_1){
	 time = time_2 - time_1;
 }
 if (time_2<time_1){
	 time = (MAX_TIME - time_1) + time_2 ;
 }
 return time;
}


signed char MyTime::CheckTimeError(ttime time)
{
	if ((time.t_hour == TIME_ERROR) || (time.t_min == TIME_ERROR) || (time.t_sec == TIME_ERROR))
		return TIME_ERROR;
	return TIME_OK;
}

class DataPower : private MyTime{
	public:
		DataPower(unsigned short _probes);
		void UpdateEnergy(unsigned int eng, ttime time);
		short ActualPower ;
	private:		
		unsigned short probes;
		tenergy *energy;
		unsigned short pointer;
	public:
		~DataPower(){
			if (energy!=NULL) free (energy);
		}
};

DataPower::DataPower(unsigned short _probes) : MyTime()
{
		probes = _probes;
		energy = (tenergy *)malloc(sizeof(tenergy)*probes);
		memset(energy,0,sizeof(tenergy)*probes);
		pointer = 0;
		ActualPower = 0 ;
}

void DataPower::UpdateEnergy(unsigned int eng, ttime time){
	unsigned short i,j;
	unsigned long num;
	char finish;
	tenergy *tmp_energy;
	tenergy energy_avg ;
	tenergy tmp_e;
	ActualPower = SZARP_NO_DATA;
	if (pointer<probes){
		energy[pointer].t_en = eng;
		energy[pointer].t_sec = Time2Sec(time);
		pointer++;
	}else{
		/* Wyszukiwanie minimalnego offsetu dla ktorego energie beda != */

		tmp_e.t_en = energy[0].t_en;
		tmp_e.t_sec = energy[0].t_sec;
		energy_avg.t_en=0;
		energy_avg.t_sec=0;
		
		for (i=1;i<probes;i++){
			energy_avg.t_en+=energy[i].t_en;
			energy_avg.t_sec+=energy[i].t_sec;
		}
		energy_avg.t_en /= (unsigned long)(probes -1);
		energy_avg.t_sec /= (unsigned long)(probes -1);
		num = ((unsigned long)energy_avg.t_en-(unsigned long)tmp_e.t_en );	
		if (num==0){
			ActualPower = 0;
		}else{
		ActualPower = (short)((num*3600)/((unsigned long)DiffSec(  (unsigned long)tmp_e.t_sec, (unsigned long)energy_avg.t_sec)));
		}
//		i=1;
//		while((finish==NO)&&(i < probes)){
//			if ((tmp_e.t_en < energy[i].t_en) && (tmp_e.t_sec!=energy[i].t_sec)){
//				finish = YES;
//				ActualPower = (short)((((unsigned long)energy[i].t_en-(unsigned long)tmp_e.t_en )*3600)/((unsigned long)DiffSec(  (unsigned long)tmp_e.t_sec, (unsigned long)energy[i].t_sec)));
//				
//			}
//			i++;
//		}
//		
//		for (i=0;i<probes;i++ ){
//			
//		}
		
		finish=NO;
		//Jesli bufor jest z probkami z nstepnego dnia ActualPower jest szukany
		for (i=0;i<probes-1;i++){
			if ((energy[i].t_sec)>(energy[i+1].t_sec)){
			j=1;
			while((finish==NO)&&(j < probes)){
			if ((tmp_e.t_en < energy[j].t_en) && (tmp_e.t_sec!=energy[j].t_sec)){
				finish = YES;
				ActualPower = (short)((((unsigned long)energy[j].t_en-(unsigned long)tmp_e.t_en )*3600)/((unsigned long)DiffSec(  (unsigned long)tmp_e.t_sec, (unsigned long)energy[j].t_sec)));
				
			}
			j++;
		}

				break;
	}
			
   }
		
		tmp_energy = (tenergy *)malloc(sizeof(tenergy)*probes);
		memset(tmp_energy,0,sizeof(tenergy)*probes);
		for (i=0;i<probes-1;i++ ){
			tmp_energy[i].t_en = energy[i+1].t_en;
			tmp_energy[i].t_sec = energy[i+1].t_sec;
		}
		tmp_energy[probes-1].t_en = eng;
		tmp_energy[probes-1].t_sec = Time2Sec(time);
		memcpy(energy,tmp_energy,sizeof(tenergy)*probes);
		free(tmp_energy);

	}
	
}
/************************************************************************************************************/
/* //KLASY ZAIMPORTOWANE Z SRE2DMN */
/************************************************************************************************************/

void GetLocalttime(ttime *tt){
	time_t lt;
	struct tm *mytm;
	lt = time(NULL);
	mytm = localtime(&lt);
	tt->t_hour  = (signed char)mytm->tm_hour;
	tt->t_min  = (signed char)mytm->tm_min;
	tt->t_sec  = (signed char)mytm->tm_sec;
}

/**
 * AqtBus communication config.
 */
class           AqtBus {
      public:
	/** info about single parameter */
				/**< array of params to read */
	int             m_params_count;
				/**< size of params array */
				  /**< virtual memory map */

	int		m_energy ;
	int		m_refresh ;

	/**
	 * @param params number of params to read
	 * @param sends number of params to send (write)
	 */
	AqtBus(int params) {
		assert(params >= 0);
		m_params_count = params;
	}

	~AqtBus() {
	}



	/**
	 * Filling m_read structure SZARP_NO_DATA value
	 */
	void            SetNoData(IPCHandler * ipc);

	/**
	 * Filling m_read structure SZARP_NO_DATA value
	 */
	void            SetNoData(IPCHandler * ipc, unsigned short Offset);
	
	/**
	 * Function oppening serial port special for Pollustat E Heat Meter
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

	int GetResponse(int fd, unsigned char *Response);

	unsigned short *CutBlock(unsigned char *DataPacket, short Offset);
	char CheckCRC (unsigned char *Packet, int PacketSize) ;
	int parseDevice(xmlNodePtr node) ;

	/**
	 *
	 * Parses DataBlock
	 * 
	 */
	int ParseBlock( unsigned short *bloc, short position, short size ); 

	/**
	 * Generating CRC 16 for AqtBus Query
	 */ 
	unsigned short CRC16(unsigned char *Packet, int PacketSize);

	/**
	 * Generating Query
	 */
	unsigned char *CreateQuery(unsigned char Device, unsigned short Address) ;

	void SendQuery (int fd, unsigned short Address) ; 

	/**
	 * Calculates energy
	*/
	unsigned int CalculateEnergy(unsigned char Wh_j_fl,  unsigned int Energy);

	/**
	 * Calculates Volume
	*/
      private:

};


void AqtBus::SetNoData(IPCHandler * ipc)
{
	int i;

	for (i = 0; i < ipc->m_params_count; i++)
		ipc->m_read[i] = SZARP_NO_DATA;

}

void AqtBus::SetNoData(IPCHandler * ipc, unsigned short  Offset)
{
	if (Offset < ipc->m_params_count) 
		ipc->m_read[Offset] = SZARP_NO_DATA;

}

int AqtBus::InitComm(const char *Device, long BaudRate, unsigned char DataBits,
	     unsigned char StopBits, unsigned char Parity)
{
	int             CommId;
	long            BaudRateStatus;
	long            DataBitsStatus;
	long            StopBitsStatus;
	long            ParityStatus;
	struct termios  rsconf;
	int serial;
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
			BaudRateStatus = B2400;
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
			ParityStatus = PARENB | PARODD;
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
	ioctl(CommId, TIOCMGET, &serial);
	serial |= TIOCM_DTR; //DTR is High
	serial |= TIOCM_RTS; //RTS is High
	ioctl(CommId, TIOCMSET, &serial);
	return CommId;
}

int AqtBus::GetResponse(int fd, unsigned char *Response)
{
 fd_set rfds;
 struct timeval tv;
 int i,j;
 int retval;
 FD_ZERO(&rfds);
 FD_SET(fd, &rfds);
 tv.tv_sec = 15;
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

unsigned short *CutBlock(unsigned char *DataPacket,  short Offset){
	unsigned short *Block ;
	int i;
	if (Offset < 0) return NULL ;
	Block = (unsigned short *)malloc(8 * sizeof(unsigned short));
	for (i=0 ; i < 8 ; i++){ /* Czytamy same bloki 8 bajtowe */
		Block[i] = 0;
		Block[i] |= DataPacket[Offset + 2 * i] << 8 ; // MSB
		Block[i] |= DataPacket[Offset + 2 * i + 1] ; // LSB
		
			
	}
	return (Block);
	
}


int AqtBus::ParseBlock( unsigned short *bloc, short position, short size ) 
{
  int v = 0;
  int p;

  for (p = position + size - 1; p >= position; p--) {
    if ((bloc[p / 16] & (1 << (p & 15))) != 0) {
      v = (v << 1) + 1;
    } else
      v <<= 1;
  }
  return v;
}  
unsigned short AqtBus::CRC16(unsigned char *Packet, int PacketSize)
{
	unsigned short  CRC,
	                temp,
	                carry;
	int             i,
	                j;

	CRC = 0xffff;
	for (i = 0; i < PacketSize; i++) {
		CRC = CRC ^ Packet[i];
		for (j = 0; j < 8; j++) {
			temp = CRC;
			carry = temp & 0x0001;
			CRC = CRC >> 1;
			if (carry == 1)
				CRC = CRC ^ 0xa001;
		}
	}
	/*
	 * Zamiana bajtow 
	 */
	temp = CRC & 0xff;
	CRC = CRC >> 8;
	CRC = CRC | (temp << 8);
	return CRC;
}
	
/* to jest w sumie Read Hodling Register */	
unsigned char *AqtBus::CreateQuery(unsigned char Device, unsigned short Address)
{
	
	unsigned char *buffer ;
	unsigned short crc ;
	buffer = (unsigned char *)malloc(8*sizeof(unsigned char));
	buffer[0] = Device;
	buffer[1] = 0x03;
	buffer[2] = ((Address & 0xff00) >> 8);
	buffer[3] = Address & 0x00ff;
	buffer[4] = ((8 & 0xff00) >> 8);
	buffer[5] = 8 & 0x00ff;
	crc = CRC16(buffer, 8 - 2);
	buffer[6] = (crc & 0xff00) >> 8;
	buffer[7] = crc & 0xff;
	return buffer ;
}

void AqtBus::SendQuery (int fd, unsigned short Address )
{
	unsigned char *Query ;
	Query = (unsigned char *)malloc(1);
	Query[0]=0xff;
	int ret = write(fd, Query, 1); /* Budzenie przelicznika ze snu */
	(void)ret;
	free (Query);
	usleep(1000*100);
//	usleep (1000 * 25) ;
	Query = NULL ;
	Query = CreateQuery(DEFAULT_DEVICE, Address) ;
	ret = write(fd, Query, PACKET_SIZE_8);
	(void)ret;
//	printf("Sprawdzam CRC zapytania: %d\n",CheckCRC(Query,PACKET_SIZE_8));	
	free (Query) ;
	usleep(1500*1000);
}

char AqtBus::CheckCRC (unsigned char *Packet, int PacketSize)
{
	if (PacketSize!=30) return RESPONSE_BAD;
	if (Packet[0]!=0xff)return RESPONSE_BAD;
	return RESPONSE_OK ;
}

unsigned int AqtBus::CalculateEnergy(unsigned char Wh_j_fl, unsigned int Energy)
{
	unsigned int Enr;
	Enr = Energy ;
	if (!Wh_j_fl)
		Enr = (unsigned int)((float)Enr * 3.6);
	return Enr ;
}

int AqtBus::parseDevice(xmlNodePtr node)
{
	assert(node != NULL);
	char           *str;
	
	str = (char *) xmlGetNsProp(node,
				    BAD_CAST("refresh"),
				    BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (str == NULL) {
		sz_log(0,
		    "warning: attribute aqtdmn:refresh not found in device element, line %ld",
		    xmlGetLineNo(node));
		m_refresh = 12 ;
	}else{
		m_refresh = atoi(str) ;
		if (m_refresh <= 0) m_refresh = 12 ;
		free (str) ;
	}

	str = (char *) xmlGetNsProp(node,
				    BAD_CAST("energy"),
				    BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (str == NULL) {
		sz_log(0,
		    "warning: atribute aqtdmn:energy mode not found in device element, line %ld",
		    xmlGetLineNo(node));
		m_energy = ENERGY_READ;
	
	} else if (strcmp(str, "calculated") == 0){
			m_energy = ENERGY_CALCULATED ;
			free(str);
		}
	else if (strcmp(str, "read") == 0){
			m_energy = ENERGY_READ ;
			free(str);
		}
	else {
		sz_log(0,
		    "warning: attribute aqtdmn:energy (must be 'calculated' or 'read'), line %ld",
		    xmlGetLineNo(node));
		m_energy = ENERGY_READ;
		free (str) ;
	}
	return 0;
}

int main(int argc, char *argv[])
{
	DaemonConfig   *cfg;
	AqtBus     *aqtinfo;
	IPCHandler     *ipc;
	int             fd;
	int 		ResponseStatus;
	unsigned short ResponseCounter ;
	unsigned char MyResponse ;
	unsigned short *Block;
	unsigned char buf[1000];
	unsigned char Wh_j_fl = 0 ;
	int value, value2 ;		
	unsigned char ind; /* internal no data ;( */
	int energy_register ;
	int calculated_power;
	int calculated_flow;
	ttime tt;
	value=0;	
	cfg = new DaemonConfig("aqtdmn");

	if (cfg->Load(&argc, argv))
		return 1;
	aqtinfo = new AqtBus(cfg->GetDevice()->GetFirstRadio()->
				GetFirstUnit()->GetParamsCount());

	if (aqtinfo->parseDevice(cfg->GetXMLDevice()))
		return 1;

	if (cfg->GetSingle()) {
		printf("\
line number: %d\n\
device: %ls\n\
params in: %d\n", cfg->GetLineNumber(), cfg->GetDevice()->GetPath().c_str(), aqtinfo->m_params_count);
	
	printf("refresh: %d s\n",aqtinfo->m_refresh);
	printf("energy: %s\n",aqtinfo->m_energy == ENERGY_READ?"READ":"CALCULATED");
	}

	

	ipc = new IPCHandler(cfg);
	if (!cfg->GetSingle()) {
		if (ipc->Init())
			return 1;
	}
	if (ipc->m_params_count!=N_OF_PARAMS){
		sz_log(2, "Error in configuration. Check Params.xml");
		return 1;
	}
	
	sz_log(2, "starting main loop");
	//aqtinfo->SetNoData(ipc);
//	int i;
	energy_register = 0;
	calculated_flow = 0;
	calculated_power = 0;
	DataPower power(10);
	DataPower flow(10);
	while (true) {
		fd = aqtinfo->InitComm(SC::S2A(cfg->GetDevice()->GetPath()).c_str(),
			      cfg->GetDevice()->GetSpeed(), 8, 1, ODD);

		/* Rejestry */
		MyResponse = RESPONSE_BAD ;
		ResponseCounter = 0 ;
		/* Wg opisu algorytmu nalezy do 3 razy zapytac cieplomierz o jeden parametr */
		/* a potem mozna wywalic NO_DATA */
		ind = 0;
		while ((MyResponse != RESPONSE_OK) && (ResponseCounter < 3) ){
			aqtinfo->SendQuery (fd, AQT_REG09_B);

			ResponseStatus = aqtinfo->GetResponse(fd,buf);
		//	for(i=0;i<29; i++)
		//		printf("dane[%d]=%d\n ", i, buf[i] );
			if (ResponseStatus<0){ /* Gdy Select wyrzuci ERROR */
				MyResponse = RESPONSE_BAD ;
				if ((cfg->GetSingle() == 1)||(cfg->GetDiagno()==1)){
					printf("ERROR: Select Error - propably noc connected!\n");
				}

			}
			else
			if (ResponseStatus == PACKET_SIZE_8){ /* Gdy trafi sie echo */
				MyResponse = RESPONSE_BAD ;
				if ((cfg->GetSingle() == 1)||(cfg->GetDiagno()==1)){
					printf("WARNING: echo\n");
				}
	
			}
			else	
			if (ResponseStatus == PACKET_SIZE_9){ /* Gdy trafi sie echo */
				MyResponse = RESPONSE_BAD ;
				if ((cfg->GetSingle() == 1)||(cfg->GetDiagno()==1)){
					printf("WARNING: echo\n");
				}
			}
			else
				MyResponse = RESPONSE_OK ;
			ResponseCounter++ ;
		}
		
		if (MyResponse != RESPONSE_BAD){	
			Block = CutBlock (buf, ResponseStatus - DATA_OFFSET) ;
			if (Block != NULL){
				Wh_j_fl = aqtinfo->ParseBlock( Block, AQT_WH_J_FL_P, AQT_WH_J_FL_S )  ;  
				if ((cfg->GetSingle() == 1)||(cfg->GetDiagno()==1))
					printf (" Rejestr WH_J_FL_B: %d \n", Wh_j_fl ) ;
				free (Block);
				ind = 1;
			}
		}

		MyResponse = RESPONSE_BAD ;
		ResponseCounter = 0 ;
		/* Wg opisu algorytmu nalezy do 3 razy zapytac cieplomierz o jeden parametr */
		/* a potem mozna wywalic NO_DATA */
		while ((MyResponse != RESPONSE_OK) && (ResponseCounter < 3) ){
			aqtinfo->SendQuery (fd, AQT_ENERGY_B);

			ResponseStatus = aqtinfo->GetResponse(fd,buf);
			//for(i=0;i<29; i++)
			//	printf("dane[%d]=%d\n ", i, buf[i] );
	
			if (ResponseStatus<0) /* Gdy Select wyrzuci ERROR */
				MyResponse = RESPONSE_BAD ;
			else
			if (ResponseStatus == PACKET_SIZE_8) /* Gdy trafi sie echo */
				MyResponse = RESPONSE_BAD ;
			else	
			if (ResponseStatus == PACKET_SIZE_9) /* Gdy trafi sie echo */
				MyResponse = RESPONSE_BAD ;
			else
				MyResponse = RESPONSE_OK ;
			ResponseCounter++ ;
		}
		MyResponse = aqtinfo->CheckCRC(buf, ResponseStatus) ;
		
		if (MyResponse == RESPONSE_BAD){
			aqtinfo->SetNoData(ipc, AQT_ENERGY_O);
			aqtinfo->SetNoData(ipc, AQT_ENERGY_O+1);
		}else{
			Block = CutBlock (buf, ResponseStatus - DATA_OFFSET) ;
			if ((Block == NULL) || (ind == 0)){
				aqtinfo->SetNoData(ipc, AQT_ENERGY_O);
				aqtinfo->SetNoData(ipc, AQT_ENERGY_O+1);
				calculated_power = SZARP_NO_DATA ;
			}else{
				value = aqtinfo->ParseBlock( Block, AQT_ENERGY_P, AQT_ENERGY_S )  ; 
					ipc->m_read[AQT_ENERGY_O] = aqtinfo->CalculateEnergy(Wh_j_fl, value) & 0xffff;
					ipc->m_read[AQT_ENERGY_O+1] = aqtinfo->CalculateEnergy(Wh_j_fl, value) >> 16;
					energy_register = aqtinfo->CalculateEnergy(Wh_j_fl, value) ;
					GetLocalttime(&tt);
					power.UpdateEnergy(energy_register,tt);
					calculated_power = power.ActualPower;
				 if ((cfg->GetSingle() == 1)||(cfg->GetDiagno()==1)){
					printf (" Energia: %d \n", aqtinfo->CalculateEnergy(Wh_j_fl, value));
					printf("Godzina: %d\n",tt.t_hour);
					printf("Minuta: %d\n",tt.t_min);
					printf("Sekunda: %d\n",tt.t_sec);
				 }
				 free (Block) ;
			}
		}
		/* Przep³yw */
		MyResponse = RESPONSE_BAD ;
		ResponseCounter = 0 ;
		/* Wg opisu algorytmu nalezy do 3 razy zapytac cieplomierz o jeden parametr */
		/* a potem mozna wywalic NO_DATA */
		while ((MyResponse != RESPONSE_OK) && (ResponseCounter < 3) ){
			aqtinfo->SendQuery (fd, AQT_VOLUME_B);

			ResponseStatus = aqtinfo->GetResponse(fd,buf);
			if (ResponseStatus<0) /* Gdy Select wyrzuci ERROR */
				MyResponse = RESPONSE_BAD ;
			else
			if (ResponseStatus == PACKET_SIZE_8) /* Gdy trafi sie echo */
				MyResponse = RESPONSE_BAD ;
			else	
			if (ResponseStatus == PACKET_SIZE_9) /* Gdy trafi sie echo */
				MyResponse = RESPONSE_BAD ;
			else
				MyResponse = RESPONSE_OK ;
			ResponseCounter++ ;
		}
		MyResponse = aqtinfo->CheckCRC(buf, ResponseStatus) ;
		if (MyResponse == RESPONSE_BAD){
			aqtinfo->SetNoData(ipc, AQT_VOLUME_O);
			aqtinfo->SetNoData(ipc, AQT_VOLUME_O+1);
		}else{
			Block = CutBlock (buf, ResponseStatus - DATA_OFFSET) ;
			if ((Block == NULL) || (ind == 0) ){
				aqtinfo->SetNoData(ipc, AQT_VOLUME_O);
				aqtinfo->SetNoData(ipc, AQT_VOLUME_O+1);
				calculated_flow = SZARP_NO_DATA ;
			}else{
				value = aqtinfo->ParseBlock( Block, AQT_VOLUME_P, AQT_VOLUME_S )  ; 
				ipc->m_read[AQT_VOLUME_O] =  value & 0xffff;
				ipc->m_read[AQT_VOLUME_O+1] = value >> 16;
				GetLocalttime(&tt);
				flow.UpdateEnergy(energy_register,tt);
				calculated_flow = flow.ActualPower;
				if ((cfg->GetSingle() == 1)||(cfg->GetDiagno()==1))
					printf (" Przep³yw przyrostowy: %d \n", value ) ;
				free (Block);
			}
		}
		
		/* Temperatura zasilania */
		MyResponse = RESPONSE_BAD ;
		ResponseCounter = 0 ;
		/* Wg opisu algorytmu nalezy do 3 razy zapytac cieplomierz o jeden parametr */
		/* a potem mozna wywalic NO_DATA */
		while ((MyResponse != RESPONSE_OK) && (ResponseCounter < 3) ){
			aqtinfo->SendQuery (fd, AQT_TEMPH_B);

			ResponseStatus = aqtinfo->GetResponse(fd,buf);
			if (ResponseStatus<0) /* Gdy Select wyrzuci ERROR */
				MyResponse = RESPONSE_BAD ;
			else
			if (ResponseStatus == PACKET_SIZE_8) /* Gdy trafi sie echo */
				MyResponse = RESPONSE_BAD ;
			else	
			if (ResponseStatus == PACKET_SIZE_9) /* Gdy trafi sie echo */
				MyResponse = RESPONSE_BAD ;
			else
				MyResponse = RESPONSE_OK ;
			ResponseCounter++ ;
		}
		MyResponse = aqtinfo->CheckCRC(buf, ResponseStatus) ;
		if (MyResponse == RESPONSE_BAD){
			aqtinfo->SetNoData(ipc, AQT_TEMPH_O);
			aqtinfo->SetNoData(ipc, AQT_TEMPH_O+1);
		//
			aqtinfo->SetNoData(ipc, AQT_TEMPL_O);
			aqtinfo->SetNoData(ipc, AQT_TEMPL_O+1);
		}else{
			Block = CutBlock (buf, ResponseStatus - DATA_OFFSET) ;
			if (Block == NULL){
				aqtinfo->SetNoData(ipc, AQT_TEMPH_O);
				aqtinfo->SetNoData(ipc, AQT_TEMPH_O+1);
			//
				aqtinfo->SetNoData(ipc, AQT_TEMPL_O);
				aqtinfo->SetNoData(ipc, AQT_TEMPL_O+1);			
			}else{
				value = aqtinfo->ParseBlock( Block, AQT_TEMPH_P, AQT_TEMPH_S ) ;
				value2 = aqtinfo->ParseBlock( Block, AQT_TEMPL_P, AQT_TEMPL_S ) ;
				value *=5 ;
				value2 *=5 ;
				ipc->m_read[AQT_TEMPH_O] = value & 0xffff;
				ipc->m_read[AQT_TEMPH_O+1] = value >> 16;
				ipc->m_read[AQT_TEMPL_O] = value2 & 0xffff;
				ipc->m_read[AQT_TEMPL_O+1] = value2 >> 16;
				if ((cfg->GetSingle() == 1)||(cfg->GetDiagno()==1)){
					printf (" Temperatura zasilania: %d \n", value) ;
					printf (" Temperatura powrotu: %d \n", value2) ;
				 }
				free (Block) ;
			}
		}
		/* Przep³yw aktualny */
		/* Parametr wyliczany lub odcyztywany - w zaleznosci od konfiguracji */
		if (aqtinfo->m_energy == ENERGY_READ )
		{
			MyResponse = RESPONSE_BAD ;
			ResponseCounter = 0 ;
			/* Wg opisu algorytmu nalezy do 3 razy zapytac cieplomierz o jeden parametr */
			/* a potem mozna wywalic NO_DATA */
			while ((MyResponse != RESPONSE_OK) && (ResponseCounter < 3) ){
				aqtinfo->SendQuery (fd, AQT_FLOW_B);
	
				ResponseStatus = aqtinfo->GetResponse(fd,buf);
				if (ResponseStatus<0) /* Gdy Select wyrzuci ERROR */
					MyResponse = RESPONSE_BAD ;
				else
				if (ResponseStatus == PACKET_SIZE_8) /* Gdy trafi sie echo */
					MyResponse = RESPONSE_BAD ;
				else	
				if (ResponseStatus == PACKET_SIZE_9) /* Gdy trafi sie echo */
					MyResponse = RESPONSE_BAD ;
				else
					MyResponse = RESPONSE_OK ;
				ResponseCounter++ ;
			}
			MyResponse = aqtinfo->CheckCRC(buf, ResponseStatus) ;	
			if (MyResponse == RESPONSE_BAD){
				aqtinfo->SetNoData(ipc, AQT_FLOW_O);
				aqtinfo->SetNoData(ipc, AQT_FLOW_O+1);
			}else{
				Block = CutBlock (buf, ResponseStatus - DATA_OFFSET) ;
				if ((Block == NULL) || (ind == 0) ){
					aqtinfo->SetNoData(ipc, AQT_FLOW_O);
					aqtinfo->SetNoData(ipc, AQT_FLOW_O+1);		
				}else{
					value = aqtinfo->ParseBlock( Block, AQT_FLOW_P, AQT_FLOW_S )  ;
					ipc->m_read[AQT_FLOW_O] = value & 0xffff;
					ipc->m_read[AQT_FLOW_O+1] = value >> 16;
	
					if ((cfg->GetSingle() == 1)||(cfg->GetDiagno()==1))
						printf (" Przep³yw aktualny: %d \n", value ) ;
					free (Block);
				}
			}
		}else{
		/* Gdy energia jest odczytywana */
			if (calculated_flow == SZARP_NO_DATA){
				ipc->m_read[AQT_FLOW_O] = SZARP_NO_DATA ;
				ipc->m_read[AQT_FLOW_O+1] = SZARP_NO_DATA ;
				
			}else{
				ipc->m_read[AQT_FLOW_O] = calculated_flow & 0xffff;
				ipc->m_read[AQT_FLOW_O+1] = calculated_flow >> 16;
			}
		
			if ((cfg->GetSingle() == 1)||(cfg->GetDiagno()==1))
						printf (" Przep³yw aktualny [wyliczany]: %d \n", calculated_flow ) ;

		}
		if (aqtinfo->m_energy == ENERGY_READ )
		{	/* Moc aktualna */
			MyResponse = RESPONSE_BAD ;
			ResponseCounter = 0 ;
			/* Wg opisu algorytmu nalezy do 3 razy zapytac cieplomierz o jeden parametr */
			/* a potem mozna wywalic NO_DATA */
			while ((MyResponse != RESPONSE_OK) && (ResponseCounter < 3) ){
				aqtinfo->SendQuery (fd, AQT_POWER_B);
	
				ResponseStatus = aqtinfo->GetResponse(fd,buf);
				if (ResponseStatus<0) /* Gdy Select wyrzuci ERROR */
					MyResponse = RESPONSE_BAD ;
				else
				if (ResponseStatus == PACKET_SIZE_8) /* Gdy trafi sie echo */
					MyResponse = RESPONSE_BAD ;
				else	
				if (ResponseStatus == PACKET_SIZE_9) /* Gdy trafi sie echo */
					MyResponse = RESPONSE_BAD ;
				else
					MyResponse = RESPONSE_OK ;
				ResponseCounter++ ;
			}
			MyResponse = aqtinfo->CheckCRC(buf, ResponseStatus) ;
	
			if (MyResponse == RESPONSE_BAD){
				aqtinfo->SetNoData(ipc, AQT_POWER_O);
				aqtinfo->SetNoData(ipc, AQT_POWER_O+1);
			}else{
				Block = CutBlock (buf, ResponseStatus - DATA_OFFSET) ;
				if ((Block == NULL) || (ind == 0) ){
					aqtinfo->SetNoData(ipc, AQT_POWER_O);
					aqtinfo->SetNoData(ipc, AQT_POWER_O+1);		
				}else{
					value = aqtinfo->ParseBlock( Block, AQT_POWER_P, AQT_POWER_S )  ;
					value *= 3;
					ipc->m_read[AQT_POWER_O] = value & 0xffff;
					ipc->m_read[AQT_POWER_O+1] = value >> 16;
	
					if ((cfg->GetSingle() == 1)||(cfg->GetDiagno()==1))
						printf (" Moc aktualna: %d \n", value ) ;
					free (Block);
				}
			}

		}else{
			if (calculated_power == SZARP_NO_DATA){
				ipc->m_read[AQT_POWER_O] = SZARP_NO_DATA ;
				ipc->m_read[AQT_POWER_O+1] = SZARP_NO_DATA ;
			}else{
				ipc->m_read[AQT_POWER_O] = calculated_power & 0xffff;
				ipc->m_read[AQT_POWER_O+1] = calculated_power >> 16;
			}

				if ((cfg->GetSingle() == 1)||(cfg->GetDiagno()==1))
					printf (" Moc aktualna [wyliczana]: %d \n", calculated_power ) ;
		}
		ipc->GoParcook();
		sleep(aqtinfo->m_refresh); //Wait 12 minutes
		close(fd);
	}
//	free (ParsedData);
	return 0;
}
