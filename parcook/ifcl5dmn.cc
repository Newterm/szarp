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
 * Ifcl5dmn 
 * Pawe³ Kolega
 * demon dla ciep³omierza Infocal 5 - Firmy DanfoSS/Siemens
 * Protokol MBUS, engine sciagniety z Infcl5dmn
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
#include "conversion.h"
#include "liblog.h"

#define DAEMON_ERROR 1

#define SND_NKE_LENGTH 5
#define REQ_UD2_LENGTH 5

#define GENERAL_ERROR -1
#define TIMEOUT_ERROR -2
#define PACKET_SIZE_ERROR -1

/* Dlugosci odpowiedzi na podstawie ktorej dobierane sa OFFSET-y */
#define RECEIVED_DATA_SIZE_84 84 /* Stan dotychczasowy */
#define RECEIVED_DATA_SIZE_83 83 /* Nowosc */

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
 * Infcl5Mbus communication config.
 */
class           Infcl5Mbus {
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
	Infcl5Mbus(int params, int sends) {
		assert(params >= 0);
		assert(sends >= 0);

		m_params_count = params;
		m_sends_count = sends;
	}

	~Infcl5Mbus() {
	}



	/**
	 * Filling m_read structure SZARP_NO_DATA value
	 */
	void            SetNoData(IPCHandler * ipc);

	/**
	 * Function oppening serial port special for Infocal 5 Heat Meter
	 * @param Device Path to device np "/dev/ttyS0"
	 */

	int OpenLine(const char *line) ;

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
	unsigned long ParseValue2(unsigned char *Packet);

	/**
	 * Copies one packet from Offset
	 * @param DataIn pointer to Data Source
	 * @param DataOut pointer to Data destination
	 * @param Offset Copy offset
	 * @return - Length of packet
	 */

	unsigned short PacketCopy(unsigned char *DataIn, unsigned char *DataOut,unsigned short Offset);
	unsigned short PacketCopy2(unsigned char *DataIn, unsigned char *DataOut,unsigned short Offset);
};


void Infcl5Mbus::SetNoData(IPCHandler * ipc)
{
	int i;

	for (i = 0; i < ipc->m_params_count; i++)
		ipc->m_read[i] = SZARP_NO_DATA;

}


int Infcl5Mbus::OpenLine(const char *line)
{
	int             linedes;
	struct termio   rsconf;
	
	linedes = open(line, O_RDWR | O_NDELAY | O_NONBLOCK);
	if (linedes < 0)
		return (-1);
	ioctl(linedes, TCGETA, &rsconf);
	rsconf.c_iflag = 0;
	rsconf.c_oflag = 0;
	rsconf.c_cflag = CS8 | CLOCAL | CREAD | B300 | PARENB;
	rsconf.c_lflag = 0;
	rsconf.c_cc[4] = 0;
	rsconf.c_cc[5] = 0;
	ioctl(linedes, TCSETA, &rsconf);
	return (linedes);
}

int Infcl5Mbus::GetResponse(int fd, unsigned char *Response)
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


unsigned long Infcl5Mbus::pow10(int n)
{
	int i;
	unsigned long tmp;
	tmp = 1;
	for (i=0;i<n;i++) tmp *= 10;
	return tmp;
}

/* W M-BUSie  Dane zapisywane s± w postaci BCD  */
unsigned long Infcl5Mbus::ParseValue(unsigned char *Packet)
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

unsigned long Infcl5Mbus::ParseValue2(unsigned char *Packet)
{
 unsigned long tmpValue;
 unsigned int length ;
 unsigned int i;
 tmpValue = 0 ;
 if (Packet[0] - 0x06 <=0) return 0;
 length = Packet[0] +1 - 0x06 ;
 for (i=2;i<length;i++){
	 tmpValue += ((Packet[i]/16)*10 + Packet[i] % 16 )*pow10((i-2)*2);
 }
 return tmpValue ;
}


unsigned short Infcl5Mbus::PacketCopy(unsigned char *DataIn, unsigned char *DataOut,unsigned short Offset)
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

unsigned short Infcl5Mbus::PacketCopy2(unsigned char *DataIn, unsigned char *DataOut,unsigned short Offset)
{
 unsigned short PacketSize;
 unsigned short i ;
 if (DataIn[Offset] - 0x06 <=0) return 0;
 PacketSize =  DataIn[Offset] +1 - 0x06 ;

 for (i=Offset;i<(Offset+PacketSize);i++){
	 DataOut[i-Offset] = DataIn[i];
 }
 DataOut[PacketSize]=0;
 return PacketSize;
}


char Infcl5Mbus::ParsePacket(unsigned char *Data, unsigned int DataSize, short *ParsedData)
{
 unsigned char tmpData[250];
/* Tutaj podajemy offsety pozycji -1 (tak jak index tablicy danych) */
	 unsigned short ENERGY_OFFSET = 26;
	 unsigned short FLOW_OFFSET = 45;
	 unsigned short INLET_TEMPERATURE_OFFSET = 51;
	 unsigned short OUTLET_TEMPERATURE_OFFSET = 56;
	 unsigned short TEMPERATURE_DIFFERENCE_OFFSET = 61;
	 unsigned short  POWER_OFFSET = 39;
 
if (DataSize == RECEIVED_DATA_SIZE_84){ /* stan dotychczasowy  */
	ENERGY_OFFSET = 26;
	FLOW_OFFSET = 45;
	INLET_TEMPERATURE_OFFSET = 51;
	OUTLET_TEMPERATURE_OFFSET = 56;
	TEMPERATURE_DIFFERENCE_OFFSET = 61;
	POWER_OFFSET = 39;
}
if (DataSize == RECEIVED_DATA_SIZE_83){ /* Nowosc */
	ENERGY_OFFSET = 26;
	FLOW_OFFSET = 44;
	INLET_TEMPERATURE_OFFSET = 50;
	OUTLET_TEMPERATURE_OFFSET = 55;
	TEMPERATURE_DIFFERENCE_OFFSET = 60;
	POWER_OFFSET = 38;
}

unsigned long buffer;
 if (DataSize<=0) return -1;
 
 if (PacketCopy2(Data,tmpData,ENERGY_OFFSET) == 0) return -2;
 buffer = ParseValue2(tmpData);
 if (DataSize == RECEIVED_DATA_SIZE_84){ /* stan dotychczasowy  */
	buffer = ParseValue2(tmpData);
 }
 if (DataSize == RECEIVED_DATA_SIZE_83){ /* Nowosc */
	buffer = ParseValue(tmpData);
 }

 buffer /= 100 ;
 ParsedData[0] = (short)(buffer & 0x0000ffff); /* Energy MSB */
 ParsedData[1] = (short)((buffer & 0xffff0000) >>16); /* Energy LSB */ 
 if (PacketCopy(Data,tmpData,FLOW_OFFSET)   == 0) return -3;
 buffer = ParseValue(tmpData);
 ParsedData[2] = (short)(buffer & 0x0000ffff); /* Energy MSB */
 ParsedData[3] = (short)((buffer & 0xffff0000) >>16); /* Energy LSB */ 
 if (PacketCopy(Data,tmpData,INLET_TEMPERATURE_OFFSET) == 0) return -4;
 
 buffer = ParseValue(tmpData);
 ParsedData[4] =(short) (buffer & 0x0000ffff); /* Energy MSB */
 ParsedData[5] = (short)((buffer & 0xffff0000) >>16); /* Energy LSB */
 if (PacketCopy(Data,tmpData,OUTLET_TEMPERATURE_OFFSET)  == 0) return -5;
 buffer = ParseValue(tmpData);

 ParsedData[6] = (short)(buffer & 0x0000ffff); /* Energy MSB */
 ParsedData[7] = (short)((buffer & 0xffff0000) >>16); /* Energy LSB */ 
 if (PacketCopy(Data,tmpData,TEMPERATURE_DIFFERENCE_OFFSET)  == 0) return -6;
 buffer = ParseValue(tmpData);
 
 ParsedData[8] = (short)(buffer & 0x0000ffff); /* Energy MSB */
 ParsedData[9] = (short)((buffer & 0xffff0000) >>16); /* Energy LSB */ 
 if (PacketCopy(Data,tmpData,POWER_OFFSET)   == 0) return -7;
 buffer = ParseValue(tmpData);
 buffer /= 100 ;
 ParsedData[10] = (short)(buffer & 0x0000ffff); /* Energy MSB */
 ParsedData[11] = (short)((buffer & 0xffff0000) >>16); /* Energy LSB */ 
/* dummy to avoid varnings */
 
 return 0;
}


int main(int argc, char *argv[])
{
	DaemonConfig   *cfg;
	Infcl5Mbus     *mbinfo;
	IPCHandler     *ipc;
	int             fd;
	int 		ResponseStatus;
	int		i;
	short ParsedData[12];	
	unsigned char buf[1000];
	unsigned long buffer;
		

	cfg = new DaemonConfig("polludmn");

	if (cfg->Load(&argc, argv))
		return 1;
	mbinfo = new Infcl5Mbus(cfg->GetDevice()->GetFirstRadio()->
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
	fd = mbinfo->OpenLine(SC::S2A(cfg->GetDevice()->GetPath()).c_str()) ;
//	fd = mbinfo->InitComm(cfg->GetDevice()->GetPath(),
//			      cfg->GetDevice()->GetSpeed(), 8, 1, 0);
		write(fd,SND_NKE,SND_NKE_LENGTH); //Sending first Question

		if (cfg->GetSingle()){
				fprintf(stderr,"Sending : SND_NKE\n");
			}

		ResponseStatus = mbinfo->GetResponse(fd,buf);
		if ((buf[0]!=RESPONSE_OK) || (ResponseStatus<0)) {
			mbinfo->SetNoData(ipc);
			if (cfg->GetSingle()){
				fprintf(stderr,"Error: NO DIALTONE\n");
			}

		}
		else{
			write(fd,REQ_UD2,REQ_UD2_LENGTH);
			if (cfg->GetSingle()){
				fprintf(stderr,"Sending : REQ_UD2\n");
			}
			sleep(5);
			ResponseStatus = mbinfo->GetResponse(fd,buf); //Sending second question
			if (ResponseStatus<0){ 
				mbinfo->SetNoData(ipc); 
				if (cfg->GetSingle()){
					fprintf(stderr,"Error: NO DIALTONE\n");
				}
			}
			else{
				if (mbinfo->ParsePacket(buf, (unsigned int)ResponseStatus, ParsedData)==0){
					if (cfg->GetSingle()){
						for (i=0;i<ResponseStatus;i++){
						fprintf(stderr,"Data[%d]=%x\n",i,buf[i]);
					}
					}
					memcpy(ipc->m_read, ParsedData, mbinfo->m_params_count*sizeof(short));		
					if (cfg->GetSingle()){
						fprintf(stderr,"Received data :\n");
						buffer = 0;
						for (i=0;i<mbinfo->m_params_count;i++){
							fprintf(stderr,"ParsedData[%d] = %d\n",i,ParsedData[i]);
							/* Dodatkowe skladanie energy i power-a */
							switch (i){
								case 0:
									buffer = ParsedData[i] ; 
								break;
								case 1:
									buffer |= ParsedData[i] <<16 ; 
									fprintf(stderr, "Scalled Energy %lu \n",buffer);
								break;
								case 2:
									buffer = ParsedData[i] ;
								break;
								case 3:
									buffer |= ParsedData[i] <<16 ; 
									fprintf(stderr, "Scalled Flow %lu \n",buffer);
								break;
								case 4:
									buffer = ParsedData[i] ;
								break;
								case 5:
									buffer |= ParsedData[i] <<16 ; 
									fprintf(stderr, "Scalled Inlet temperature %lu \n",buffer);
								break;
	
								case 6:
									buffer = ParsedData[i] ;
								break;
								case 7:
									buffer |= ParsedData[i] <<16 ; 
									fprintf(stderr, "Scalled Outled temperature %lu \n",buffer);
								break;
								case 8:
									buffer = ParsedData[i] ; 
								break;
								case 9:
									buffer |= ParsedData[i] <<16 ;
									fprintf(stderr, "Scalled Diff temperature %lu \n",buffer);
								break;
								case 10:
									buffer = ParsedData[i] ; 
								break;
								case 11:
									buffer |= ParsedData[i] <<16 ; 
									fprintf(stderr, "Scalled Power %lu \n",buffer);
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
		sleep(10); //Wait 10 minutes
		close(fd);
	}
//	free (ParsedData);
	return 0;
}
