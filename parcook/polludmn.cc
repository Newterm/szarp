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
 * Polludmn 
 * Pawe³ Kolega
 * demon dla ciep³omierza Pollustat - E demon powsta³ po zdebugowaniu protoko³u
 * pomiêdzy programem Minicom a ciep³omierzem (Transmisja oparta o protokó³ Mbus - nie myliæ z ModBus-em).
 * Ramki, które akceptuje ciep³omierz:
 *  Datum/uhrzeit
 * #68#09#09#68#53#FE#51#04#6D#30#14#AD#01#05#16
 * Mittelungszeit
 * #68#09#09#68#53#FE#51#04#6D#30#14#AD#01#05#16
 * Mbus primaadrrese setzen
 * #68#09#09#68#53#FE#51#04#6D#30#14#AD#01#05#16
 * Absolut maxima loshen
 * #68#09#09#68#53#FE#51#04#6D#30#14#AD#01#05#16
 * Zahlereinstellungen lesen
 * #10#40#FE#3E#16
 * #10#5B#FE#59#16
 * Przy poprawnym zaakceprowaniu komendy cieplomierz zwraca
 * #E5
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


#define _IPCTOOLS_H_
#define _HELP_H_
#define _ICON_ICON_
#include "szarp.h"
#include "msgtypes.h"

#include "ipchandler.h"
#include "liblog.h"
#include "conversion.h"

#define SLEEP_ERROR 180 

#define DAEMON_ERROR 1

#define SND_NKE_LENGTH 5
#define REQ_UD2_LENGTH 5

#define GENERAL_ERROR -1
#define TIMEOUT_ERROR -2
#define PACKET_SIZE_ERROR -1

/* First Question */
const unsigned char SND_NKE[SND_NKE_LENGTH] = {0x10, 0x40, 0xFE, 0x3E, 0x16};

/* Second Question */
const unsigned char REQ_UD2[REQ_UD2_LENGTH] = {0x10, 0x5B, 0xFE, 0x59, 0x16};

/* Response OK */

const unsigned char RESPONSE_OK = 0xE5 ; 


#define NO_PARITY 0
#define EVEN 1
#define ODD 2

/**
 * PolluMbus communication config.
 */
class           PolluMbus {
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
	PolluMbus(int params, int sends) {
		assert(params >= 0);
		assert(sends >= 0);

		m_params_count = params;
		m_sends_count = sends;
	}

	~PolluMbus() {
	}



	/**
	 * Filling m_read structure SZARP_NO_DATA value
	 */
	void            SetNoData(IPCHandler * ipc);

	/**
	 * Function oppening serial port special for Pollustat E Heat Meter
	 * @param Device Path to device np "/dev/ttyS0"
	 * @param BaudRate baud rate
	 * @param StopBits stop bits 1 od 2
	 * @param Parity parity : NO_PARITY, ODD, EVEN 
	 * @param return file descriptor
	 */

	int InitComm(const std::wstring& device, long BaudRate, unsigned char DataBits,
	     unsigned char StopBits, unsigned char Parity);

	/**
	 * Function gets response from serial port
	 * @param fd File descriptor
	 * @param Response  Pointer to buffer
	 * @param return >0 - how bytes read, <=0 error
	 */

	int GetResponse(int fd, unsigned char *Response);

	/**
	 * Parse all response
	 */
	char ParsePacket(unsigned char *Data, unsigned int DataSize, short *ParsedData);

      private:
	/**
	 * Function calculates decimal powers of 10
	 * @param n 
	 * @return - n power of 10
 	*/
	unsigned long pow10(int n);
	/**
	 * Parses one packet and returns data in decimal value
	 * @param Packet - Pointer to packet data
	 * @return - Value from Packet
	 */
	unsigned long ParseValue(unsigned char *Packet);

	/**
	 * Copies one packet from Offset
	 * @param DataIn pointer to Data Source
	 * @param DataOut pointer to Data destination
	 * @param Offset Copy offset
	 * @return - Length of packet
	 */

	unsigned short PacketCopy(unsigned char *DataIn, unsigned char *DataOut,unsigned short Offset);
};


void PolluMbus::SetNoData(IPCHandler * ipc)
{
	int i;

	for (i = 0; i < ipc->m_params_count; i++)
		ipc->m_read[i] = SZARP_NO_DATA;

}

int PolluMbus::InitComm(const std::wstring &device, long BaudRate, unsigned char DataBits,
	     unsigned char StopBits, unsigned char Parity)
{
	int             CommId;
	long            BaudRateStatus;
	long            DataBitsStatus;
	long            StopBitsStatus;
	long            ParityStatus;
	struct termios  rsconf;
	int serial;
	CommId = open(SC::S2A(device).c_str(), O_RDWR | O_NDELAY |O_NONBLOCK);
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
	ioctl(CommId, TIOCMGET, &serial);
	serial |= TIOCM_DTR; //DTR is High
	serial |= TIOCM_RTS; //RTS is High
	ioctl(CommId, TIOCMSET, &serial);
	return CommId;
}

int PolluMbus::GetResponse(int fd, unsigned char *Response)
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


unsigned long PolluMbus::pow10(int n)
{
	int i;
	unsigned long tmp;
	tmp = 1;
	for (i=0;i<n;i++) tmp *= 10;
	return tmp;
}


unsigned long PolluMbus::ParseValue(unsigned char *Packet)
{
 unsigned long tmpValue;
 unsigned int length ;
 unsigned int i;
 tmpValue = 0 ;
 if (Packet[0] - 0x06 <=0) return 0;
 length = Packet[0] - 0x06 ;
 for (i=2;i<length;i++){
	 tmpValue += ((Packet[i]/16)*10 + Packet[i] % 16 )*pow10((i-2)*2);
 }
 return tmpValue ;
}

unsigned short PolluMbus::PacketCopy(unsigned char *DataIn, unsigned char *DataOut,unsigned short Offset)
{
 unsigned short PacketSize;
 unsigned short i ;
 if (DataIn[Offset] - 0x06 <=0) return 0;
 PacketSize =  DataIn[Offset] - 0x06 ;
 for (i=Offset;i<(Offset+PacketSize);i++){
	 DataOut[i-Offset] = DataIn[i];
 }
 DataOut[PacketSize]=0;
 return PacketSize;
}

char PolluMbus::ParsePacket(unsigned char *Data, unsigned int DataSize, short *ParsedData)
{
 unsigned char tmpData[250];

const unsigned short ENERGY_OFFSET = 19;
const unsigned short	FLOW_OFFSET = 31;
const unsigned short INLET_TEMPERATURE_OFFSET = 43;
const unsigned short OUTLET_TEMPERATURE_OFFSET = 47;
const unsigned short TEMPERATURE_DIFFERENCE_OFFSET = 51;
const unsigned short  POWER_OFFSET = 37;

unsigned long buffer;

 if (DataSize<=0) return -1;
 
 if (PacketCopy(Data,tmpData,ENERGY_OFFSET) == 0) return -2;
 buffer = ParseValue(tmpData);
 ParsedData[0] = buffer & 0x0000ffff; /* Energy MSB */
 ParsedData[1] = (buffer & 0xffff0000) >>16; /* Energy LSB */ 
 if (PacketCopy(Data,tmpData,FLOW_OFFSET)   == 0) return -3;
 ParsedData[2] = (short)ParseValue(tmpData); 
 if (PacketCopy(Data,tmpData,INLET_TEMPERATURE_OFFSET) == 0) return -4;
 ParsedData[3] = (short)ParseValue(tmpData); 
 if (PacketCopy(Data,tmpData,OUTLET_TEMPERATURE_OFFSET)  == 0) return -5;
 ParsedData[4] = (short)ParseValue(tmpData); 
 if (PacketCopy(Data,tmpData,TEMPERATURE_DIFFERENCE_OFFSET)  == 0) return -6;
 ParsedData[5] = (short)(ParseValue(tmpData) / 10);
 if (PacketCopy(Data,tmpData,POWER_OFFSET)   == 0) return -7;
 ParsedData[6] = (short)(ParseValue(tmpData) / 100);
 return 0;
}


int main(int argc, char *argv[])
{
	DaemonConfig   *cfg;
	PolluMbus     *mbinfo;
	IPCHandler     *ipc;
	int             fd;
	int 		ResponseStatus;
	int		i;
	short ParsedData[10];	
	unsigned char buf[1000];
	unsigned long buffer;
		

	cfg = new DaemonConfig("polludmn");

	if (cfg->Load(&argc, argv))
		return 1;
	mbinfo = new PolluMbus(cfg->GetDevice()->GetFirstRadio()->
				GetFirstUnit()->GetParamsCount(),
				cfg->GetDevice()->GetFirstRadio()->
				GetFirstUnit()->GetSendParamsCount());

	if (cfg->GetSingle()) {
		printf("\
line number: %d\n\
device: %ls\n\
params in: %d\n", cfg->GetLineNumber(), cfg->GetDevice()->GetPath().c_str(), mbinfo->m_params_count);
	}


	ipc = new IPCHandler(cfg);
	if (!cfg->GetSingle()) {
		if (ipc->Init())
			return 1;
	}

	
	sz_log(2, "starting main loop");
	mbinfo->SetNoData(ipc);
	while (true) {
		fd = mbinfo->InitComm(cfg->GetDevice()->GetPath(),
			      cfg->GetDevice()->GetSpeed(), 8, 2, EVEN);
		if (fd < 0){
			sz_log(2, "problem with serial port (open)\n");
			sleep(SLEEP_ERROR);
			continue;	
		}

		if (write(fd,SND_NKE,SND_NKE_LENGTH)<0){ //Sending first Question
			sz_log(2, "problem with serial port (open)\n");
			sleep(SLEEP_ERROR);
			continue;	
		}

		if (cfg->GetSingle()){
				fprintf(stderr,"Sending : SND_NKE\n");
			}

		ResponseStatus = mbinfo->GetResponse(fd,buf);
		if ((buf[0]!=RESPONSE_OK) || (ResponseStatus<0)) {
			mbinfo->SetNoData(ipc);
			if (cfg->GetSingle()){
				fprintf(stderr,"Error: NO DIALTONE\n");
			}
			if (ResponseStatus == GENERAL_ERROR){
				sz_log(2, "problem with serial port (read)\n");
				sleep(SLEEP_ERROR);
				continue;	
			}

		}
		else{
			if (write(fd,REQ_UD2,REQ_UD2_LENGTH)<0){
				sz_log(2, "problem with serial port (write)\n");
				sleep(SLEEP_ERROR);
				continue;	
			}
			
			if (cfg->GetSingle()){
				fprintf(stderr,"Sending : REQ_UD2\n");
			}
			sleep(1);
			ResponseStatus = mbinfo->GetResponse(fd,buf); //Sending second question
			if (ResponseStatus<0){ 
				mbinfo->SetNoData(ipc); 
				if (cfg->GetSingle()){
					fprintf(stderr,"Error: NO DIALTONE\n");
				}
				if (ResponseStatus == GENERAL_ERROR){
					sz_log(2, "problem with serial port (read)\n");
					sleep(SLEEP_ERROR);
					continue;	
				}
			}
			else{
				if (mbinfo->ParsePacket(buf, (unsigned int)ResponseStatus, ParsedData)==0){
					memcpy(ipc->m_read, ParsedData, mbinfo->m_params_count*sizeof(short));		
					if (cfg->GetSingle()){
						fprintf(stderr,"Received data :\n");
						buffer = 0;
						for (i=0;i<mbinfo->m_params_count;i++){
							fprintf(stderr,"ParsedData[%d] = %d\n",i,ParsedData[i]);
							/* Dodatkowe skladanie energy i power-a */
							switch (i){
								case 0:
									buffer = 0;
									buffer |= ParsedData[i] <<16 ; 
								break;
								case 1:
									buffer |= ParsedData[i] ; 
									fprintf(stderr, "Scalled Energy %lu \n",buffer);
								break;

								default:
									buffer = 0;
								break;
							}
						}
					
					}

				}
				else{
					mbinfo->SetNoData(ipc);
					if (cfg->GetSingle()){
						fprintf(stderr,"Error: NO DIALTONE\n");
					}

				}
			}

		}
		ipc->GoParcook();
		if (cfg->GetSingle()) {
			sleep(10); //Wait 10 sec in 'single' mode
		} else {
			sleep(290); //Wait almost 5 minutes
		}
		close(fd);
	}
//	free (ParsedData);
	return 0;
}
