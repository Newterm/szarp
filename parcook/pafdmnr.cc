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
 * pafdmnr.c - wersja pafdmn.c wspó³pracuj±ca z licznikiem
 * z Rawicza
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <errno.h>
#include <libxml/tree.h>
#include <termio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include "conversion.h"


#define _IPCTOOLS_H_
#define _HELP_H_
#define _ICON_ICON_
#include "szarp.h"
#include "msgtypes.h"

#include "ipchandler.h"
#include "liblog.h"
#include "custom_assert.h"

// #define WRITE_MESSAGE_SIZE 7
#define WRITE_MESSAGE_SIZE 11

#define READ_MESSAGE_SIZE 256

#define NUMBER_OF_VALS 9

// offset danych
#define OF -1

#if 0
void
Usage(char *name)
{
    printf
	("Pafal 2EC8 driver; can be used on both COM and Specialix ports\n");
    printf("Usage:\n");
    printf("      %s [s|d] <n> <port>\n", name);
    printf("Where:\n");
    printf("[s|d] - optional simple (s) or diagnostic (d) modes\n");
    printf("<n> - line number (necessary to read line<n>.cfg file)\n");
    printf("<port> - full port name, eg. /dev/term/01 or /dev/term/a12\n");
    printf("Comments:\n");
    printf("No parameters are taken from line<n>.cfg (base period = 1)\n");
    printf
	("Port parameters: 300 baud, 7E1; signals required: TxD, RxD, GND\n");
    exit(1);
}
#endif

#define SIZE_OF_BUF 8
unsigned char   outbuf[WRITE_MESSAGE_SIZE] =
    { '/', '?', '!', '\r', '\n', 6, '1', '0', 'A', '\r', '\n' };

int             buf[5];

class DaemonClass{
	
	
	public:
		int Simple; /** m_signle flag */
		int Diagno; /** m_diagno flag */
		int LineDes;
		int m_paramscount; /** m_paramscount flag */
    		int vals[NUMBER_OF_VALS];

	       	char *path_to_device;/** path to device e.g. /dev/ttyS0 */	
		DaemonClass(int _m_single, int _m_diagno, \
		int _m_paramscount, char *_path_to_device)\
		{Simple = _m_single; Diagno = _m_diagno; \
		if (Simple) Diagno=1; m_paramscount = _m_paramscount;\
		path_to_device=(char *)malloc(strlen(_path_to_device) + 1);\
		memcpy(path_to_device,_path_to_device,\
		strlen(_path_to_device) + 1);ASSERT(path_to_device!=NULL);\
		LineDes=0;};
		void ReadData();
		int ReadOutput(int *buf, unsigned short *size);
		void OpenLine();		
		void CloseLine(){close(LineDes);};		
		
		~DaemonClass(){free (path_to_device);};
	
	private:
		void PartitionMessage(unsigned char *inbuf, unsigned short lenght, int *buf, unsigned short *size);
};

void DaemonClass::OpenLine()
{
    int 	    serial;
    struct termios   rsconf;
    LineDes = open(path_to_device, O_RDWR | O_NDELAY | O_NONBLOCK);
    ASSERT(LineDes >= 0);
    tcgetattr(LineDes, &rsconf);
    rsconf.c_cflag = B300 | CS7 | CLOCAL | CREAD | CSTOPB | PARENB;
    rsconf.c_iflag = 0;
    rsconf.c_oflag = 0;
    rsconf.c_lflag = 0;
    rsconf.c_cc[VMIN] = 10;
    rsconf.c_cc[VTIME] = 0;
    tcsetattr(LineDes, TCSANOW, &rsconf);
    ioctl(LineDes, TIOCMGET, &serial);
    serial |= TIOCM_DTR; //DTR is HIGH TO SUPPLY OPTO 
    serial |= TIOCM_RTS; //RTS is HIGH TO SUPPLY OPTO
    ioctl(LineDes, TIOCMSET, &serial);
}

void DaemonClass::PartitionMessage(unsigned char *inbuf, unsigned short lenght, int *buf,
		 unsigned short *size)
{

    int             i;
    unsigned char   tempbuf[7];
    int             sign1;
    int             sign2;
    unsigned char   comma[3];

    int             PValue;
    int             QValue;

    int             IValue1;
    int             IValue2;
    int             IValue3;

    int             UValue1;
    int             UValue2;
    int             UValue3;

    int             FValue;

    int             value;

    PValue = 0;
    QValue = 0;

    IValue1 = 0;
    IValue2 = 0;
    IValue3 = 0;

    UValue1 = 0;
    UValue2 = 0;
    UValue3 = 0;

    FValue = 0;

    sign1 = inbuf[OF+168] - 48;
    sign2 = inbuf[OF+169] - 48;
    //sign1 = inbuf[OF+168];
    //sign2 = inbuf[OF+169];
    if (Diagno) {
	    printf("SIGN: %x %x\n", inbuf[OF+168], inbuf[OF+169]);
    }

    // Obliczanie mocy czynnej

    for (i = 0; i <= 16; i += 8) {

	tempbuf[0] = inbuf[OF+46 + i];
	tempbuf[1] = inbuf[OF+47 + i];
	tempbuf[2] = inbuf[OF+44 + i];
	tempbuf[3] = inbuf[OF+45 + i],
	tempbuf[4] = inbuf[OF+42 + i];
	tempbuf[5] = inbuf[OF+43 + i];
	tempbuf[6] = '\0';

	comma[0] = inbuf[OF+48 + i];
	comma[1] = inbuf[OF+49 + i];
	comma[2] = '\0';

	value = atoi((char*)tempbuf);

	if (i == 0) {
	    if ((sign1 & 8) == 8) {
		value = -value;
	    }
	}
	if (i == 8) {
	    if ((sign1 & 4) == 4)
		value = -value;
	}
	if (i == 16) {
	    if ((sign1 & 2) == 2)
		value = -value;
	}

	if (Diagno)
	    printf("MOC (%d): %s\n", i / 8, tempbuf);

	if (strcmp((char*)comma, "3F") == 0)
	    value = value / 10;
	if (strcmp((char*)comma, "3E") == 0)
	    value = value / 100;
	if (strcmp((char*)comma, "3D") == 0)
	    value = value / 1000;
	if (strcmp((char*)comma, "3C") == 0)
	    value = value / 10000;

	PValue += value;
    }

    // Obliczanie mocy biernej

    for (i = 0; i <= 16; i += 8) {

	tempbuf[0] = inbuf[OF+75 + i];
	tempbuf[1] = inbuf[OF+76 + i];
	tempbuf[2] = inbuf[OF+73 + i];
	tempbuf[3] = inbuf[OF+74 + i],
	tempbuf[4] = inbuf[OF+71 + i];
	tempbuf[5] = inbuf[OF+72 + i];
	tempbuf[6] = '\0';

	comma[0] = inbuf[OF+77 + i];
	comma[1] = inbuf[OF+78 + i];
	comma[2] = '\0';

	value = atoi((char*)tempbuf);

	if (i == 0) {
	    if ((sign2 & 8) == 8)
		value = -value;
	}
	if (i == 8) {
	    if ((sign2 & 4) == 4)
		value = -value;
	}
	if (i == 16) {
	    if ((sign2 & 2) == 2)
		value = -value;
	}

	if (Diagno)
	    printf("MOC BIERNA (%d): %s\n", i/8, tempbuf);

	if (strcmp((char*)comma, "3F") == 0)
	    value = value / 10;
	if (strcmp((char*)comma, "3E") == 0)
	    value = value / 100;
	if (strcmp((char*)comma, "3D") == 0)
	    value = value / 1000;
	if (strcmp((char*)comma, "3C") == 0)
	    value = value / 10000;

	QValue += value;
    }

    // Obliczanie napiecia

    for (i = 0; i <= 16; i += 8) {

	tempbuf[0] = inbuf[OF+104 + i];
	tempbuf[1] = inbuf[OF+105 + i];
	tempbuf[2] = inbuf[OF+102 + i];
	tempbuf[3] = inbuf[OF+103 + i],
	tempbuf[4] = inbuf[OF+100 + i];
	tempbuf[5] = inbuf[OF+101 + i];
	tempbuf[6] = '\0';

	comma[0] = inbuf[OF+106 + i];
	comma[1] = inbuf[OF+107 + i];
	comma[2] = '\0';

	value = atoi((char*)tempbuf);

	if (Diagno)
	    printf("V (%d): %s\n", i/8, tempbuf);

	if (strcmp((char*)comma, "3F") == 0)
	    value = value / 10;
	if (strcmp((char*)comma, "3E") == 0)
	    value = value / 100;
	if (strcmp((char*)comma, "3D") == 0)
	    value = value / 1000;
	if (strcmp((char*)comma, "3C") == 0)
	    value = value / 10000;

	if (i == 0)
	    UValue1 = value;
	if (i == 8)
	    UValue2 = value;
	if (i == 16)
	    UValue3 = value;
    }

    // Obliczanie pradu

    for (i = 0; i <= 16; i += 8) {

	tempbuf[0] = inbuf[OF+133 + i];
	tempbuf[1] = inbuf[OF+134 + i];
	tempbuf[2] = inbuf[OF+131 + i];
	tempbuf[3] = inbuf[OF+132 + i],
    	tempbuf[4] = inbuf[OF+129 + i];
	tempbuf[5] = inbuf[OF+130 + i];
	tempbuf[6] = '\0';

	comma[0] = inbuf[OF+135 + i];
	comma[1] = inbuf[OF+136 + i];
	comma[2] = '\0';

	value = atoi((char*)tempbuf);

	if (Diagno)
	    printf("I (%d): %s\n", i/8, tempbuf);

	if (strcmp((char*)comma, "3E") == 0)
	    value = value / 10;
	if (strcmp((char*)comma, "3D") == 0)
	    value = value / 100;
	if (strcmp((char*)comma, "3C") == 0)
	    value = value / 1000;

	if (i == 0)
	    IValue1 = value;
	if (i == 8)
	    IValue2 = value;
	if (i == 16)
	    IValue3 = value;
    }

    // Obliczanie fazy


    tempbuf[0] = inbuf[OF+162];
    tempbuf[1] = inbuf[OF+163];
    tempbuf[2] = inbuf[OF+160];
    tempbuf[3] = inbuf[OF+161],
    tempbuf[4] = inbuf[OF+158];
    tempbuf[5] = inbuf[OF+159];
    tempbuf[6] = '\0';

    comma[0] = inbuf[OF+164 + i];
    comma[1] = inbuf[OF+165 + i];
    comma[2] = '\0';

    value = atoi((char*)tempbuf);

    if (Diagno)
	printf("Fi (%d): %s\n", i/8, tempbuf);

    if (strcmp((char*)comma, "3F") == 0)
	value = value / 10;
    if (strcmp((char*)comma, "3E") == 0)
	value = value / 100;
    if (strcmp((char*)comma, "3D") == 0)
	value = value / 1000;
    if (strcmp((char*)comma, "3C") == 0)
	value = value / 10000;

    FValue = value;

    buf[0] = PValue;
    buf[1] = QValue;
    buf[2] = UValue1;
    buf[3] = UValue2;
    buf[4] = UValue3;
    buf[5] = IValue1;
    buf[6] = IValue2;
    buf[7] = IValue3;
    buf[8] = FValue;

    if (Diagno)
	printf(" %d,%d,%d,%d,%d,%d,%d,%d,%d\n", buf[0], buf[1], buf[2],
	       buf[3], buf[4], buf[5], buf[6], buf[7], buf[8]);
    *size = 8;
}


int DaemonClass::ReadOutput(int *buf, unsigned short *size)
{
    int             i,
                    j;
    unsigned short  lenght;
    unsigned char   inbuf[READ_MESSAGE_SIZE];
    unsigned char   test[6] = { 6, '1', '0', 'A', '\r', '\n' };
    int retval;
    fd_set rfds;
    struct timeval tv;
      
    /*
     * ustalenie dok³adnej postaci ramki - ilo¶æ odczytywanych rejestrów
     * (number) oraz adres pocz±tkowego rejestru odczytywanych danych
     */
    errno = 0;
    lenght = 180 + OF;
    memset(inbuf, 0, READ_MESSAGE_SIZE);
    /*
     * wys³anie komunikatu do urz±dzenia 
     */
    i = write(LineDes, outbuf, WRITE_MESSAGE_SIZE);
    if (Diagno)
	printf("frame %x%x%x%x%x%x%x%x%x%x%x was sent \n", outbuf[0],
	       outbuf[1], outbuf[2], outbuf[3], outbuf[4], outbuf[5],
	       outbuf[6], outbuf[7], outbuf[8], outbuf[9], outbuf[10]);
    if (i < 0)
	return -1;
    /*
     * d³ugo¶æ odczytywanej ramki - wynika z tego jak± wysy³amy ramkê do
     * licznika 
     */
    /*
     * odczyt odpowiedzi urz±dzenia 
     */
    //usleep (30000);
    sleep(2);
    i = 0;
    
    FD_ZERO(&rfds);
    FD_SET(LineDes, &rfds);
    tv.tv_sec = 30;
    tv.tv_usec = 0;
    retval = select(LineDes+1, &rfds, NULL, NULL, &tv);
  
    if (retval == -1){
	  perror("select()");
	  return -1 ;
    }
 	else
    if (retval) {
	   for (i = 0; read(LineDes, &inbuf[i], 1) == 1; i++) {
		   usleep(10000);
	   }
	   i--;

    }
      else return -1;
        
    if (i != 20) {
	if (Diagno)
    printf("Error %d bytes was received!\n", i);
	return -1;
    }
    
    for (j = 0; j < i; j++)
	if (Diagno)
	    printf(" %c", inbuf[j]);
    if (Diagno)
	printf(" was received \n");
	sleep(2) ; 

	
	
    i = write(LineDes, test, 6);
    if (i < 0)
	return -1;
    if (Diagno)
	printf("frame %d%c%c%c%c%c was sent \n", test[0], test[1],
	       test[2], test[3], test[4], test[5]);
    i = 0;
    sleep(10);
    retval = select(LineDes+1, &rfds, NULL, NULL, &tv);
  
    if (retval == -1){
	  perror("select()");
	  return -1 ;
    }
 	else
    if (retval){
	   for (i = 1; read(LineDes, &inbuf[i], 1) == 1; i++) {
		   usleep(10000);
	   }
	if (Diagno){	    
	    printf("Received data size :%d; Actual Data size: %d\n",i,lenght);
	}
    }
      else {
	      i = 0;
      }
   
    for (j = 0; j <= i; j++)
	if (Diagno)
	    printf("%c", inbuf[j]);
    if (Diagno)
	printf(" was received , %d \n", i);
    if (i != lenght) {
	if (Diagno)
	    printf("Error  %d bytes  was received (shold be %d)\n", i,lenght);
	return -1;
    }
    PartitionMessage(inbuf, i, buf, size);
    return i;
}


void DaemonClass::ReadData()
{
    unsigned short           size;
    int             
                    j,
                    res;

    vals[0] = vals[1] = vals[2] = vals[3] = vals[4] = SZARP_NO_DATA;

    j = 0;
    res = -1;

    while (j < 5 && res < 0) {

	if (this->ReadOutput(vals, &size) > 0)
	{
	    res = 1;
	    if (Diagno)
		printf("Odczyt danych: ");
	    if (Diagno)
		printf
		    ("vals[0] = %d , vals[1] = %d , vals[2] = %d , vals[3] = %d, vals[4] = %d, vals[5] = %d, vals[6] = %d, vals[7] = %d, vals[8] = %d\n",
		     vals[0], vals[1], vals[2], vals[3], vals[4], vals[5],
		     vals[6], vals[7], vals[8]);
	}

	else {
	    vals[0] = vals[1] = vals[2] = vals[3] = vals[4] = vals[5] =
		vals[6] = vals[7] = vals[8] = SZARP_NO_DATA;
	    if (Diagno)
		printf
		    ("Brak danych: vals[0] = vals[1] = vals[2] = vals[3] = vals[4] = SZARP_NO_DATA \n");
	}
	sleep(2);
	j++;
    }
}

int main(int argc, char *argv[])
{
	
	int i;
	DaemonConfig *cfg = new DaemonConfig("pafdmnr");
	IPCHandler   *ipc;
	DaemonClass  *pafdmninfo;

	cfg->SetUsageHeader("Pafal 2EC8 driver; can be used on both COM and Specialix ports\nPort parameters: 300 baud, 7E1; signals required: TxD, RxD, GND\n");

	if (cfg->Load(&argc, argv))
		return 1;
	pafdmninfo = new DaemonClass((int)cfg->GetSingle(), (int)cfg->GetDiagno(), (int)cfg->GetDevice()->GetFirstRadio()->GetFirstUnit()->GetParamsCount(), (char *) SC::S2A(cfg->GetDevice()->GetPath()).c_str());

	if (pafdmninfo->m_paramscount != NUMBER_OF_VALS){
		sz_log(0, "amount of params must be %d",NUMBER_OF_VALS);
		delete pafdmninfo;
		return 1;
	}

	if (cfg->GetSingle()) {
		printf("\
line number: %d\n\
device: %ls\n\
params in: %d\n", cfg->GetLineNumber(), cfg->GetDevice()->GetPath().c_str(), pafdmninfo->m_paramscount);
	}
	
	try {
		auto ipc_ = std::unique_ptr<IPCHandler>(new IPCHandler(*cfg));
		ipc = ipc_.release();
	} catch(...) {
		return 1;
	}

	for (i=0;i<pafdmninfo->m_paramscount;i++){
		ipc->m_read[i] = SZARP_NO_DATA;
	}

	while (1) {
		pafdmninfo->OpenLine();
		pafdmninfo->ReadData();
		for (i=0;i<pafdmninfo->m_paramscount;i++)
			ipc->m_read[i] = pafdmninfo->vals[i]; 
		ipc->GoParcook();
		sleep(5);
        	pafdmninfo->CloseLine();	
    	}

	delete pafdmninfo;
	return 0;
}

