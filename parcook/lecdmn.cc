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
 * LECdmn 
 * Pawe³ Kolega
 * demon dla ciep³omierzy LEC-4 i LEC-5 firmy Apator-Kfap
 * oparty na protokole wewnêtrznym Asi ()
 * napisany na podstawie dokumentacji dostarczonej przez producenta
 * $Id$
 */
/*
 @description_start
 @class 4
 @devices Apator-Kfap LEC-4/LEC-5 heatmeter.
 @devices.pl Ciep³omierze Apator-Kfap LEC-4/LEC-5.
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

#define DAEMON_ERROR 1

#define GENERAL_ERROR -1
#define TIMEOUT_ERROR -2
#define PACKET_SIZE_ERROR -1

#define NO_PARITY 0
#define EVEN 1
#define ODD 2

/* Identyfikatory zapytañ */
#define ENERGY_ORDER 0x45  /* Energia */
#define VOLUME_ORDER 0x44 /* Przep³yw  */
#define VOLUME_ORDER_LEC5 0x53 /* Przep³yw LEC5 */
#define OUTLET_TEMPERATURE_ORDER 0x5A /* Temperatura wyj¶ciowa */
#define INLET_TEMPERATURE_ORDER 0x4F /* Temperatura wej¶ciowa */
#define POWER_ORDER 0x50 /* Moc  */
#define WATER_ORDER 0x56 /* Woda */

/* predefiniowane sta³e  */
#define NET_ID 0x01 /* Jak nie wisz do czego s³u¿y - nie ruszaj */
#define QUERY_SIZE 5
#define MAX_RETRIES 5 /* Maksymalna ilo¶æ prób odczytu danej warto¶ci  */
#define MAX_RESPONSE 30 /* Maksymalna d³ugo¶æ odpowiedzi  */

#define OK 1
#define FAIL 0

#define LEC3 0
#define LEC4 1
#define LEC5 2

/**
 * LECAsi communication config.
 */


class AvgBuffer {
	public:
	AvgBuffer(int size);
	~AvgBuffer();
	/**
	 * Buffer - point to avg buffer
	 * FillValue - initial value
	 * Size - size of buffer
	*/
	void InitAvgBuffer(int FillValue);
	/**
	 * Buffer - point to avg buffer
	 * Ptr - Pointer to buffer
	 * Size - size of buffer
	*/
	void InitAvgBufferOneTime(int FillValue);
	/**
	 * Buffer - point to avg buffer
	 * Ptr - Pointer to buffer
	 * Size - size of buffer
	*/

	int CalcAvgBuffer(int Value); 

	int CalcAvgBuffer(); 
	
	private:
	int _size;
	int * _AvgBuffer;
	int _ptr;
	char Latch;

};

	AvgBuffer::AvgBuffer(int size){
		_ptr = 0;
		_size = size;
		_AvgBuffer = (int *)malloc(_size * sizeof(int));
		Latch = 0;
	}

	void AvgBuffer::InitAvgBuffer(int FillValue){
	int i;
	for (i=0;i<_size;i++)
		_AvgBuffer[i] = FillValue;
	}

	void AvgBuffer::InitAvgBufferOneTime(int FillValue){
		if (!Latch){
			Latch = 1;
			InitAvgBuffer(FillValue);
		}
	}

int AvgBuffer::CalcAvgBuffer(int Value){
	int Div;
	int Sum;
	int i;
	Sum = 0;
	Div = 0;
	for (i=0;i<_size;i++){
		if (_AvgBuffer[i]!=SZARP_NO_DATA){ Div ++; Sum+=_AvgBuffer[i];}

	}
	_ptr = (_ptr % _size) + 1;
	_AvgBuffer[_ptr-1] = Value;
	if (Sum==0) return 0;
	if (Div==0) return SZARP_NO_DATA;
	return (Sum/Div);
}

int AvgBuffer::CalcAvgBuffer(){
	int Div;
	int Sum;
	int i;
	Sum = 0;
	Div = 0;
	for (i=0;i<_size;i++){
		if (_AvgBuffer[i]!=SZARP_NO_DATA){ Div ++; Sum+=_AvgBuffer[i];}
	}
	if (Sum==0) return 0;
	if (Div==0) return SZARP_NO_DATA;
	return (Sum/Div);
}

AvgBuffer::~AvgBuffer(){
	free (_AvgBuffer);
}

#define MAX_PERCENTAGE_ERROR 8 //[%]
class CorrectBuffer: public AvgBuffer {
	public:
		CorrectBuffer (int size) : AvgBuffer(size) {};
		int CorrectError(int PercentageError, int Value);

};

int CorrectBuffer::CorrectError(int PercentageError, int Value){
	int iValue;
	int MyValue = 0;
	int AvgValue = 0;
	iValue = CalcAvgBuffer (Value) ;
	AvgValue = CalcAvgBuffer () ; 
	if ( iValue != 0 && (abs (Value - iValue)/iValue) * 100 > PercentageError) {
		MyValue = AvgValue;
	}else{
		MyValue = Value;	
	}
	return MyValue;
}


class           LECAsi {
      public:
	/** info about single parameter */
				/**< array of params to read */
	int             m_params_count;
				/**< size of params array */
				  /**< virtual memory map */
	int             m_sends_count;
				/**< size of sends array */
	int		m_type;
				/**< type of meter (LEC3,LEC4,LEC5) */

	/**
	 * @param params number of params to read
	 * @param sends number of params to send (write)
	 */
	LECAsi(int params, int sends) {
		assert(params >= 0);
		assert(sends >= 0);

		m_params_count = params;
		m_sends_count = sends;
	}

	void  parseDevice(xmlNodePtr node);

	~LECAsi() {
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

	int GetResponse(int fd, unsigned char *Response);

	/**
	 * Creates LEC - Asi Query
	 * @param order order
	 * @param net_id net identifier (default 0xff)
	 * @return pointer to query. User must self frees this pointer !malloc
	 */
	unsigned char *CreateQuery(unsigned char order,unsigned char net_id, unsigned short size);
	/**
	 * Checks CRC in incoming packet
	 * @param Packet packet
	 * @param Size size of packet
	 * @return OK or FAIL
	 */
	char CheckCRC2(unsigned char *Packet, unsigned short Size);
	/**
	 * Checks packet in incoming packet
	 * @param Packet packet
	 * @param size size of packet
	 * @param MaxResponse maximum allowed length of response
	 * @param net_id net identifier (default 0xff)  
	 * @return OK or FAIL
	 */
	char CheckResponse2 (unsigned char *Packet,unsigned short size, unsigned short MaxResponse, unsigned char net_id);

	/**
	 * Parses incoming packet
	 * @param Packet packet
	 * @param size size of packet
	 * @return parsed value
	 */
	unsigned int ParsePacket2 (unsigned char *Packet, unsigned short size, char *status);

	unsigned int GetTotalData(int fd, unsigned char order,unsigned char net_id, unsigned short qsize, unsigned short MaxResponse, unsigned short MaxRetries, char *status);		
      private:
	/**
	 * Calculates CRC
	 * @param Packet packet body
	 * @param Size size of packet
	 * @return crc
	 */
	unsigned char CalculateCRC(unsigned char *Packet,unsigned short Size) ;
	/**
	 * Calculates CRC for response (may be LEC firmware error)
	 * @param Packet packet body
	 * @param Size size of packet
	 * @return crc
	 */
	
	unsigned char CalculateCRC2(unsigned char *Packet,unsigned short Size) ;
};


void LECAsi::parseDevice(xmlNodePtr node)
{
        assert(node != NULL);
	char           *str;

	str = (char*)	xmlGetNsProp(node,
					BAD_CAST("type"),
					BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (str == NULL) {
		sz_log(0,
			"attribute lecdmn:type not found in device element, line %ld",
			xmlGetLineNo(node));
		m_type = LEC4;
		free(str);
	}else if (strcmp(str,"lec3")==0){
		m_type = LEC3;
		free (str);
	}else if(strcmp(str,"lec4")==0){
		m_type = LEC4;
		free (str);
	}else if(strcmp(str,"lec5")==0){
		m_type = LEC5;
		free(str);
	}else{
	        sz_log(0,"warning: attribute lec:type (must be 'lec3','lec4' or 'lec5'), line %ld",
			xmlGetLineNo(node));
		m_type = LEC4;
		free (str) ;
	}
	return;
}

void LECAsi::SetNoData(IPCHandler * ipc)
{
	int i;

	for (i = 0; i < ipc->m_params_count; i++)
		ipc->m_read[i] = SZARP_NO_DATA;

}

int LECAsi::InitComm(const char *Device, long BaudRate, unsigned char DataBits,
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
			BaudRateStatus = B1200;
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

int LECAsi::GetResponse(int fd, unsigned char *Response)
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
		//fprintf(stderr,"Odebralem Response[%d]=%02x\t%c\n",i,Response[i],Response[i]);
		usleep(1000);
		j++ ;
	}
	return (j);
     }
     else
     {   return TIMEOUT_ERROR ;
	    
     }

}

unsigned char *LECAsi::CreateQuery(unsigned char order,unsigned char net_id, unsigned short size)
{
 unsigned char *buffer;
 if (size<5) return NULL ; //Jaki¶ babol
 buffer = (unsigned char *)malloc(sizeof(unsigned char)*size);
 buffer [0] = 0xC0 ;
 buffer [1] = net_id ;
 buffer [2] = order ;
 buffer [3] = 0xBC ;
 buffer [4] = CalculateCRC(buffer,4);
 return buffer ;
}

unsigned char LECAsi::CalculateCRC(unsigned char *Packet, unsigned short Size)
{
  short i;
  unsigned short sum;
  sum = 0;
  for (i=0;i<Size;i++){
	sum+=Packet[i] ;	
  }
  sum = ~sum ; /* Negacja sumy wszystkich bajtów */
  sum &= 0x007f ; /* Gasimy najstarszy bit oraz obciêcie sumy do 8 bitów */ 
  return (unsigned char) sum ;
} 

/* Uwaga algorytm sporz±dzony metod± reverse engineering byæ mo¿e jest to b³±d w firmware  */
unsigned char LECAsi::CalculateCRC2(unsigned char *Packet, unsigned short Size)
{
	short i;
	unsigned char sum;
	sum = 0;
	for (i=0;i<Size;i++){
		sum+=Packet[i] ;
	}
	sum = ~sum ; /* Negacja sumy wszystkich bajtów */
	sum &= 0x7f ; /* Gasimy najstarszy bit oraz obciêcie sumy do 8 bitów */
	sum -= 1; /*  */ 
	return (unsigned char) sum ;
}

char LECAsi::CheckCRC2(unsigned char *Packet, unsigned short Size)
{
	unsigned char crc ;
	if (Size == 0) return FAIL ;
	crc = CalculateCRC2(Packet, Size - 1);
	if (crc == Packet[Size -1]) return OK;
	//fprintf(stderr,"BLAD CRC!\n");
	return FAIL ;
}

/* Wersja dla CRC2  */
char LECAsi::CheckResponse2 (unsigned char *Packet,unsigned short size, unsigned short MaxResponse, unsigned char net_id)
{
	if (size == 0) return FAIL;
	if (size > MaxResponse) return FAIL;
	/* Pare CheckPointow */
	if (Packet[0]!=0xC0) return FAIL ;
	if (Packet[1]!=net_id) return FAIL ;
	if (Packet[3]!=0xBC) return FAIL ;
	if (Packet[size - 2]!=0xBC) return FAIL ;
	if (CheckCRC2(Packet,size)==FAIL) return FAIL;
	return OK ;
}
/* Parsuje pakiet do postaci liczbowej */
unsigned int LECAsi::ParsePacket2 (unsigned char *Packet, unsigned short size, char *status)
{
	char *buffer;
	unsigned int  b ;
	short i,j, k ;
	char pierwsze_zera = 0;
	char zle_znaki = 0;
	char *tmp;
	/* Allokuje tyle pamiêci ile ma pakiet */
	*status=OK;
	buffer = (char *)malloc(sizeof(char)*size);
	memset(buffer,0x0,size);
        pierwsze_zera = 0;
	zle_znaki = 0;
	j=0;
	k=0;
	for (i=5;i<=12;i++){
		if (!isdigit(Packet[i]) && Packet[i]!='.') zle_znaki = 1;
		if (isdigit(Packet[i])){
			k++;
			if (Packet[i]-0x30 > 0) pierwsze_zera = 1;
		 	if (pierwsze_zera == 1){
			 	buffer[j] = Packet[i] ;
			 	j++ ;
			 }
		}
	}

	if (k!=7){
		*status=FAIL;
	}

	b = (unsigned int)strtol(buffer, &tmp, 0);
        if (tmp[0] != 0) {
		*status=FAIL;
	}

	if (zle_znaki){
		b = (unsigned int)SZARP_NO_DATA<<16;
		b |= (unsigned int)SZARP_NO_DATA;
		*status=FAIL;
	}
	free (buffer);
	return b ;
}

unsigned int LECAsi::GetTotalData(int fd, unsigned char order,unsigned char net_id, unsigned short qsize, unsigned short MaxResponse, unsigned short MaxRetries, char *status){
	int ResponseStatus ;
	unsigned  short Retries = 0 ;
	unsigned char *Query ;
	unsigned char buf[1000];
	int bfr ;
	char status2;
	//short i ;
	ResponseStatus = -1 ;
	while ((Retries<MaxRetries)&&(ResponseStatus<0)){
		usleep(10000);
		/* Zgodnie z algorytmem MAX_RETRIES x pytamy o to samo jesli nie przyjda dane wywalamy NO_DATA  */
		Query = CreateQuery(order, net_id, qsize) ;
		/*for(int i=0;i<QUERY_SIZE;i++){
			printf("Query[%d]=%02x\n",i,Query[i]);
		}*/
		write(fd,Query, qsize); //Sending first Question
		free (Query);	
		sleep(3);

		memset(buf,0,sizeof(buf));
		ResponseStatus = GetResponse(fd,buf); //Sending second question
		if (ResponseStatus<=qsize) ResponseStatus = -1;//Echo
		if (ResponseStatus>0) {
			if (CheckResponse2(buf,ResponseStatus, MaxResponse, net_id) == FAIL){ 
				ResponseStatus = -1 ;
			} else {
				break;
			}
		}
		Retries++;	
	}

	if (ResponseStatus<=0){
		*status = FAIL ;
		bfr = 0;
	}else{
		*status = OK ;
		bfr = (int)ParsePacket2 (buf, ResponseStatus, &status2);
		if (status2!=OK) *status=FAIL;
		if (bfr < 0)
			*status = FAIL ;
	}
	return bfr ;
}


int main(int argc, char *argv[])
{

	DaemonConfig   *cfg;
	LECAsi     *asiinfo;
	IPCHandler     *ipc;
	int             fd;
	//unsigned char *Query ;
	//int 		ResponseStatus;
	//int		i;
	unsigned int	bfr ;
	char status ;

	CorrectBuffer CInlet(8);
	CorrectBuffer COutlet(8);
	CorrectBuffer CFlow(8);
	CorrectBuffer CPower(8);
	CorrectBuffer CWater(8);

	cfg = new DaemonConfig("lecdmn");

	if (cfg->Load(&argc, argv))
		return 1;
	asiinfo = new LECAsi(cfg->GetDevice()->GetFirstRadio()->
				GetFirstUnit()->GetParamsCount(),
				cfg->GetDevice()->GetFirstRadio()->
				GetFirstUnit()->GetSendParamsCount());

	asiinfo->parseDevice(cfg->GetXMLDevice());
	
	if (cfg->GetSingle()) {
		printf("\
line number: %d\n\
device: %ls\n\
params in: %d\n", cfg->GetLineNumber(), cfg->GetDevice()->GetPath().c_str(), asiinfo->m_params_count);
	if(asiinfo->m_type==LEC3) printf("type: LEC3\n");
	else if(asiinfo->m_type==LEC4) printf("type: LEC4\n");
	else if(asiinfo->m_type==LEC5) printf("type: LEC5\n");
	}

	
	ipc = new IPCHandler(cfg);
	if (!cfg->GetSingle()) {
		if (ipc->Init())
			return 1;
	}

	
	sz_log(2, "starting main loop");
	asiinfo->SetNoData(ipc);
	while (true) {
		fd = asiinfo->InitComm(SC::S2A(cfg->GetDevice()->GetPath()).c_str(),1200, 8, 1, EVEN);

		if (cfg->GetSingle()){
				fprintf(stderr,"Sending : ENERGY_ORDER Query\n");
		}

		bfr = asiinfo->GetTotalData(fd, ENERGY_ORDER, NET_ID, QUERY_SIZE, MAX_RESPONSE, MAX_RETRIES, &status);		
		if (status == FAIL){
			if (cfg->GetSingle()){
				fprintf(stderr,"FAILED\n");
			}
			ipc->m_read[0] = SZARP_NO_DATA ;
			ipc->m_read[1] = SZARP_NO_DATA ;
		}
		else{
			if (cfg->GetSingle()){
				fprintf(stderr,"Energy: %d\n",bfr);
			}
			ipc->m_read[0]=(short)(bfr&0xffff);
			ipc->m_read[1]=(short)(bfr>>16);
						      
		}

		if (cfg->GetSingle()){
				fprintf(stderr,"Sending : VOLUME_ORDER Query\n");
		}

		if(asiinfo->m_type==LEC5)
		bfr = asiinfo->GetTotalData(fd, VOLUME_ORDER_LEC5, NET_ID, QUERY_SIZE, MAX_RESPONSE, MAX_RETRIES, &status);		
		else
		bfr = asiinfo->GetTotalData(fd, VOLUME_ORDER, NET_ID, QUERY_SIZE, MAX_RESPONSE, MAX_RETRIES, &status);
		if (status == FAIL){
			if (cfg->GetSingle()){
				fprintf(stderr,"FAILED\n");
			}
			ipc->m_read[2] = SZARP_NO_DATA ;
			ipc->m_read[3] = SZARP_NO_DATA ;
		}
		else{
			CFlow.InitAvgBufferOneTime(bfr) ;
			bfr = CFlow.CorrectError(MAX_PERCENTAGE_ERROR, bfr) ;
	
			if (cfg->GetSingle()){
				fprintf(stderr,"Volume: %d\n",bfr);
			}
			ipc->m_read[2]=(short)(bfr&0xffff);
			ipc->m_read[3]=(short)(bfr>>16);
						      
		}
				
		if (cfg->GetSingle()){
				fprintf(stderr,"Sending : OUTLET_TEMPERATURE_ORDER Query\n");
		}

		bfr = asiinfo->GetTotalData(fd, OUTLET_TEMPERATURE_ORDER, NET_ID, QUERY_SIZE, MAX_RESPONSE, MAX_RETRIES, &status);		
		if (status == FAIL){
			if (cfg->GetSingle()){
				fprintf(stderr,"FAILED\n");
			}
			ipc->m_read[4] = SZARP_NO_DATA ;
			ipc->m_read[5] = SZARP_NO_DATA ;
		}
		else{
			COutlet.InitAvgBufferOneTime(bfr) ;
			bfr = COutlet.CorrectError(MAX_PERCENTAGE_ERROR, bfr) ;
	
			if (cfg->GetSingle()){
				fprintf(stderr,"Out temperature: %d\n",bfr);
			}
			ipc->m_read[4]=(short)(bfr&0xffff);
			ipc->m_read[5]=(short)(bfr>>16);
						      
		}

		if (cfg->GetSingle()){
				fprintf(stderr,"Sending : INLET_TEMPERATURE_ORDER Query\n");
		}

		bfr = asiinfo->GetTotalData(fd, INLET_TEMPERATURE_ORDER, NET_ID, QUERY_SIZE, MAX_RESPONSE, MAX_RETRIES, &status);		
		if (status == FAIL){
			if (cfg->GetSingle()){
				fprintf(stderr,"FAILED\n");
			}
			ipc->m_read[6] = SZARP_NO_DATA ;
			ipc->m_read[7] = SZARP_NO_DATA ;
		}
		else{
			CInlet.InitAvgBufferOneTime(bfr) ;
			bfr = CInlet.CorrectError(MAX_PERCENTAGE_ERROR, bfr) ;
			if (cfg->GetSingle()){
				fprintf(stderr,"In temperature: %d\n",bfr);
			}
			ipc->m_read[6]=(short)(bfr&0xffff);
			ipc->m_read[7]=(short)(bfr>>16);
		}
		
		if (cfg->GetSingle()){
				fprintf(stderr,"Sending : POWER_ORDER Query\n");
		}

		bfr = asiinfo->GetTotalData(fd, POWER_ORDER, NET_ID, QUERY_SIZE, MAX_RESPONSE, MAX_RETRIES, &status);		
		if (status == FAIL){
			if (cfg->GetSingle()){
				fprintf(stderr,"FAILED\n");
			}
			ipc->m_read[8] = SZARP_NO_DATA ;
			ipc->m_read[9] = SZARP_NO_DATA ;
		}
		else{
			CPower.InitAvgBufferOneTime(bfr) ;
			bfr = CPower.CorrectError(MAX_PERCENTAGE_ERROR, bfr) ;
	
			if (cfg->GetSingle()){
				fprintf(stderr,"Power: %d\n",bfr);
			}
			ipc->m_read[8]=(short)(bfr&0xffff);
			ipc->m_read[9]=(short)(bfr>>16);
						      
		}

		if (cfg->GetSingle()){
				fprintf(stderr,"Sending : WATER_ORDER Query\n");
		}

		bfr = asiinfo->GetTotalData(fd, WATER_ORDER, NET_ID, QUERY_SIZE, MAX_RESPONSE, MAX_RETRIES, &status);		
		if (status == FAIL){
			if (cfg->GetSingle()){
				fprintf(stderr,"FAILED\n");
			}
			ipc->m_read[10] = SZARP_NO_DATA ;
			ipc->m_read[11] = SZARP_NO_DATA ;
		}
		else{
			CWater.InitAvgBufferOneTime(bfr) ;
			bfr = CWater.CorrectError(MAX_PERCENTAGE_ERROR, bfr) ;
	
			if (cfg->GetSingle()){
				fprintf(stderr,"Water: %d\n",bfr);
			}
			ipc->m_read[10]=(short)(bfr&0xffff);
			ipc->m_read[11]=(short)(bfr>>16);
						      
		}

		ipc->GoParcook();
		if (cfg->GetSingle()){
			sleep(10); //Wait 10s
		}else{
			sleep(8 * 60); //Wait 2m
		}
		close(fd);
	}
//	free (ParsedData);
	return 0;
}
