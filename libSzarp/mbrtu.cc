/* 
  libSzarp - SZARP library

*/
/*
 * Biblioteka modbus RTU Pawe³ Kolega 
 */

/*
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef MINGW32
#include "mbrtu.h"
#include <termio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <signal.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>

char mbrtu_single;

int InitComm(const char *Device, long BaudRate, unsigned char DataBits,
	     unsigned char StopBits, unsigned char Parity)
{
	int CommId;
	struct termio rsconf;

	// CommId = open(Device, O_RDWR | O_NOCTTY | O_NONBLOCK); //To bylo
	// do tej pory
	CommId = open(Device, O_RDWR | O_NDELAY | O_NONBLOCK);
	// CommId = open(Device, O_RDWR | O_NOCTTY );
	// CommId = open(Device, O_RDWR | O_NOCTTY | O_NDELAY);
	if (CommId < 0)
		return CommId;

	ioctl(CommId, TCGETA, &rsconf);

	switch (BaudRate) {
	case 300:
		rsconf.c_cflag = B300;
		break;
	case 600:
		rsconf.c_cflag = B600;
		break;
	case 1200:
		rsconf.c_cflag = B1200;
		break;
	case 2400:
		rsconf.c_cflag = B2400;
		break;
	case 4800:
		rsconf.c_cflag = B4800;
		break;
	case 9600:
		rsconf.c_cflag = B9600;
		break;
	case 19200:
		rsconf.c_cflag = B19200;
		break;
	case 38400:
		rsconf.c_cflag = B38400;
		break;
	case 115200:
		rsconf.c_cflag = B115200;
		break;
	default:
		rsconf.c_cflag = B9600;
		break;
	}

	switch (DataBits) {
	case 7:
		rsconf.c_cflag |= CS7;
		break;
	case 8:
		rsconf.c_cflag |= CS8;
		break;
	default:
		rsconf.c_cflag |= CS8;
		break;
	}

	switch (StopBits) {
	case 1:
		rsconf.c_cflag |= 0;
		break;
	case 2:
		rsconf.c_cflag |= CSTOPB;
		break;
	default:
		rsconf.c_cflag |= 0;
		break;
	}

	switch (Parity) {
	case NO_PARITY:
		rsconf.c_cflag |= 0;
		break;
	case EVEN:
		rsconf.c_cflag |= PARENB;
		break;
	case ODD:
		rsconf.c_cflag |= PARENB | PARODD;
		break;
	default:
		rsconf.c_cflag |= 0;
		break;
	}
	// fcntl(CommId, F_SETFL, FASYNC); /* Ta linia robi blocka na ridzie
	// */
//      tcgetattr(CommId, &rsconf);//Jak nizej
	rsconf.c_cflag |= CLOCAL | CREAD;

	rsconf.c_iflag = 0;
	rsconf.c_oflag = 0;
	rsconf.c_lflag = 0;
//      rsconf.c_cc[VMIN] = 1;//Po ostatniej zmianie
//      rsconf.c_cc[VTIME] = 0;//Jak wyzej
	rsconf.c_cc[4] = 0;
	rsconf.c_cc[5] = 0;
	ioctl(CommId, TCSETA, &rsconf);
//                          
	// tcflush(CommId, TCIFLUSH);
	// tcflush(CommId, TCOFLUSH);
//      tcsetattr(CommId, TCSANOW, &rsconf);//Jak wyzej
	return CommId;
}

void ClearPort(int CommId, int Mode)
{
	switch (Mode) {
	case IN_BUF:
		tcflush(CommId, TCIFLUSH);
		break;
	case OUT_BUF:
		tcflush(CommId, TCOFLUSH);
		break;
	case BOTH_BUF:
		tcflush(CommId, TCIFLUSH);
		tcflush(CommId, TCOFLUSH);
		break;
	default:
		tcflush(CommId, TCIFLUSH);
		tcflush(CommId, TCOFLUSH);
		break;
	}
}

unsigned short CRC16(unsigned char *Packet, int PacketSize)
{
	unsigned short CRC, temp, carry;
	int i, j;

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
	if (mbrtu_single == TRUE) {
		fprintf(stderr, "\nCRC = 0x%x\n", CRC);
	}
	return CRC;
}

void int2bin(signed short iint, unsigned short *oBIN)
{
	unsigned short temp;
	memcpy(&temp, &iint, sizeof(short));
	*oBIN = temp;
}

void bin2int(unsigned short iBIN, signed short *oint)
{
	signed short temp;
	memcpy(&temp, &iBIN, sizeof(short));
	*oint = temp;
}

void float2bin(float ifloat, unsigned short *bMSB, unsigned short *bLSB)
{
	unsigned long temp;
	memcpy(&temp, &ifloat, sizeof(float));
	*bMSB = temp >> 16;
	*bLSB = temp & 0x0000ffff;
	if (mbrtu_single == TRUE) {
		fprintf(stderr, "\nfloat = %f\n", ifloat);
		fprintf(stderr, "\ndec = %lu\n", temp);
	}

}

void bin2float(unsigned short bMSB, unsigned short bLSB, float *ofloat)
{
	unsigned long temp;
	float tempfloat;

	temp = 0x0;
	temp |= (bMSB << 16);
	temp |= bLSB;
	memcpy(&tempfloat, &temp, sizeof(float));
	*ofloat = tempfloat;
}

signed short float2int(float ifloat, unsigned short prec)
{
	if (ifloat > (float)SHRT_MAX)
		ifloat = (float)SHRT_MAX;
	if (ifloat < (float)SHRT_MIN)
		ifloat = (float)SHRT_MIN;
	switch (prec) {

	case 0:
		return (signed short)ceil(ifloat);
		break;
	case 1:
		return (signed short)ceil(ifloat);
		break;
	case 10:
		return (signed short)ceil(ifloat * 10);
		break;
	case 100:
		return (signed short)ceil(ifloat * 100);
		break;
	case 1000:
		return (signed short)ceil(ifloat * 1000);
		break;
	}
	return 0;
}

/* int 4 */
signed int float2int4(float ifloat, unsigned short prec)
{
	if (ifloat > (float)INT_MAX)
		ifloat = (float)INT_MAX;
	if (ifloat < (float)INT_MIN)
		ifloat = (float)INT_MIN;
	switch (prec) {

	case 0:
		return (signed int)ceil(ifloat);
		break;
	case 1:
		return (signed int)ceil(ifloat);
		break;
	case 10:
		return (signed int)ceil(ifloat * 10);
		break;
	case 100:
		return (signed int)ceil(ifloat * 100);
		break;
	case 1000:
		return (signed int)ceil(ifloat * 1000);
		break;
	}
	return 0;
}

float int2float(signed short iint, unsigned short prec)
{
	switch (prec) {

	case 0:
		return (float)iint;
		break;
	case 1:
		return (float)iint;
		break;
	case 10:
		return ((float)iint) / 10;
		break;
	case 100:
		return ((float)iint) / 100;
		break;
	case 1000:
		return ((float)iint) / 1000;
		break;
	}
	return 0;
}

unsigned short bcd2int(unsigned short val, int *ret_code)
{
	assert(ret_code != NULL);

	unsigned short ret = 0;
	*ret_code = 1;

	for (int i = 3; i >= 0; i--) {
		unsigned short tmp = (val >> (4 * i)) & 0xF;
		if (tmp > 9) {
			return 0;
		}
		ret = ret * 10 + tmp;
	}
	*ret_code = 0;
	return ret;
}

unsigned short int2bcd(unsigned short val, int *ret_code)
{
	assert(ret_code != NULL);

	unsigned short ret = 0;
	if (val > 9999) {
		*ret_code = 1;
		return 0;
	}
	*ret_code = 0;

	for (int i = 0; i < 4; i++) {
		unsigned short tmp = val % 10;
		ret = ret | (tmp << (4 * i));
		val = val / 10;
	}
	return ret;
}

void CreateMasterPacket(tWMasterFrame MasterFrame, unsigned char *oPacket,
			int MaxPacketSize, int *PacketSize)
{
	unsigned short crc;
	unsigned char *buffer;
	unsigned char u;

	*PacketSize = 0;

	switch (MasterFrame.FunctionId) {
	case MB_F_RHR:		/* Funkcja 3 (Odczyt bloku rejestrów) */
		/*
		 * W funkcji 3 pakiet jest zawsze tej samej wielko¶ci 
		 */
		*PacketSize = 8;
		buffer = (unsigned char *)malloc(*PacketSize);
		buffer[0] = MasterFrame.DeviceId;
		buffer[1] = MasterFrame.FunctionId;
		buffer[2] = ((MasterFrame.Address & 0xff00) >> 8);
		buffer[3] = MasterFrame.Address & 0x00ff;
		buffer[4] = ((MasterFrame.DataSize & 0xff00) >> 8);
		buffer[5] = MasterFrame.DataSize & 0x00ff;
		crc = CRC16(buffer, *PacketSize - 2);
		buffer[6] = (crc & 0xff00) >> 8;
		buffer[7] = crc & 0xff;
		memcpy(oPacket, buffer, *PacketSize);
		free(buffer);
		break;

	case MB_F_RIR:		/* Funkcja 4 (Odczyt bloku wej¶æ) */
		/*
		 * W funkcji 4 pakiet jest zawsze tej samej wielko¶ci 
		 */
		*PacketSize = 8;
		buffer = (unsigned char *)malloc(*PacketSize);
		buffer[0] = MasterFrame.DeviceId;
		buffer[1] = MasterFrame.FunctionId;
		buffer[2] = ((MasterFrame.Address & 0xff00) >> 8);
		buffer[3] = MasterFrame.Address & 0x00ff;
		buffer[4] = ((MasterFrame.DataSize & 0xff00) >> 8);
		buffer[5] = MasterFrame.DataSize & 0x00ff;
		crc = CRC16(buffer, *PacketSize - 2);
		buffer[6] = (crc & 0xff00) >> 8;
		buffer[7] = crc & 0xff;
		memcpy(oPacket, buffer, *PacketSize);
		free(buffer);
		break;

	case MB_F_WSC:		/* Funkcja 5 (Zapis pojedyñczej ceWy) */
		/*
		 * W funkcji 5 pakiet jest zawsze tej samej wielko¶ci 
		 */
		*PacketSize = 8;
		buffer = (unsigned char *)malloc(*PacketSize);
		buffer[0] = MasterFrame.DeviceId;
		buffer[1] = MasterFrame.FunctionId;
		buffer[2] = ((MasterFrame.Address & 0xff00) >> 8);
		buffer[3] = MasterFrame.Address & 0x00ff;
		buffer[4] = ((MasterFrame.Body[0] & 0xff00) >> 8);
		buffer[5] = MasterFrame.Body[0] & 0xff;
		crc = CRC16(buffer, *PacketSize - 2);
		buffer[6] = (crc & 0xff00) >> 8;
		buffer[7] = crc & 0xff;
		memcpy(oPacket, buffer, *PacketSize);
		free(buffer);
		break;

	case MB_F_WSR:		/* Funkcja 6 (Zapis pojedyñczego rehestru) */
		/*
		 * W funkcji 6 pakiet jest zawsze tej samej wielko¶ci 
		 */
		*PacketSize = 8;
		buffer = (unsigned char *)malloc(*PacketSize);
		buffer[0] = MasterFrame.DeviceId;
		buffer[1] = MasterFrame.FunctionId;
		buffer[2] = ((MasterFrame.Address & 0xff00) >> 8);
		buffer[3] = MasterFrame.Address & 0x00ff;
		buffer[4] = ((MasterFrame.Body[0] & 0xff00) >> 8);
		buffer[5] = MasterFrame.Body[0] & 0xff;
		crc = CRC16(buffer, *PacketSize - 2);
		buffer[6] = (crc & 0xff00) >> 8;
		buffer[7] = crc & 0xff;
		memcpy(oPacket, buffer, *PacketSize);
		free(buffer);
		break;

	case MB_F_WMR:		/* Funkcja 10 (Zapis do wielu rejestrów) */
		*PacketSize = 1 + 1 + 2 + 2 + 1 + 2 * MasterFrame.DataSize + 2;
		buffer = (unsigned char *)malloc(*PacketSize);
		buffer[0] = MasterFrame.DeviceId;
		buffer[1] = MasterFrame.FunctionId;
		buffer[2] = ((MasterFrame.Address & 0xff00) >> 8);
		buffer[3] = MasterFrame.Address & 0x00ff;
		buffer[4] = ((MasterFrame.DataSize & 0xff00) >> 8);
		buffer[5] = MasterFrame.DataSize & 0x00ff;
		buffer[6] = (MasterFrame.DataSize & 0xff) << 1;
		for (u = 0; u < MasterFrame.DataSize; u++) {
			buffer[7 + 2 * u] = (MasterFrame.Body[u] & 0xff00) >> 8;
			buffer[8 + 2 * u] = (MasterFrame.Body[u] & 0xff);
		}
		crc = CRC16(buffer, *PacketSize - 2);
		buffer[*PacketSize - 2] = (crc & 0xff00) >> 8;
		buffer[*PacketSize - 1] = crc & 0xff;
		memcpy(oPacket, buffer, *PacketSize);
		free(buffer);
		break;
	}
	if (mbrtu_single == TRUE) {
		int debugi;

		fprintf(stderr, "\n");
		for (debugi = 0; debugi < *PacketSize; debugi++) {
			fprintf(stderr, "buffer[%d] = 0x%x\n", debugi,
				oPacket[debugi]);
		}
	}
}

unsigned short DecodeMasterPacket(unsigned char *iPacket, int MaxPacketSize,
				  tRMasterFrame * MasterFrame)
{
	unsigned char u;
	unsigned short us;
	unsigned short crc = 0;

	/*
	 * Inicjalizacja parametrów 
	 */
	MasterFrame->DeviceId = 0;
	MasterFrame->FunctionId = 0;
	MasterFrame->Address = 0;
	MasterFrame->DataSize = 0;
	for (us = 0; us < MAX_BODY_SIZE; us++)
		MasterFrame->Body[us] = 0;
	MasterFrame->CRC = 0;
	MasterFrame->PacketSize = 0;

	/*
	 * Wspólne parametry 
	 */
	MasterFrame->DeviceId = iPacket[0];
	MasterFrame->FunctionId = iPacket[1];
	MasterFrame->Address = 0;
	MasterFrame->Address = (iPacket[2] << 8) | (iPacket[3]);
	if (mbrtu_single == TRUE) {
		fprintf(stderr, "MasterFrame->DeviceId = 0x%x\n",
			MasterFrame->DeviceId);
		fprintf(stderr, "MasterFrame->FunctionId = 0x%x\n",
			MasterFrame->FunctionId);
		fprintf(stderr, "MasterFrame->Address = 0x%x\n",
			MasterFrame->Address);
	}
	switch (MasterFrame->FunctionId) {
	case MB_F_RHR:		/* Funkcja 3 (Odczyt bloku rejestrów) */
		/*
		 * W funkcji 3 pakiet jest zawsze tej samej wielko¶ci 
		 */
		MasterFrame->PacketSize = 8;
		if (mbrtu_single == TRUE) {
			fprintf(stderr,
				"MasterFrame->PacketSize = %d\n",
				MasterFrame->PacketSize);
		}
		MasterFrame->DataSize = 0;
		MasterFrame->DataSize = (iPacket[4] << 8) | (iPacket[5]);
		if (mbrtu_single == TRUE) {
			fprintf(stderr,
				"MasterFrame->DataSize = %d\n",
				MasterFrame->DataSize);
		}
		MasterFrame->CRC = 0;
		MasterFrame->CRC = (iPacket[6] << 8) | (iPacket[7]);

		if (mbrtu_single == TRUE) {
			fprintf(stderr, "MasterFrame->CRC = 0x%x\n",
				MasterFrame->CRC);
		}
		crc = CRC16(iPacket, MasterFrame->PacketSize - 2);
		break;

	case MB_F_RIR:		/* Funkcja 4 (Odczyt bloku wej¶æ) */
		/*
		 * W funkcji 4 pakiet jest zawsze tej samej wielko¶ci 
		 */
		MasterFrame->PacketSize = 8;
		if (mbrtu_single == TRUE) {
			fprintf(stderr,
				"MasterFrame->PacketSize = %d\n",
				MasterFrame->PacketSize);
		}
		MasterFrame->DataSize = 0;
		MasterFrame->DataSize = (iPacket[4] << 8) | (iPacket[5]);
		if (mbrtu_single == TRUE) {
			fprintf(stderr,
				"MasterFrame->DataSize = %d\n",
				MasterFrame->DataSize);
		}
		MasterFrame->CRC = 0;
		MasterFrame->CRC = (iPacket[6] << 8) | (iPacket[7]);
		if (mbrtu_single == TRUE) {
			fprintf(stderr, "MasterFrame->CRC = 0x%x\n",
				MasterFrame->CRC);
		}
		crc = CRC16(iPacket, MasterFrame->PacketSize - 2);
		break;

	case MB_F_WSC:		/* Funkcja 5 (Zapis pojedyñczej ceWy) */
		/*
		 * W funkcji 5 pakiet jest zawsze tej samej wielko¶ci 
		 */
		MasterFrame->PacketSize = 8;
		if (mbrtu_single == TRUE) {
			fprintf(stderr,
				"MasterFrame->PacketSize = %d\n",
				MasterFrame->PacketSize);
		}
		MasterFrame->Body[0] = 0;
		MasterFrame->Body[0] = (iPacket[4] << 8) | (iPacket[5]);
		if (mbrtu_single == TRUE) {
			fprintf(stderr,
				"MasterFrame->Body[0] = 0x%x\n",
				MasterFrame->Body[0]);
		}
		MasterFrame->CRC = 0;
		MasterFrame->CRC = (iPacket[6] << 8) | (iPacket[7]);
		if (mbrtu_single == TRUE) {
			fprintf(stderr, "MasterFrame->CRC = 0x%x\n",
				MasterFrame->CRC);
		}
		crc = CRC16(iPacket, MasterFrame->PacketSize - 2);
		break;

	case MB_F_WSR:		/* Funkcja 6 (Zapis pojedyñczego rehestru) */
		/*
		 * W funkcji 6 pakiet jest zawsze tej samej wielko¶ci 
		 */
		MasterFrame->PacketSize = 8;
		if (mbrtu_single == TRUE) {
			fprintf(stderr,
				"MasterFrame->PacketSize = %d\n",
				MasterFrame->PacketSize);
		}
		MasterFrame->Body[0] = 0;
		MasterFrame->Body[0] = (iPacket[4] << 8) | (iPacket[5]);
		if (mbrtu_single == TRUE) {
			fprintf(stderr,
				"MasterFrame->Body[0] = 0x%x\n",
				MasterFrame->Body[0]);
		}
		MasterFrame->CRC = 0;
		MasterFrame->CRC = (iPacket[6] << 8) | (iPacket[7]);
		if (mbrtu_single == TRUE) {
			fprintf(stderr, "MasterFrame->CRC = 0x%x\n",
				MasterFrame->CRC);
		}
		crc = CRC16(iPacket, MasterFrame->PacketSize - 2);
		break;

	case MB_F_WMR:		/* Funkcja 10 (Zapis do wielu rejestrów) */
		MasterFrame->DataSize = (iPacket[4] << 8) | (iPacket[5]);
		if (mbrtu_single == TRUE) {
			fprintf(stderr,
				"MasterFrame->DataSize = %d\n",
				MasterFrame->DataSize);
		}
		MasterFrame->PacketSize =
		    1 + 1 + 2 + 2 + 1 + 2 * MasterFrame->DataSize + 2;
		if (mbrtu_single == TRUE) {
			fprintf(stderr,
				"MasterFrame->PacketSize = %d\n",
				MasterFrame->PacketSize);
		}
		for (u = 0; u < MasterFrame->DataSize; u++) {
			MasterFrame->Body[u] =
			    (iPacket[7 + 2 * u] << 8) | (iPacket[8 + 2 * u]);
			if (mbrtu_single == TRUE) {
				fprintf(stderr,
					"MasterFrame->Body[%d] = 0x%x\n",
					u, MasterFrame->Body[u]);
			}
		}
		MasterFrame->CRC = 0;
		MasterFrame->CRC =
		    (iPacket[MasterFrame->PacketSize - 2] << 8) |
		    (iPacket[MasterFrame->PacketSize - 1]);
		if (mbrtu_single == TRUE) {
			fprintf(stderr, "MasterFrame->CRC = 0x%x\n",
				MasterFrame->CRC);
		}
		crc = CRC16(iPacket, MasterFrame->PacketSize - 2);
		break;
	}
	return crc;
}

void CreateSlavePacket(tWSlaveFrame SlaveFrame, unsigned char *oPacket,
		       int MaxPacketSize, int *PacketSize)
{
	unsigned short crc;
	unsigned char *buffer;
	unsigned char HowBytes;
	unsigned char u;

	*PacketSize = 0;

	switch (SlaveFrame.FunctionId) {
	case MB_F_RHR:		/* Funkcja 3 (Odczyt bloku rejestrów) */
		HowBytes = (unsigned char)(2 * SlaveFrame.DataSize);
		*PacketSize = 1 + 1 + 1 + HowBytes + 2;
		buffer = (unsigned char *)malloc(*PacketSize);
		buffer[0] = SlaveFrame.DeviceId;
		buffer[1] = SlaveFrame.FunctionId;
		buffer[2] = HowBytes;
		for (u = 0; u < SlaveFrame.DataSize; u++) {
			buffer[3 + 2 * u] =
			    ((SlaveFrame.Body[u] & 0xff00) >> 8);
			buffer[4 + 2 * u] = SlaveFrame.Body[u] & 0xff;
		}
		crc = CRC16(buffer, *PacketSize - 2);
		buffer[*PacketSize - 2] = (crc & 0xff00) >> 8;
		buffer[*PacketSize - 1] = crc & 0xff;
		memcpy(oPacket, buffer, *PacketSize);
		free(buffer);
		break;

	case MB_F_RIR:		/* Funkcja 4 (Odczyt bloku wej¶æ) */
		HowBytes = (unsigned char)(2 * SlaveFrame.DataSize);
		*PacketSize = 1 + 1 + 1 + HowBytes + 2;
		buffer = (unsigned char *)malloc(*PacketSize);
		buffer[0] = SlaveFrame.DeviceId;
		buffer[1] = SlaveFrame.FunctionId;
		buffer[2] = HowBytes;
		for (u = 0; u < SlaveFrame.DataSize; u++) {
			buffer[3 + 2 * u] =
			    ((SlaveFrame.Body[u] & 0xff00) >> 8);
			buffer[4 + 2 * u] = SlaveFrame.Body[u] & 0xff;
		}
		crc = CRC16(buffer, *PacketSize - 2);
		buffer[*PacketSize - 2] = (crc & 0xff00) >> 8;
		buffer[*PacketSize - 1] = crc & 0xff;
		memcpy(oPacket, buffer, *PacketSize);
		free(buffer);
		break;

	case MB_F_WSC:		/* Funkcja 5 (Zapis pojedyñczej ceWy) */
		/*
		 * W funkcji 5 pakiet jest zawsze tej samej wielko¶ci 
		 */
		*PacketSize = 8;
		buffer = (unsigned char *)malloc(*PacketSize);
		buffer[0] = SlaveFrame.DeviceId;
		buffer[1] = SlaveFrame.FunctionId;
		buffer[2] = ((SlaveFrame.Address & 0xff00) >> 8);
		buffer[3] = SlaveFrame.Address & 0x00ff;
		buffer[4] = ((SlaveFrame.Body[0] & 0xff00) >> 8);
		buffer[5] = SlaveFrame.Body[0] & 0xff;
		crc = CRC16(buffer, *PacketSize - 2);
		buffer[6] = (crc & 0xff00) >> 8;
		buffer[7] = crc & 0xff;
		memcpy(oPacket, buffer, *PacketSize);
		free(buffer);
		break;

	case MB_F_WSR:		/* Funkcja 6 (Zapis pojedyñczego rehestru) */
		/*
		 * W funkcji 6 pakiet jest zawsze tej samej wielko¶ci 
		 */
		*PacketSize = 8;
		buffer = (unsigned char *)malloc(*PacketSize);
		buffer[0] = SlaveFrame.DeviceId;
		buffer[1] = SlaveFrame.FunctionId;
		buffer[2] = ((SlaveFrame.Address & 0xff00) >> 8);
		buffer[3] = SlaveFrame.Address & 0x00ff;
		buffer[4] = ((SlaveFrame.Body[0] & 0xff00) >> 8);
		buffer[5] = SlaveFrame.Body[0] & 0xff;
		crc = CRC16(buffer, *PacketSize - 2);
		buffer[6] = (crc & 0xff00) >> 8;
		buffer[7] = crc & 0xff;
		memcpy(oPacket, buffer, *PacketSize);
		free(buffer);
		break;

	case MB_F_WMR:		/* Funkcja 10 (Zapis do wielu rejestrów) */
		/*
		 * W funkcji 10 pakiet jest zawsze tej samej
		 * wielko¶ci 
		 */
		*PacketSize = 8;
		buffer = (unsigned char *)malloc(*PacketSize);
		buffer[0] = SlaveFrame.DeviceId;
		buffer[1] = SlaveFrame.FunctionId;
		buffer[2] = ((SlaveFrame.Address & 0xff00) >> 8);
		buffer[3] = SlaveFrame.Address & 0x00ff;
		buffer[4] = ((SlaveFrame.DataSize & 0xff00) >> 8);
		buffer[5] = SlaveFrame.DataSize & 0xff;
		crc = CRC16(buffer, *PacketSize - 2);
		buffer[6] = (crc & 0xff00) >> 8;
		buffer[7] = crc & 0xff;
		memcpy(oPacket, buffer, *PacketSize);
		free(buffer);
		break;
	}
	if (mbrtu_single == TRUE) {
		int debugi;

		fprintf(stderr, "\n");
		for (debugi = 0; debugi < *PacketSize; debugi++) {
			fprintf(stderr, "buffer[%d] = 0x%x\n", debugi,
				oPacket[debugi]);
		}
	}

}

void CreateErrorPacket(tWErrorFrame ErrorFrame, unsigned char *oPacket,
		       int MaxPacketSize, int *PacketSize)
{
	unsigned short crc;

	*PacketSize = 1 + 1 + 1 + 2;
	oPacket[0] = ErrorFrame.DeviceId;
	oPacket[1] = ErrorFrame.FunctionId | 0x80;
	oPacket[2] = ErrorFrame.Exception;
	crc = CRC16(oPacket, *PacketSize - 2);
	oPacket[3] = (crc & 0xff00) >> 8;
	oPacket[4] = crc & 0xff;
	if (mbrtu_single == TRUE) {
		int debugi;

		fprintf(stderr, "\n");
		for (debugi = 0; debugi < *PacketSize; debugi++) {
			fprintf(stderr, "buffer[%d] = 0x%x\n", debugi,
				oPacket[debugi]);
		}
	}
}

unsigned short DecodeSlavePacket(unsigned char *iPacket, int MaxPacketSize,
				 tRSlaveFrame * SlaveFrame)
{
	unsigned char u;
	unsigned short us;
	unsigned short crc = 0;

	/*
	 * Inicjalizacja parametrów 
	 */
	u = 0;
	SlaveFrame->Address = 0;
	SlaveFrame->DataSize = 0;
	for (us = 0; us < MAX_BODY_SIZE; us++)
		SlaveFrame->Body[us] = 0;
	SlaveFrame->CRC = 0;
	SlaveFrame->PacketSize = 0;
	/*
	 * Wspólne parametry 
	 */
	SlaveFrame->FunctionId = u;
	SlaveFrame->DeviceId = iPacket[0];
	SlaveFrame->FunctionId = iPacket[1];
	if (mbrtu_single == TRUE) {
		fprintf(stderr, "SlaveFrame->DeviceId = 0x%x\n",
			SlaveFrame->DeviceId);
		fprintf(stderr, "SlaveFrame->FunctionId = 0x%x\n",
			SlaveFrame->FunctionId);
	}
	switch (SlaveFrame->FunctionId) {
	case MB_F_RHR:		/* Funkcja 3 (Odczyt bloku rejestrów) */
		SlaveFrame->DataSize = iPacket[2] >> 1;
		if (mbrtu_single == TRUE) {
			fprintf(stderr,
				"SlaveFrame->DataSize = 0x%x\n",
				SlaveFrame->DataSize);
		}
		SlaveFrame->PacketSize =
		    1 + 1 + 1 + 2 * SlaveFrame->DataSize + 2;
		if (mbrtu_single == TRUE) {
			fprintf(stderr,
				"SlaveFrame->PacketSize = 0x%x\n",
				SlaveFrame->PacketSize);
		}
		for (u = 0; u < SlaveFrame->DataSize; u++) {
			SlaveFrame->Body[u] = 0;
			SlaveFrame->Body[u] =
			    iPacket[3 + 2 * u] << 8 | iPacket[4 + 2 * u];
			if (mbrtu_single == TRUE) {
				fprintf(stderr,
					"SlaveFrame->Body[%d] = 0x%x\n",
					u, SlaveFrame->Body[u]);
			}
		}
		SlaveFrame->CRC =
		    (iPacket[SlaveFrame->PacketSize - 2] << 8) |
		    (iPacket[SlaveFrame->PacketSize - 1]);
		if (mbrtu_single == TRUE) {
			fprintf(stderr, "SlaveFrame->CRC = 0x%x\n",
				SlaveFrame->CRC);
		}
		crc = CRC16(iPacket, SlaveFrame->PacketSize - 2);
		break;

	case MB_F_RIR:		/* Funkcja 4 (Odczyt bloku wej¶æ) */
		/*
		 * W funkcji 4 pakiet jest zawsze tej samej wielko¶ci 
		 */
		SlaveFrame->DataSize = iPacket[2] >> 1;
		if (mbrtu_single == TRUE) {
			fprintf(stderr,
				"SlaveFrame->DataSize = 0x%x\n",
				SlaveFrame->DataSize);
		}
		SlaveFrame->PacketSize =
		    1 + 1 + 1 + 2 * SlaveFrame->DataSize + 2;
		if (mbrtu_single == TRUE) {
			fprintf(stderr,
				"SlaveFrame->PacketSize = 0x%x\n",
				SlaveFrame->PacketSize);
		}
		for (u = 0; u < SlaveFrame->DataSize; u++) {
			SlaveFrame->Body[u] = 0;
			SlaveFrame->Body[u] =
			    iPacket[3 + 2 * u] << 8 | iPacket[4 + 2 * u];
			if (mbrtu_single == TRUE) {
				fprintf(stderr,
					"SlaveFrame->Body[%d] = 0x%x\n",
					u, SlaveFrame->Body[u]);
			}
		}
		SlaveFrame->CRC = 0;
		SlaveFrame->CRC =
		    (iPacket[SlaveFrame->PacketSize - 2] << 8) |
		    (iPacket[SlaveFrame->PacketSize - 1]);
		if (mbrtu_single == TRUE) {
			fprintf(stderr, "SlaveFrame->CRC = 0x%x\n",
				SlaveFrame->CRC);
		}
		crc = CRC16(iPacket, SlaveFrame->PacketSize - 2);
		break;

	case MB_F_WSC:		/* Funkcja 5 (Zapis pojedyñczej cewy) */
		/*
		 * W funkcji 5 pakiet jest zawsze tej samej wielko¶ci 
		 */
		SlaveFrame->PacketSize = 8;
		if (mbrtu_single == TRUE) {
			fprintf(stderr,
				"SlaveFrame->PacketSize = %d\n",
				SlaveFrame->PacketSize);
		}
		SlaveFrame->Address = 0;
		SlaveFrame->Address = (iPacket[2] << 8) | (iPacket[2]);
		if (mbrtu_single == TRUE) {
			fprintf(stderr, "SlaveFrame->Address = %d\n",
				SlaveFrame->Address);
		}
		SlaveFrame->Body[0] = 0;
		SlaveFrame->Body[0] = (iPacket[4] << 8) | (iPacket[5]);
		if (mbrtu_single == TRUE) {
			fprintf(stderr,
				"SlaveFrame->Body[0] = 0x%x\n",
				SlaveFrame->Body[0]);
		}
		SlaveFrame->CRC = 0;
		SlaveFrame->CRC = (iPacket[6] << 8) | (iPacket[7]);
		if (mbrtu_single == TRUE) {
			fprintf(stderr, "SlaveFrame->CRC = 0x%x\n",
				SlaveFrame->CRC);
		}
		crc = CRC16(iPacket, SlaveFrame->PacketSize - 2);
		break;

	case MB_F_WSR:		/* Funkcja 6 (Zapis pojedyñczego rehestru) */
		/*
		 * W funkcji 6 pakiet jest zawsze tej samej wielko¶ci 
		 */
		SlaveFrame->PacketSize = 8;
		if (mbrtu_single == TRUE) {
			fprintf(stderr,
				"SlaveFrame->PacketSize = %d\n",
				SlaveFrame->PacketSize);
		}
		SlaveFrame->Address = 0;
		SlaveFrame->Address = (iPacket[2] << 8) | (iPacket[2]);
		if (mbrtu_single == TRUE) {
			fprintf(stderr, "SlaveFrame->Address = %d\n",
				SlaveFrame->Address);
		}
		SlaveFrame->Body[0] = 0;
		SlaveFrame->Body[0] = (iPacket[4] << 8) | (iPacket[5]);
		if (mbrtu_single == TRUE) {
			fprintf(stderr,
				"SlaveFrame->Body[0] = 0x%x\n",
				SlaveFrame->Body[0]);
		}
		SlaveFrame->CRC = 0;
		SlaveFrame->CRC = (iPacket[6] << 8) | (iPacket[7]);
		if (mbrtu_single == TRUE) {
			fprintf(stderr, "SlaveFrame->CRC = 0x%x\n",
				SlaveFrame->CRC);
		}
		crc = CRC16(iPacket, SlaveFrame->PacketSize - 2);
		break;

	case MB_F_WMR:		/* Funkcja 10 (Zapis do wielu rejestrów) */
		SlaveFrame->PacketSize = 8;
		if (mbrtu_single == TRUE) {
			fprintf(stderr,
				"SlaveFrame->PacketSize = %d\n",
				SlaveFrame->PacketSize);
		}
		SlaveFrame->Address = 0;
		SlaveFrame->Address = (iPacket[2] << 8) | (iPacket[2]);
		if (mbrtu_single == TRUE) {
			fprintf(stderr, "SlaveFrame->Address = %d\n",
				SlaveFrame->Address);
		}
		SlaveFrame->DataSize = 0;
		SlaveFrame->DataSize = (iPacket[4] << 8) | (iPacket[5]);
		if (mbrtu_single == TRUE) {
			fprintf(stderr,
				"SlaveFrame->DataSize] = 0x%x\n",
				SlaveFrame->DataSize);
		}
		SlaveFrame->CRC = 0;
		SlaveFrame->CRC = (iPacket[6] << 8) | (iPacket[7]);
		if (mbrtu_single == TRUE) {
			fprintf(stderr, "SlaveFrame->CRC = 0x%x\n",
				SlaveFrame->CRC);
		}
		crc = CRC16(iPacket, SlaveFrame->PacketSize - 2);
		break;
	default:
		SlaveFrame->FunctionId = 0;
		break;
	}
	return crc;
}

int ReceiveSlavePacket(int CommId, tRSlaveFrame * oSlaveFrame,
		       char *CRCStatus, int ReceiveTimeout,
		       int DelayBetweenChars)
{
	unsigned char buffer[MAX_BUFFER_SIZE];
	int i;
	int status = 0;
	unsigned short crc = 0;

	fd_set rfds;
	struct timeval tv;
	int retval;
	FD_ZERO(&rfds);
	FD_SET(CommId, &rfds);
	tv.tv_sec = ReceiveTimeout;	/* Oczekiwanie 10s */
	tv.tv_usec = 0;
	retval = select(CommId + 1, &rfds, NULL, NULL, &tv);
	if (retval == -1) {
		return GENERAL_ERROR;
	} else if (retval) {
		for (i = 0; read(CommId, &buffer[i], 1) == 1; i++) {
			status++;
			if (i >= MAX_BUFFER_SIZE)
				break;	/* Zabezpieczenie przed wyciekiem pamieci */
			usleep(1000 * DelayBetweenChars);
		}

		crc = DecodeSlavePacket(buffer, MAX_BUFFER_SIZE, oSlaveFrame);
		if (crc == oSlaveFrame->CRC) {
			if (CRCStatus != NULL) {
				*CRCStatus = CRC_OK;
			}
		} else {
			if (CRCStatus != NULL) {
				*CRCStatus = CRC_ERROR;
			}
		}
		return status;
	}
	return TIMEOUT_ERROR;
}

int SendMasterPacket(int CommId, tWMasterFrame iMasterFrame)
{
	unsigned char buffer[MAX_BUFFER_SIZE];
	int PacketSize;

	if (mbrtu_single) {
		fprintf(stderr, "Sending master packet :\n");
	}
	CreateMasterPacket(iMasterFrame, buffer, MAX_BUFFER_SIZE, &PacketSize);
	return (write(CommId, buffer, PacketSize));
}

/*
 * Procedury dla slave 
 */
int SendSlavePacket(int CommId, tWSlaveFrame iSlaveFrame)
{
	unsigned char buffer[MAX_BUFFER_SIZE];
	int PacketSize;

	if (mbrtu_single) {
		fprintf(stderr, "Sending slave packet :\n");
	}
	CreateSlavePacket(iSlaveFrame, buffer, MAX_BUFFER_SIZE, &PacketSize);
//      int i;
//      for (i=0;i<PacketSize;i++)
//      fprintf(stderr," buffer[%d]=%d\n",i,buffer[i]);
	return (write(CommId, buffer, PacketSize));
}

int ReceiveMasterPacket(int CommId, tRMasterFrame * oMasterFrame,
			char *CRCStatus, int ReceiveTimeout,
			int DelayBetweenChars)
{
	unsigned char buffer[MAX_BUFFER_SIZE];
	int i;
	int status = 0;
	unsigned short crc = 0;

	fd_set rfds;
	struct timeval tv;
	int retval;
	FD_ZERO(&rfds);
	FD_SET(CommId, &rfds);
	tv.tv_sec = ReceiveTimeout;	/* Oczekiwanie 10s */
	tv.tv_usec = 0;
	retval = select(CommId + 1, &rfds, NULL, NULL, &tv);
	if (retval == -1) {
		return GENERAL_ERROR;
	} else if (retval) {
		for (i = 0; read(CommId, &buffer[i], 1) == 1; i++) {
			status++;
			if (i >= MAX_BUFFER_SIZE)
				break;	/* Zabezpieczenie przed wyciekiem pamieci */
			usleep(1000 * DelayBetweenChars);
		}

		crc = DecodeMasterPacket(buffer, MAX_BUFFER_SIZE, oMasterFrame);
		if (crc == oMasterFrame->CRC) {
			if (CRCStatus != NULL) {
				*CRCStatus = CRC_OK;
			}
		} else {
			if (CRCStatus != NULL) {
				*CRCStatus = CRC_ERROR;
			}
		}
		return status;
	}
	return TIMEOUT_ERROR;
}

void SendErrorPacket(int CommId, tWErrorFrame iErrorFrame)
{
	unsigned char buffer[MAX_BUFFER_SIZE];
	int PacketSize;

	if (mbrtu_single) {
		fprintf(stderr, "Sending Error packet :\n");
	}
	CreateErrorPacket(iErrorFrame, buffer, MAX_BUFFER_SIZE, &PacketSize);
	write(CommId, buffer, PacketSize);
}

void CloseComm(int CommId)
{
	close(CommId);
}
#endif
