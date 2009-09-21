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

  Pawel Palucha <pawel@praterm.com.pl>
  $Id$

  Daemon for Aquametro CALEC MCP-300 heatmeter.
  Serial port settings: 9600 8N1
  Wiring: RX, TX, GND

  Protocol:

  Q: AM
  R: OK
  Q: R4
  R: ACK
  Q: <AD1,AD2> (register address in MCP format, see below)
  R: <data> (4 bytes)
  R: <checksum> (1 byte - lower byte of arithmetic sum of previous 6 bytes, starting from AD1)

  MCP address format, R is register address:
  AD2 = R & 0xFF
  AD1 = ((R >> 8) & 0x07) | ((R & 0x70) << 1)

  Data may be:

  fixedpoint: unsigned integer, 7 digits
  coding: 1111 bbbb  bbbb bbbb  bbbb bbbb 0000 dddd
  'b' bits code 6 last digits, 'd' bits code first digit, so if E is 4 bytes encoded
  representation, value is:
  val = 1000000 * (E & 0x0F) + (E >> 8) & 0xFFFFF

  floating point: 
  coding: s1mm mmmm  mmmm mmmm  mmmm mmmm  eeee eeee
  s is sign, 0 - positive, 1 - negative
  mantisa M = 0.1mmmmmmmmmmmmmmmmmmmmmmmmm
  E = eeeeeeee : -127 << 127 (signed 7 bytes integer)
  value = M*2^E
  If value is negative, 3 first bytes are inverted (bit by bit).

  Available registers (value, address, number of bytes, coding, unit):

  1	Energy meter		0x2000	4	fixedpoint	10 * MWh
  2	Volume meter		0x2040	4	fixedpoint	10 * t
  3	Output temperature	0x2080	4	floatingpoint	C degree
  4	Return temperature	0x2084	4	floatingpoint	C degree
  5	Temperature difference	0x2088	4	floatingpoint	C degree
  6	Flow			0x2098	4	floatingpoint	kg/h
  7	Power			0x209c	4	floatingpoint	kW
  8	Serial number		0x0100	16	ASCII
  9	Number of impulses	0x018c	4	floatingpoint	-

  Daemon output 16 values:

  1. Energy meter, most significant word, MWh, precision 1
  2. Energy meter, less significant word
  3. Volume meter, most significant word, t, precision 1
  4. Volume meter, less significant word
  5. Output temperature, precision 2
  6. Return temperature, precision 2
  7. Flow, m3/h, precision 2
  8. Power, kW, precision 1
*/


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <assert.h>
#include <signal.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <termio.h>
#include <fcntl.h>
#include <netinet/in.h>

#include "ipchandler.h"
#include "liblog.h"

#include "conversion.h"

#include <string>
using std::string;

#define DAEMON_INTERVAL 10
#define	DAEMON_TIMEOUT	5
#define CALEC_PARAMS_COUNT	8
#define CALEC_REGISTERS_COUNT	6

/**
 * Modbus communication config.
 */
class CalecDaemon {
public :
	/**
	 * @param params number of params to read
	 */
	CalecDaemon();

	/** 
	 * Parses XML 'device' element.
	 * @return - 0 on success, 1 on error 
	 */
	int ParseConfig(DaemonConfig * cfg);

	/** Helper function for reading from port.
	 * @param buffer output buffer
	 * @param count number of bytes to read
	 * @return count on success, 0 on timeout, -1 on error
	 */
	int Read(unsigned char* buffer, int count);
	/**
	 * Read data.
	 * @param ipc IPCHandler
	 * @return 0 on success, 1 on error
	 */
	int ReadData(IPCHandler *ipc);

	/** Wait for next cycle. */
	void Wait();
	/** Close serial port */
	void ClosePort();
	
protected :
	/** Try to open serial port */
	void OpenPort();
	char* m_port;		/**< path to serial port */
	int m_fd;		/**< serial port file descriptor */
	int m_single;		/**< are we in 'single' mode */
	public:
	int m_params_count;	/**< size of params array */
	protected:
	time_t m_last;
	/* Read single register.
	 * @param address register address
	 * @param size number of output parameters (1 or 2)
	 * @param fixed register format, 1 for fixed point, 0 for floating point
	 * @param precision value multiplier
	 * @param output output buffer
	 * @return 0 when we should continue with reading next registers, 1 if there was
	 * a serious error and we should close port
	 */
	int ReadRegister(int address, int size, int fixed, int precision, short* output);
	/** Convert address to MCP address format.
	 * @param address register address to convert
	 * @param buf 2 bytes buffer for conversion result
	 */
	void MCPAddress(int address, unsigned char buf[2]);
	/** Decode MCP representation of fixed-point value.
	 * @param buf 4 bytes returned by heatmeter
	 * @param err pointer to error code, set to 1 on error, 0 otherwise
	 * @return result value, unspecified on error
	 */
	unsigned long DecodeFixed(unsigned char buf[4], int* err);
	/** Decode MCP representation of floating-point value.
	 * @see DecodeFixed
	 */
	double DecodeFloating(unsigned char buf[4], int* err);
};

/**
 * @param params number of params to read
 * @param sends number of params to send (write)
 */
CalecDaemon::CalecDaemon() 
{
	m_params_count = CALEC_PARAMS_COUNT;
	m_port = NULL;
	m_single = 0;
	m_last = 0;
	m_fd = -1;
}



int CalecDaemon::ParseConfig(DaemonConfig * cfg)
{
	int dev_num;
	
	/* get config data */
	assert (cfg != NULL);
	dev_num = cfg->GetLineNumber();
	assert (dev_num > 0);

	m_single = cfg->GetSingle();
	
	std::wstring tmp = cfg->GetDevice()->GetPath();
	m_port = strdup(SC::S2A(tmp).c_str());
	if (m_single) {
		printf("Using port '%s'\n", m_port);
	}

	return 0;
}


void CalecDaemon::Wait() 
{
	time_t t;
	t = time(NULL);
	
	
	if (t - m_last < DAEMON_INTERVAL) {
		int i = DAEMON_INTERVAL - (t - m_last);
		if (m_single) {
			printf("Waiting %d seconds\n", i);
		}
		while (i > 0) {
			i = sleep(i);
		}
	}
	m_last = time(NULL);
	return;
}

void CalecDaemon::OpenPort()
{
	m_fd = open(m_port, O_RDWR | O_NOCTTY | O_NONBLOCK, 0);
	if (m_fd == -1) {
		sz_log(1, "calecdmn: cannot open device %s, errno:%d (%s)", m_port,
				errno, strerror(errno));
		return;
	}
	struct termios ti;
	if (tcgetattr(m_fd, &ti) == -1) {
		sz_log(1, "calecdmn: cannot retrieve port settings, errno:%d (%s)",
				errno, strerror(errno));
		ClosePort();
		return;
	}
	ti.c_oflag = ti.c_iflag = ti.c_lflag = 0;
	ti.c_cflag = B9600 | CS8 | CREAD | CLOCAL ;
	if (tcsetattr(m_fd, TCSANOW, &ti) == -1) {
		sz_log(1,"calecdmn: cannot set port settings, errno: %d (%s)",
				errno, strerror(errno));
		ClosePort();
	}
}

void CalecDaemon::ClosePort()
{
	if (m_fd == -1) return;
	close(m_fd);
	m_fd = -1;
}

int CalecDaemon::Read(unsigned char* buffer, int count)
{
	int to_read = count;
	fd_set rfds;
	struct timeval tv;
	FD_ZERO(&rfds);
	FD_SET(m_fd, &rfds);
	tv.tv_sec = DAEMON_TIMEOUT;
	tv.tv_usec = 0;

	while (to_read > 0) {
		int retval = select(m_fd + 1, &rfds, NULL, NULL, &tv);
		if (retval == -1) {
			sz_log(0, "select() error on descriptor");
			return -1;
		}
		if (retval == 0) { /* timeout */
			if (m_single) {
				printf("timeout after receiving %d bytes\n", count - to_read);
			}
			return 0;
		}
		retval = read(m_fd, buffer + (count - to_read), to_read);
		if (retval == -1) {
			if (m_single) {
				printf("read() failed, errno is %d (%s)\n",
						errno, strerror(errno));
			}
			return -1;
		}
		if (m_single) {
			for (int i = 0; i < retval; i++) {
				printf("got: 0x%x\n", *(buffer + count - to_read + i));
			}
		}
		to_read -= retval;
	}
	return count;
}
	
void CalecDaemon::MCPAddress(int address, unsigned char buf[2])
{
	address = htons(address);
	memcpy(buf, &address, 2);
	buf[0] = (buf[0] & 0x07) | ((buf[0] & 0x70) << 1);
}
	
unsigned long CalecDaemon::DecodeFixed(unsigned char buf[4], int* err)
{
	*err = 1;
	/*

	{ buf[0]}  { buf[1]}  { buf[2]}  { buf[3]}
	1111 bbbb  bbbb bbbb  bbbb bbbb  0000 dddd
	     { 6 digits: 0 - 999,999  }       { first digit 
	                                         }
	  */
	if ((buf[0] & 0xF0) != 0xF0) {
		if (m_single) {
			printf("First 4 bits of fixed point value not set!\n");
		}
		return 0;
	}
	if ((buf[3] & 0xF0) != 0x00) {
		if (m_single) {
			printf("Bits 25 - 28 of fixed point value not set to 0!\n");
		}
		return 0;
	}
	*err = 0;
	unsigned long val = ((((unsigned long)buf[3]) & 0x0F) * 1000000)
		+ (unsigned long)buf[2]
		+ (((unsigned long)buf[1]) << 8)
		+ (((unsigned long)(buf[0] & 0x0F)) << 16);
	if (m_single) {
		printf("Got fixed point value: %0ld\n", val);
	}
	return val;
}
	
double CalecDaemon::DecodeFloating(unsigned char buf[4], int* err)
{
	*err = 0;
	double val;

	/*

	{ buf[0]}  { buf[1]}  { buf[2]}  { buf[3]}
	s1mm mmmm  mmmm mmmm  mmmm mmmm  eeee eeee

	s = sign, 0 - positive, 1 - negative, if 1, 3 first bytes are bit-inverted
	M (mantisa) = 0.1mmmmmmmmmmmmmmmmmmmmmm (binary)
	E (exponent) = eeeeeeee (signed, -127 - 128)

	val = s * M * 2^E

	  */

	double sign = 1;
	if (buf[0] & 0x80) {
		/* negative value, we have to inverse 3 first bytes */
		sign = -1;
		buf[0] = ~buf[0];
		buf[1] = ~buf[1];
		buf[2] = ~buf[2];
	}
	if ((buf[0] | buf[1] | buf[2] | buf[3]) == 0x00) {
		val = 0.0;
	} else if ((buf[0] & 0x40) == 0) {
		*err = 1;
		if (m_single) {
			printf("Second bit of floating point value should be 1!");
		}
		return 0;
	} else {
		/* treat mantisa as integer ... */
		unsigned long mant = 
			(unsigned long)buf[2] 
			+ (((unsigned long)buf[1]) << 8) 
			+ ((unsigned long)(buf[0] & 0x7F) << 16);
		/* so we need to shift result by 23 bits */
		long exp = 
			(long)((signed char)buf[3]) - 23; 
		val = (double)mant * pow(2.0, (double)exp) * sign;
	}
	if (m_single) {
		printf("Got floating point value: %g\n", val);
	}
	return val;
}


int CalecDaemon::ReadRegister(int address, int size, int fixed, int precision, short* output)
{
	unsigned char buf[16];

	if (m_fd == -1) {
		OpenPort();
	}
	if (m_fd == -1) {
		return 1;
	}

	int ret;

	if (m_single) {
		printf("\nSending 'AM'\n");
	}
	ret = write(m_fd, "AM", 2);
	if (ret != 2) {
		if (m_single) {
			printf("Failed, return is %d, errno is %d\n", ret, errno);
		}
		ClosePort();
		return 1;
	}

	if (m_single) {
		printf("Waiting for 'OK'\n");
	}
	ret = Read(buf, 2);
	if (ret != 2) {
		if (m_single) {
			if (ret == 0) {
				printf("Select() timeout\n");
			} else {
				printf("select()/read() error\n");
			}
		}
		return 1;
	} else if (m_single) {
		printf("Got 'OK'\n");
	}

	if (m_single) {
		printf("Sending 'R 0x4'\n");
	}
	ret = write(m_fd, "R\004", 2);
	if (ret != 2) {
		if (m_single) {
			printf("Failed, return is %d, errno is %d\n", ret, errno);
		}
		ClosePort();
		return 1;
	}

	if (m_single) {
		printf("Waiting for 'ACK'\n");
	}
	ret = Read(buf, 1);
	if (ret != 1) {
		if (m_single) {
			if (ret == 0) {
				printf("Select() timeout\n");
			} else {
				printf("select()/read() error\n");
			}
		}
		return 1;
	}
	if (buf[0] != 0x06) {
		if (m_single) {
			printf("Expected 0x06, got 0x%x\n", buf[0]);
		}
		return 1;
	}
	if (m_single) {
		printf("Got 'ACK'\n");
	}

	MCPAddress(address, buf);

	int checksum = buf[0] + buf[1];

	if (m_single) {
		printf("Asking for register 0x%02x, sending adress <0x%02x, 0x%02x>\n",
				address, buf[0], buf[1]);
	}
	ret = write(m_fd, buf, 2);
	if (ret != 2) {
		if (m_single) {
			printf("Failed, return is %d, errno is %d\n", ret, errno);
		}
		ClosePort();
		return 1;
	}
	ret = Read(buf, 5);
	if (ret != 5) {
		if (m_single) {
			if (ret == 0) {
				printf("Select() timeout\n");
			} else {
				printf("select()/read() error\n");
			}
		}
		return 1;
	}
	if (m_single) {
		printf("Got 0x%02x 0x%02x 0x%02x 0x%02x\n", buf[0], buf[1], buf[2], buf[3]);
	}
	for (int j = 0; j < 4; j++) {
		checksum += buf[j];
	}
	checksum &= 0xFF;
	if (m_single) {
		printf("Got checksum 0x%02x, calculated is 0x%02x\n", buf[4], checksum);
	}
	if (checksum != (int)buf[4]) {
		return 1;
	}
	
	long val;
	if (fixed) {
		int err;
		val = DecodeFixed(buf, &err);
		if (err != 0) {
			sz_log(1, "Incorrect fixed point value format for register 0x%02x", address);
		}
		val *= precision;
	} else {
		int err;
		double fval = DecodeFloating(buf, &err);
		if (err != 0) {
			sz_log(1, "Incorrect floating point value format for register 0x%02x", address);
		}
		fval *= precision;
		val = (double)fval;
	}
	if (size == 2) {
		output[0] = (short)((val >> 16) & 0xFFFF);
		output[1] = (short)(val & 0xFFFF);
	} else {
		output[0] = (short)val;
	}
	return 0;
}

int CalecDaemon::ReadData(IPCHandler *ipc)
{
	int registers[CALEC_REGISTERS_COUNT] = { 0x2000, 0x2040, 0x2080, 0x2084, 0x2098, 0x209c };
	int sizes[CALEC_REGISTERS_COUNT] = { 2, 2, 1, 1, 1, 1 };
	int fixed[CALEC_REGISTERS_COUNT] = { 1, 1, 0, 0, 0, 0 };
	int precisions[CALEC_REGISTERS_COUNT] = { 1, 1, 100, 100, 10, 10 };


	for (int i = 0; i < m_params_count; i++) {
		ipc->m_read[i] = SZARP_NO_DATA;
	}


	int param_num = 0;
	for (int i = 0; i < CALEC_REGISTERS_COUNT; i++) {
		if (ReadRegister(registers[i], sizes[i], fixed[i], precisions[i], 
					ipc->m_read + param_num) != 0) {
			return 1;
		}
		param_num += sizes[i];
	}

	return 0;
}

RETSIGTYPE terminate_handler(int signum)
{
	sz_log(2, "signal %d cought, exiting", signum);
	exit(1);
}

void init_signals()
{
	int ret;
	struct sigaction sa;
	sigset_t block_mask;

	sigfillset(&block_mask);
	sigdelset(&block_mask, SIGKILL);
	sigdelset(&block_mask, SIGSTOP);
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = terminate_handler;
	ret = sigaction(SIGTERM, &sa, NULL);
	assert (ret == 0);
	ret = sigaction(SIGINT, &sa, NULL);
	assert (ret == 0);
	ret = sigaction(SIGHUP, &sa, NULL);
	assert (ret == 0);
}

int main(int argc, char *argv[])
{
	DaemonConfig   *cfg;
	IPCHandler     *ipc;
	CalecDaemon *dmn;

	xmlInitParser();
	LIBXML_TEST_VERSION
	xmlLineNumbersDefault(1);

	cfg = new DaemonConfig("calecdmn");
	assert (cfg != NULL);
	
	if (cfg->Load(&argc, argv))
		return 1;
	
	dmn = new CalecDaemon();
	assert (dmn != NULL);
	
	if (dmn->ParseConfig(cfg)) {
		return 1;
	}

	ipc = new IPCHandler(cfg);
	if (ipc->m_params_count != CALEC_PARAMS_COUNT) {
		if (cfg->GetSingle()) {
			printf("Params count %d != %d, exiting\n", ipc->m_params_count, 
					CALEC_PARAMS_COUNT);
		}
		sz_log(0, "params count should be %d, found %d params", CALEC_PARAMS_COUNT,
				ipc->m_params_count);
		return 1;
	}
	if (!cfg->GetSingle()) {
		if (ipc->Init())
			return 1;
	}

	init_signals();

	sz_log(2, "Starting CalecDaemon");

	while (true) {
		dmn->Wait();
		if (dmn->ReadData(ipc)) {
			dmn->ClosePort();
		}
		/* send data from parcook segment */
		ipc->GoParcook();
	}
	return 0;
}

