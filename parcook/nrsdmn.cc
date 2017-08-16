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
 * RSDMN 1.5 - new generation of SZARP RS deamon.
 * <reksio@praterm.com.pl>
 * Significant part of the code through courtesy of tcpdmn.cc
 * and rsdmn.
 * $Id$
 */

/*
 SZARP daemon description block.

 @descripion_start

 @class 4

 @devices Z-Elektronik PLC, Sterkom SK-2000 PLC, Sterkom SK-4000 PLC.

 @protocol Z-Elektronik.

 @config For usage instruction use '--help' option. For configuration details
 see dmncfg.h file; one extra attribute is provided by this daemon -
 extra:speed attribute for unit element sets special serial port
 communication speed for given unit. In the following example, default speed is set to 19200 bps,
 but speed for unit 2 is 9600 bps.

 @config_example
 <device ... speed="19200">
 <unit id="1" .../>
 <unit id="2" ... xmlns:extra="http://www.praterm.com.pl/SZARP/ipk-extra"
 	extra:speed=9600" ... />
 ...

 @description_end

*/

#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <deque>

using std::deque;

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "conversion.h"
#include "dmncfg.h"
#include "ipchandler.h"
#include "liblog.h"
#include "tokens.h"
#include "xmlutils.h"
#include "custom_assert.h"

/* number of pseudo-param carrying sender data checksum
 * DO NOT EVER CHANGE THIS VALUE without changing platform */
#define SENDER_CHECKSUM_PARAM 299

/* if this flag is defined demon writes one byte at a time
 * in SerailPort::Write command to a driver
 * (this prevents pl2303 converter from stalling)
 * NOTE: this is deprecated, new driver (starting from kernel version
 * 2.6.14) does not need this.
 */
//#define PL2303_WORKAROUND
//
volatile sig_atomic_t stop_daemon = 0;
volatile sig_atomic_t psetd_pid = -1;

void sig_usr_handler(int signo, siginfo_t * sig_info, void *ctx)
{
	stop_daemon = !stop_daemon;
	psetd_pid = sig_info->si_pid;
}

/**Exception thrown when problem with communitaction thorugh
 * @see SerialPort occurrs*/
class SerialException : public std::exception
{};

/**Class handling comunication with serial port.*/
class SerialPort {
public:
	/**Constructor
	 * @param path path to a serial port device
	 * @param speed connection speed
	 * @param stopbits number of stop bits
	 * @param timeout specifies number of seconds @see GetData should wait for data
	 * to arrive on a port*/
	SerialPort(const char* path, int speed, int stopbits, int timeout) :
		m_fd(-1), m_path(path), m_speed(speed), m_curspeed(speed),
		m_stopbits(stopbits),
		m_timeout(timeout)
	{};
	/**Closes the port*/
	void Close();
	/**Opens the port throws @see SerialException
	* if it was unable to open a port or set connection
	* parameters. Attempts to open a port only if it
	* is not already opened or is opened with another speed
	* @param force_speed speed for port, 0 for default device speed
	*/
	void Open(int force_speed = 0);
	/*Reads data from a port to a given buffer,
	 * null terminates read data. Throws @see SerialException
	 * if read failed, or timeout occurred closes port in both cases.
	 * @param buffer buffer where data shall be placed
	 * @param size size of a buffer
	 * @return number of read bytes (not including terminating NULL)*/
	ssize_t GetData(char* buffer, size_t size);

	/*Writes contents of a supplied buffer to a port.
	 * Throws @see SerialException and closes the port
	 * if write operation failes.
	 * @param buffer buffer containing data that should be written
	 * @param size size of the buffer*/
	void WriteData(const char* buffer, size_t size);
private:
	/**Waits for data arrival on a port, throws SerialException
	 * if no data arrived withing specified time interval
	 * or select call failed. In both cases port is closed.
	 * @param timeout number of seconds the method should wait*/
	bool Wait(int timeout);

	int m_fd; 		/**<device descriptor, -1 indicates that the device is closed*/
	const char* m_path;	/**<path to the device*/
	int m_speed;		/**<configured connection speed (9600 def)*/
	int m_curspeed;		/**<current connection speed */
	int m_stopbits;		/**<number of stop bits (1 default)*/
	int m_timeout;		/**<timeout of read operation*/
};

/**Generates queries and parses repsonses from units*/
class UnitExaminator {
public:
	UnitExaminator(bool debug, bool dump_hex) :
		m_debug(debug), m_dump_hex(dump_hex),
		m_extraspeed(0)
	{}
	/**
	 * Checks unit configuration for extra:speed attribute.
	 * @param cfg deamon configration object
	 * @param unit_num unit index in device (starting from 0)
	 */
	void ConfigExtraSpeed(DaemonConfig* cfg, int unit_num);
	/**
	 * @return per-unit configured connection speed, 0 for default
	 * device speed */
	int GetExtraSpeed() { return m_extraspeed; }
	/**Method responsibe for generating queries for unit
	 * @param query output paramer - reference to a buffer containg query
	 * 	shall be released by a caller
	 * @param size size of a buffer*/
	virtual void GenerateQuery(char *&query, size_t& size) = 0;
	/**Parses response from unit*/
	virtual void SetData(char *response, size_t count) = 0;
	/**Metod called when communication error ocurrs*/
	virtual void SetNoData() = 0;
protected:
	/**represents response from unit*/
	struct Response {
		size_t m_params_count;	/**<number of parsed params*/
		short *m_params;	/**<table of params*/
		char m_id;		/**<unit id*/
		Response() : m_params_count(0), m_params(NULL), m_id(-1) {};
	};
	/**Parses response
	 * @param response string representing response
	 * @param count size of response in bytes
	 * @param resp output param result of parsing
	 * @return true if string was successfully parsed*/
	virtual bool ParseResponse(char* response,
			size_t count, Response* resp);
	/**Prints contents of response to stdout*/
	virtual void PrintResponse(const Response &resp);
	/*Method prints to stdout contents of a buffer in
	 * hex terminal format.
	 * @param c pointer to a buffer to be printed
	 * @size size of a buffer*/
	void DrawHex(const char *c, size_t size);
	virtual ~UnitExaminator() {};

	bool m_debug;		/**<true if in debug mode*/
	bool m_dump_hex;	/**<true if data exchanged between unit and
				daemon should be print in hex terminal format*/
	int m_extraspeed;	/**<non-default connection speed for
				unit, set by extra:speed attribute,
				0 means default (device) value */
};

/**Sniffer - does not generate any queries just dumps recived data to stdout*/
class Sniffer : public UnitExaminator {
public:
	Sniffer(bool debug, bool dump_hex) : UnitExaminator(debug, dump_hex) {}
	virtual void GenerateQuery(char *&query, size_t& size);
	virtual void SetData(char *response, size_t count);
	virtual void SetNoData() { }
};

/**Scanner - scans for units within provided ids' range*/
class Scanner : public UnitExaminator {
public:
	Scanner(char id1, char id2, bool debug, bool dump_hex) :
		UnitExaminator(debug,dump_hex),
		m_id1(id1), m_id2(id2), m_id(id1)
	{}
	virtual void GenerateQuery(char *&query, size_t& size);
	virtual void SetData(char *response, size_t count);
	virtual void SetNoData() { printf("No response\n"); }
private:
	char m_id1, m_id2, m_id;
};


void SerialPort::Close()
{
	if (m_fd < 0)
		return;
	close(m_fd);
	m_fd = -1;
}

void SerialPort::Open(int force_speed)
{
	if (force_speed == 0) {
		force_speed = m_speed;
	}
	if ((m_fd >= 0) && (force_speed == m_curspeed)) {
		return;
	}
	if (force_speed != m_curspeed) {
		if (m_fd >= 0) {
			Close();
		}
		m_curspeed = force_speed;
	}

	m_fd = open(m_path, O_RDWR | O_NOCTTY | O_NONBLOCK, 0);

	if (m_fd == -1) {
		sz_log(1, "nrsdmn: cannot open device %s, errno:%d (%s)", m_path,
			errno, strerror(errno));
		throw SerialException();
	}

	struct termios ti;
	if (tcgetattr(m_fd, &ti) == -1) {
		sz_log(1, "nrsdmn: cannot retrieve port settings, errno:%d (%s)",
				errno, strerror(errno));
		Close();
		throw SerialException();
	}

	sz_log(8, "setting port speed to %d", m_curspeed);
	switch (m_curspeed) {
		case 300:
			ti.c_cflag = B300;
			break;
		case 600:
			ti.c_cflag = B600;
			break;
		case 1200:
			ti.c_cflag = B1200;
			break;
		case 2400:
			ti.c_cflag = B2400;
			break;
		case 4800:
			ti.c_cflag = B4800;
			break;
		case 9600:
			ti.c_cflag = B9600;
			break;
		case 19200:
			ti.c_cflag = B19200;
			break;
		case 38400:
			ti.c_cflag = B38400;
			break;
		default:
			ti.c_cflag = B9600;
	}

	if (m_stopbits == 2)
		ti.c_cflag |= CSTOPB;

	ti.c_oflag =
	ti.c_iflag =
	ti.c_lflag = 0;

	ti.c_cflag |= CS8 | CREAD | CLOCAL ;

	if (tcsetattr(m_fd, TCSANOW, &ti) == -1) {
		sz_log(1,"Cannot set port settings, errno: %d (%s)",
			errno, strerror(errno));
		Close();
		throw SerialException();
	}
}

bool SerialPort::Wait(int timeout)
{
	int ret;
	struct timeval tv;
	fd_set set;
	time_t t1, t2;

	time(&t1);

	sz_log(8, "Waiting for data");
	while (true) {
		time(&t2);
		tv.tv_sec = timeout - (t2 - t1);
		if (tv.tv_sec < 0) {
			sz_log(2, "I cannot wait for negative number of seconds");
			return false;
		}
		tv.tv_usec = 0;
		FD_ZERO(&set);
		FD_SET(m_fd, &set);

		ret = select(m_fd + 1, &set, NULL, NULL, &tv);
		if (ret < 0) {
			if (errno == EINTR) {
				continue;
			} else {
				Close();
				sz_log(1, "select failed, errno %d (%s)",
					errno, strerror(errno));
				throw SerialException();
			}
		} else if (ret == 0) {
			sz_log(4, "No data arrived within specified interval");
			return false;
		} else {
			sz_log(8, "Data arrived");
			return true ;
		}
	}
}

ssize_t SerialPort::GetData(char* buffer, size_t size)
{
	char eom[] = { 0xd, 0xd, 0xd };
	time_t t1,t2;
	ssize_t r = 0;
	time (&t1);
	t2 = t1;
	while (1) {
		int delay;
		/*check if we have received whole packet */
		if (r > (ssize_t)sizeof(eom) && !memcmp(buffer + r - sizeof(eom), eom, sizeof(eom)))
			return r;

		/*if there is \r\r at the end of packet already, wait one second :( */
		if (r > (ssize_t)(sizeof(eom) - 1) && !memcmp(buffer + r - (sizeof(eom) - 1), eom, sizeof(eom) - 1))
			delay = 1;
		else
			delay = m_timeout - (t2 - t1);

		if (!Wait(delay))
			return r;

		ssize_t ret;
again:
		ret = read(m_fd, buffer + r , size - r - 1);
		if (ret == -1) {
			if (errno == EINTR)
				goto again;
			else {
				Close();
				sz_log(1, "read failed, errno %d (%s)",
					errno, strerror(errno));
				throw SerialException();
			}
		}
		sz_log(8, "Read %zd bytes from port", r);
		r += ret;
		buffer[r] = 0;

		time(&t2);
	}
}

void SerialPort::WriteData(const char* buffer, size_t size)
{
	size_t sent = 0;
	int ret;
	while (sent < size) {
#ifdef PL2303_WORKAROUND
		ret = write(m_fd, buffer + sent, 1);
#else
		ret = write(m_fd, buffer + sent, size - sent);
#endif

		if (ret < 0) {
			if (errno != EINTR) {
				Close();
				sz_log(2, "write failed, errno %d (%s)",
					errno, strerror(errno));
				throw SerialException();
			} else {
				continue;
			}
		} else if (ret == 0) {
			Close();
			sz_log(2, "wrote 0 bytes to a port");
			throw SerialException();
		}
		sent += ret;
	}
}

void UnitExaminator::ConfigExtraSpeed(DaemonConfig* cfg, int unit_num)
{
	ASSERT(cfg != NULL);
	ASSERT(unit_num >= 0);
	xmlDocPtr doc = cfg->GetXMLDoc();
	ASSERT(doc != NULL);
	xmlXPathContextPtr xp_ctx = xmlXPathNewContext(doc);
	ASSERT(xp_ctx != NULL);
	xp_ctx->node = cfg->GetXMLDevice();
	int ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "ipk",
		SC::S2U(IPK_NAMESPACE_STRING).c_str());
	ASSERT(ret == 0);
	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "extra",
		BAD_CAST IPKEXTRA_NAMESPACE_STRING);
	ASSERT(ret == 0);

	char *expr;
	if (asprintf(&expr, ".//ipk:unit[position()=%d]/@extra:speed",
			unit_num + 1) == -1) {
		sz_log(0, "error occured reading extra:speed");
		throw SerialException();
	}
	ASSERT(expr);
	xmlChar *c = uxmlXPathGetProp(BAD_CAST expr, xp_ctx, false);
	free(expr);
	if (c != NULL) {
		m_extraspeed = atoi((char *)c);
		xmlFree(c);
		sz_log(8, "found extra:speed %d for unit %d",
			m_extraspeed, unit_num+1);
	}
	xmlXPathFreeContext(xp_ctx);
}

bool UnitExaminator::ParseResponse(char* response, size_t count, Response* resp)
{
	char **toks;
	int tokc = 0;

	tokenize_d(response, &toks, &tokc, "\r");

	if (tokc < 3) {
		tokenize_d(NULL, &toks, &tokc, NULL);
		if (m_debug) {
			printf("Unable to parse response, it is too short.\n");
		}
		return false;
	}

	resp->m_params_count = tokc - 5;

	resp->m_id = toks[2][0];

	int checksum = 0;
	for (size_t j = 0; j < count; j++) {
		checksum += (uint) response[j];
	}
	/* Checksum without checksum ;-) and last empty lines */
	for (char *c = toks[tokc-1]; *c; c++) {
		checksum -= (uint) *c;
	}
	for (char *c = &response[count-1]; *c == '\r'; c--) {
		checksum -= (uint) *c;
	}
	if (checksum != atoi(toks[tokc-1]))  {
		if (m_debug)
			printf("Wrong checksum in response.\n");
		sz_log(4, "Wrong checksum");
		tokenize_d(NULL, &toks, &tokc, NULL);
		return false;
	}

	resp->m_params = (short*)malloc(sizeof(short) * resp->m_params_count);
	for (size_t i = 0; i < resp->m_params_count ; i++) {
		resp->m_params[i] = atoi(toks[i+4]);
	}
	tokenize_d(NULL, &toks, &tokc, NULL);
	return true;
}

void UnitExaminator::PrintResponse(const Response &resp)
{
	printf("unit id:%c, params count: %zu\n", resp.m_id,
			resp.m_params_count);
	for (size_t i = 0;i < resp.m_params_count; ++i) {
		printf("param no: %03zu, value: %06hd\n", i,
				resp.m_params[i]);
	}
}

void UnitExaminator::DrawHex(const char *c, size_t size)
{
	size_t i, j, k;

	char ic[17];
	j = 0;

	if (size <= 1)
		return;

	for (i = 0; i < size;i++) {
		printf("%02X ", c[i]);

		if (isprint(c[i])) {	/*Only printable chars*/
			ic[j] = c[i];
		} else {
			ic[j]='.';
		}

		j++;
		ic[j]=0;

		if ((((i + 1) % 16) == 0)) {
			printf("%c%c%c%s\n", 0x20, 0x20, 0x20, ic);
			j = 0;
		}
	}

	if ((i % 16) != 0) {
		for (k = 0;k < (16 - (i % 16));k++) {
			printf("%c%c%c",0x20,0x20,0x20);
		}
		printf("%c%c%c",0x20,0x20,0x20);
		printf("%s\n",ic);
	}
}

void Sniffer::GenerateQuery(char *&query, size_t& size)
{
	/*empty, we are not sending anything*/
	query = NULL;
	size = 0;
}

void Sniffer::SetData(char *response, size_t count)
{
	if (m_dump_hex)
		DrawHex(response, count);
	Response resp;
	if (ParseResponse(response, count, &resp))
		PrintResponse(resp);

	free(resp.m_params);
}

void Scanner::GenerateQuery(char *&query, size_t& size)
{
	printf("Probing device %c\n", m_id);
	size = asprintf(&query,"\x11\x02P%c\x03\n", m_id);
	if (m_dump_hex) {
		printf("Sending:\n");
		DrawHex(query,size);
	}
	m_id++;
	if (m_id > m_id2) {
		m_id = m_id1;
	}
}

void Scanner::SetData(char *response, size_t count)
{
	if (count <= 0) {
		printf("No response\n");
		return;
	}

	printf("Got response.\n");
	if (m_dump_hex) {
		DrawHex(response, count);
	}
	Response resp;
	if (ParseResponse(response, count, &resp)) {
		PrintResponse(resp);
	}
	free(resp.m_params);
}

/******************************************************************/

/**Class is responsible for communication with a unit, generates
 * queries based on data received from parcook, stores recevied
 * data into provided buffer. Averages received values.
 */
class ZetUnit : public UnitExaminator {
public:
	/**
	 * @param unit IPK API object describing a unit
	 * @param read pointer to a data buffer whose values are copied into
	 * parcook shared memory segment
	 * @param pointer to a buffer holding values received from sender
	 * @param direct if true the unit has not assigned an id
	 * @param debug indicates if all data recived from and sent to unit
	 * should be print to stdout*/
	ZetUnit(TUnit *unit, short* read, short* send,
			bool direct, bool debug, bool dump_hex);
	/**Generates query that should be sent to a unit
	 * @param query output parameter, points to a buffer with query,
	 * it's caller responsiblity to relase this buffer
	 * @param size output parameter, size of a buffer*/
	virtual void GenerateQuery(char*& query, size_t& size);
	/**
	 * Parses data received from a unit, puts data
	 * into shared memory segment.
	 * @param response pointer to a buffer containing unit's
	 * answer to a query
	 * @param count size of response
	 */
	virtual void SetData(char* response, size_t count);
	virtual void SetNoData();
	virtual ~ZetUnit() { delete m_avg_buffer; delete [] m_nodata_send;}
protected:
	/**Averaging buffer*/
	class AvgBuffer {
	public:
		/**Constructor
		 * @param size number of probes in single element of a buffer
		 * @param avg_size maximum size of a buffer*/
		AvgBuffer(const size_t size ,const size_t avg_size) :
			m_size(size), m_avg_size(avg_size)
		{ }
		/*Inserts SZARP_NO_DATA values into a buffer*/
		void SetNoData() { DecreaseSize(); }
		/*Inserts data into a buffer, the length of the buffer should
		 * be equal to @see m_size, table pointed by the param data
		 * will be freed by the @see AvgBuffer
		 * @param data pointer to a table of probes' values
		 */
		void SetData(short *data);
		/*Retreives mean value of each probe held by a buffer
		 * @param data mean probes values' are copied into table
		 * 	pointed by this param*/
		void GetValues(short *data) const;
		~AvgBuffer();
	private:
		/**removes least recent probes from the buffer*/
		void DecreaseSize();
		const size_t m_size;	 /**<number of probes in a single buffer element*/
		const size_t m_avg_size; /**<maximum size of buffer*/
		deque<short*> m_avg_buf; /**<holds buffer data*/
	}; /* class AvgBuffer */

	AvgBuffer* m_avg_buffer;	/**<pointer to a averaging buffer*/
	short* m_read;			/**<pointer to a data buffer whose
					values are copied into parcook
					shared memory segment*/
	size_t m_read_count;		/**<size of buffer pointed by
					@see m_read */
	short* m_send;			/**<pointer to a buffer holding values
					received from sender*/
	bool* m_nodata_send;		/**<Array of boolean flags. Flags denote
					 if, for a given send param, SZARP_NO_DATA
					 shall be sent to a regulator.*/
	size_t m_sends_count;		/**<size of buffer pointed by @see
					m_send */
	const bool m_direct;		/**<indicated if this unit has assigned
					an address*/
	char m_id;			/**<id(address) a of unit*/
};

void ZetUnit::AvgBuffer::DecreaseSize()
{
	if (m_avg_buf.size() > 0) {
		short* buf = m_avg_buf.front();
		free(buf);
		m_avg_buf.pop_front();
	}
}

void ZetUnit::AvgBuffer::SetData(short *data)
{
	m_avg_buf.push_back(data);
	if (m_avg_buf.size() > m_avg_size) {
		DecreaseSize();
	}
}

void ZetUnit::AvgBuffer::GetValues(short *data) const
{
	size_t buf_size = m_avg_buf.size();
	if (buf_size > 0)
		for (size_t i = 0 ; i < m_size ; ++i) {
			int val = 0;
			int count = 0;
			for (size_t j = 0; j < buf_size ; ++j) {
				short probe = m_avg_buf[j][i];
				if (probe == SZARP_NO_DATA)
					continue;

				val += probe;
				count++;
			}
			if (count)
				data[i] = val / count;
			else
				data[i] = SZARP_NO_DATA;
		}
	else
		for (size_t i = 0 ; i < m_size ; ++i)
			data[i] = SZARP_NO_DATA;

}

ZetUnit::AvgBuffer::~AvgBuffer()
{
	for (size_t i = 0; i < m_avg_buf.size(); ++i) {
		free(m_avg_buf[i]);
	}
}

ZetUnit::ZetUnit(TUnit *unit, short* read, short* send,
		bool direct, bool debug, bool dump_hex) :
	UnitExaminator(debug, dump_hex),
	m_read(read), m_send(send), m_direct(direct)
{
	if (!m_direct) {
		m_id = unit->GetId();
	}
	m_read_count = unit->GetParamsCount();
	m_sends_count = unit->GetSendParamsCount();

	m_avg_buffer = new AvgBuffer(m_read_count, unit->GetBufSize());

	m_nodata_send = new bool[m_sends_count];

	size_t i = 0;
	TSendParam* sp = unit->GetFirstSendParam();
	for (; sp && i < m_sends_count; sp = sp->GetNext(), ++i)
		m_nodata_send[i] = sp->GetSendNoData() ? true : false;

	//IPK check ;)
	ASSERT(sp == NULL && i == m_sends_count);

}

void ZetUnit::GenerateQuery(char*& query, size_t& size)
{
	char msg_footer[] = "\x03\n";
	short sender_checksum = 0;

	FILE* query_buf = open_memstream(&query, &size);
	if (!m_direct) {
		fprintf(query_buf, "\x11\x02P%c", m_id);
		sz_log(8, "Sending query to unit:%c", m_id);
		if (m_debug) {
			printf("Sending query to unit:%c\n", m_id);
		}
		bool sender_data = false;
		for (size_t i = 0; i < m_sends_count; ++i) {
			short value = m_send[i];

			if (value == SZARP_NO_DATA &&
					m_nodata_send[i] == false)
				continue;

			m_send[i] = SZARP_NO_DATA;
			sender_data = true;

			fprintf(query_buf, "%zu,%hd,", i, value);
			sender_checksum += i + value;
			sender_checksum &= 0xffff;
			sz_log(8,"Setting send param no:%zu value:%hd\n",
					i, value);
		}
		if (sender_data)  {
			/* add checksum, strip trailing comma */
			fprintf(query_buf, "%d,%hd",
					SENDER_CHECKSUM_PARAM, sender_checksum);
		}
		fprintf(query_buf, "%s", msg_footer);
	} else {
		sz_log(8, "Sending query to unit with no address");
		fprintf(query_buf, "\x11P\x0a");
	}

	fclose(query_buf);

	sz_log(8,"Query: %s", query);

	if (m_dump_hex) {
		printf("The packet:\n");
		DrawHex(query, size);
	}
}

void ZetUnit::SetData(char* response, size_t count)
{
	ASSERT(response != NULL);

	if (m_dump_hex) {
		printf("Received packet:\n");
		DrawHex(response, count + 1);
	}
	Response resp;
	if (!ParseResponse(response, count, &resp)) {
		goto bad;
	}
	if (m_debug) {
		PrintResponse(resp);
	}
	if (resp.m_id != m_id) {
		sz_log(2, "Wrong id expected:%c, got:%c", m_id, resp.m_id);
	}
	if (resp.m_params_count != m_read_count ) {
		sz_log(2, "Wrong params count expected:%zu, got:%zu",
				m_read_count, resp.m_params_count);
		m_avg_buffer->SetNoData();
		goto bad;
	}
	m_avg_buffer->SetData(resp.m_params);
	m_avg_buffer->GetValues(m_read);
	return;
bad:
	m_avg_buffer->SetNoData();
	m_avg_buffer->GetValues(m_read);
	free(resp.m_params);
}

void ZetUnit::SetNoData()
{
	m_avg_buffer->SetNoData();
	m_avg_buffer->GetValues(m_read);
}

/******************************************************************/

class Daemon {
#define BUF_SIZE 1000	/**<transmit buffer size*/
#define WAIT_TIME 4	/**<time we will wait for unit to answer (seconds)*/
#define CYCLE_TIME 10	/**<lenght of a full daemon cycle (seconds)*/
public:
	Daemon(DaemonConfig *cfg) : m_ipc(NULL) {
		ConfigureDaemon(cfg);
	}
	/*Main daemon's routine, never returns.
	 * Periodically sends queries to units via SerialPort class.
	 * Fetches data from sender and stores values retrieved from units
	 * into parcook shared memory segment.*/
	void Go();
private:

	void SuspendOperations();

	void ConfigureDaemon(DaemonConfig *cfg);
	/**Object responsible for communication with sender and parcook*/
	IPCHandler *m_ipc;
	/**DaemonConfig - daemon configuration*/
	DaemonConfig *m_cfg;
	/**Head of list of units present on the line*/
	deque<UnitExaminator*> m_units;
	/**Object communicating with serial port*/
	SerialPort *m_port;
	/**Delay between quieries*/
	int m_delay;
	/**Cycle length(in seconds)*/
	int m_cycle_duration;
};

void Daemon::ConfigureDaemon(DaemonConfig *cfg)
{
	bool debug = cfg->GetSingle() || cfg->GetDiagno();
	const char *device_name = strdup(cfg->GetDevicePath());
	if (!device_name) {
		sz_log(0,"No device specified -- exiting");
		exit(1);
	}

	int speed = cfg->GetSpeed();
	int stopbits = cfg->GetNoConf() ? 1 : cfg->GetDevice()->GetStopBits();
	int wait_time;

	int dumphex = cfg->GetDumpHex();
	char id1,id2;
	if (cfg->GetSniff())  {
		m_units.push_back(new Sniffer(debug,dumphex));
		wait_time = WAIT_TIME;
		m_cycle_duration = 0;
	} else if (cfg->GetScanIds(&id1,&id2)) {
		m_units.push_back(new Scanner(id1,id2,debug,dumphex));
		wait_time = WAIT_TIME;
		m_cycle_duration = 0;
	} else {	/* we are acting as a regular daemon*/
		if (cfg->GetNoConf()) {
			sz_log(0, "Cannot act as daemon if no configuration is loaded");
			exit(1);
		}
		try {
			auto ipc_ = std::unique_ptr<IPCHandler>(new IPCHandler(*m_cfg));
			m_ipc = ipc_.release();
		} catch(...) {
			exit(1);
		}

		TDevice* dev = cfg->GetDevice();
		bool special = dev->IsSpecial();
		short *read = m_ipc->m_read;
		short *send = m_ipc->m_send;

		TUnit* unit = dev->GetFirstRadio()->GetFirstUnit();
		for (int i = 0; unit; unit = unit->GetNext(), i++) {
			ZetUnit *zet = new ZetUnit(unit,
						read,
						send,
						special,
						debug,
						dumphex
						);
			zet->ConfigExtraSpeed(cfg, i);
			m_units.push_back(zet);
			read += unit->GetParamsCount();
			send += unit->GetSendParamsCount();
		}
		wait_time = WAIT_TIME;
		m_cycle_duration = CYCLE_TIME;
	}
	m_port = new SerialPort(device_name,
			speed,
			stopbits,
			wait_time);
	try {
		m_port->Open();
	} catch (SerialException&) {
		sz_log(0, "Failed to open port:%s\n",device_name);
		exit(1);
	}
	m_delay = cfg->GetAskDelay();
}

void Daemon::Go()
{
	char buffer[BUF_SIZE];
	while (true) {

		if (stop_daemon)
			SuspendOperations();

		time_t t1,t2;
		time(&t1);

		if (m_ipc) {
			m_ipc->GoSender();
		}
		for (deque<UnitExaminator*>::iterator iter = m_units.begin();
				iter != m_units.end(); ++iter) {
			char *query = NULL;
			size_t query_size;

			UnitExaminator* unit = *iter;
			try {
				m_port->Open(unit->GetExtraSpeed());
				unit->GenerateQuery(query, query_size);
				m_port->WriteData(query, query_size);
				free(query);
				query = NULL;
				ssize_t read;
				read = m_port->GetData(buffer, BUF_SIZE);
				unit->SetData(buffer, read);

				if (read == 0)
					m_port->Close();

			} catch (SerialException&) {
				free(query);
				unit->SetNoData();
			}

			sleep(m_delay);
		}

		time(&t2);
		int rest = m_cycle_duration - (t2 - t1);
		while (rest > 0 && !stop_daemon) {
			rest = sleep(rest);
		}
		if (m_ipc) {
			m_ipc->GoParcook();
		}

	}
}
#undef BUF_SIZE
#undef WAIT_TIME
#undef CYCLE_TIME

void Daemon::SuspendOperations() {
	m_port->Close();

	int suspend_time = 3 * 60;

	int ret = kill(psetd_pid, SIGUSR1);
	if (ret) {
		sz_log(2, "Failed to signal psetd %s\n", strerror(errno));
		return;
	}
	sz_log(2, "Signalling suspned to %d", (int)psetd_pid);

	while (stop_daemon &&
			(suspend_time = sleep(suspend_time)));

	/*psetd didn't wake us up, continue anyway*/
	if (stop_daemon) {
		sz_log(0, "PSetd daemon did not wake as up after 3 minutes, resuming operations anyway");
		stop_daemon = 0;
	} else
		kill(psetd_pid, SIGUSR1);
}

int main(int argc, char *argv[]) {

	DaemonConfig* cfg = new DaemonConfig("nrsdmn");

	if (cfg->Load(&argc, argv)) {
		return 1;
	}

	sigset_t sigset;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGUSR1);
	sigprocmask(SIG_UNBLOCK, &sigset, NULL);

	struct sigaction sa;
	sa.sa_sigaction = sig_usr_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO;
	sigaction(SIGUSR1, &sa, NULL);


	try {
		Daemon daemon(cfg);
		if (cfg->GetNoConf() == 0) {
			/* performs libxml cleanup ! */
			cfg->CloseXML(1);
			/* performs IPK cleanup ! */
			cfg->CloseIPK();
		}
		daemon.Go();
	} catch (std::exception&) {
		sz_log(0, "Daemon died");
	}

	return 1;
}

