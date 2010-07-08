/* SZARP: SCADA software 
  

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

/** Daemon for querying energy meters.
 * $Id$
 */

/*
 @description_start
 @class 4
 @devices Energy meters using S-Bus (Saia-Burgess Bus) protocol.
 @devices.pl Liczniki energii u¿ywaj±ce protoko³u S-Bus (Saia-Burgess Bus).
 @description_end
*/


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <map>
#include <typeinfo>

#include <stdarg.h>

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/ioctl.h>

#include "liblog.h"
#include "dmncfg.h"
#include "ipchandler.h"
#include "tokens.h"
#include "xmlutils.h"
#include "conversion.h"

bool single;

void dolog(int level, const char * fmt, ...)
  __attribute__ ((format (printf, 2, 3)));

std::string tohex(const unsigned char* buffer, size_t len) {
	std::ostringstream oss;

	oss << "hex: ";

	oss.setf(std::ios_base::showbase);
	oss << std::setbase(16);

	for (size_t i = 0; i < len; ++i) {
		if ((i % 16) == 0)
			oss << std::endl;

		oss << std::setw(4) << (unsigned) buffer[i] << " ";
	}

	return oss.str();
}

const unsigned short crc_table[] = {
       0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,
       0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,0x72f7,0x62d6,0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,
       0x2462,0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58d,
       0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,0x46b4,0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,
       0x48c4,0x58e5,0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92b,
       0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,
       0x6ca6,0x7c87,0x4ce4,0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,0x8d68,0x9d49,
       0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,
       0x9188,0x81a9,0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16f,0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,0x6067,
       0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,0x02b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,
       0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50d,0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,
       0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,0x26d3,0x36f2,0x0691,0x16b0,0x6657,0x7676,0x4615,0x5634,
       0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,
       0xcb7d,0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,0x4a75,0x5a54,0x6a37,0x7a16,0x0af1,0x1ad0,0x2ab3,0x3a92,
       0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,0x8dc9,0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,
       0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,0x1ef0
};



class SerialException : public std::runtime_error {
	public:		
	SerialException(int err) : std::runtime_error(strerror(err)) {};
	SerialException(const std::string &err) : std::runtime_error(err) {};
};

/**Class handling comunication with serial port.*/
class SerialPort {
public:
	/**Constructor 
	 * @param path path to a serial port device
	 * @param speed connection speed
	 * @param stopbits number of stop bits
	 * @param timeout specifies number of seconds @see GetData should wait for data 
	 * to arrive on a port*/
	SerialPort(const char* path, int speed) : 
		m_fd(-1), m_path(path), m_speed(speed)
	{};
	/**Closes the port*/
	void Close();
	/**Opens the port throws @see SerialException 
	* if it was unable to open a port or set connection
	* parameters. Attempts to open a port only if it
	* is not already opened or is opened with another speed
	* @param force_speed speed for port, 0 for default device speed
	*/
	void Open();

	/*Reads data from a port to a given buffer,
	 * null terminates read data. Throws @see SerialException
	 * if read failed, or timeout occurred closes port in both cases.
	 * @param buffer buffer where data shall be placed
	 * @param size size of a buffer
	 * @return number of read bytes (not including terminating NULL)*/
	ssize_t GetData(void* buffer, size_t size);

	/*Writes contents of a supplied buffer to a port.
	 * Throws @see SerialException and closes the port
	 * if write operation failes.
	 * @param buffer buffer containing data that should be written
	 * @param size size of the buffer*/
	void WriteData(const void* buffer, size_t size);

	/**Waits for data arrival on a port, throws SerialException
	 * if no data arrived withing specified time interval
	 * or select call failed. In both cases port is closed.
	 * @param timeout number of seconds the method should wait*/
	bool Wait(int timeout);
private:

	int m_fd; 		/**<device descriptor, -1 indicates that the device is closed*/
	const char* m_path;	/**<path to the device*/
	int m_speed;		/**<configured connection speed (9600 def)*/ 
};

/**Data buffer*/
class Buffer {
	/**current buffer read position*/
	size_t m_read_pointer;
	/**current buffer content*/
	std::vector<unsigned char> m_values;
public:
	Buffer();
	/**one byte to the buffer*/
	void SetChar(size_t idx, unsigned char c);
	/**Puts an integer (four bytes) to the buffer*/
	void SetInt(size_t idx, unsigned int i);
	/**Puts an short (two bytes) to the buffer*/
	void SetShort(size_t idx, unsigned short s);
	/**one byte to the buffer*/
	unsigned char GetChar(size_t idx) const;
	/**an integer (four bytes) to the buffer*/
	unsigned int GetInt(size_t idx);
	/**Puts an integer (four bytes) to the buffer*/
	int GetSInt(size_t idx);
	/**Puts an short (two bytes) to the buffer*/
	unsigned short GetShort(size_t idx);
	/**Puts an short (two bytes) to the buffer*/
	short GetSShort(size_t idx);
	/**Puts one byte to the buffer*/
	void PutChar(unsigned char c);
	/**Puts an integer (four bytes) to the buffer*/
	void PutInt(unsigned int i);
	/**Puts an short (two bytes) to the buffer*/
	void PutShort(unsigned short s);
	/**Reads integer value from the buffer*/
	unsigned ReadInt();
	/**Reads signed integer value from the buffer*/
	int ReadSInt();
	/**Reads signed integer value from the buffer*/
	float GetFloat(size_t idx);
	/**Reads signed integer value from the buffer*/
	float ReadFloat();
	/**Reads char value from the buffer*/
	unsigned char ReadChar();
	/**Reads short value from the buffer*/
	unsigned short ReadShort();
	/**Reads unsinged long value from the buffer*/
	unsigned long long ReadLongLong();
	/**Reads long long value from the buffer*/
	long long ReadSLongLong();

	short ReadSShort();
	unsigned char* Content();
	/**@retrun pointer to the buffer content*/
	const unsigned char* Content() const;
	/**Appands data from this buffer to this object*/
	void PutData(const Buffer &b);
	/**@retrun number of bytes in the buffer*/
	size_t Size() const;
	/**@return refrence at value located at given index position*/
	unsigned char& at(size_t i);
	/**resizes buffer*/
	void Resize(size_t size);

};


bool SerialPort::Wait(int timeout) 
{
	int ret;
	struct timeval tv;
	fd_set set;
	time_t t1, t2;

	time(&t1);

	dolog(8, "Waiting for data");
	while (true) {
		time(&t2);
		tv.tv_sec = timeout - (t2 - t1);
		if (tv.tv_sec < 0) {
			dolog(2, "I cannot wait for negative number of seconds");
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
				dolog(1, "select failed, errno %d (%s)",
					errno, strerror(errno));
				throw SerialException(errno);
			}
		} else if (ret == 0) {
			dolog(4, "No data arrived within specified interval");
			return false;
		} else {
			dolog(8, "Data arrived");
			return true;
		}
	}
}

ssize_t SerialPort::GetData(void* buffer, size_t size) {
again:
	ssize_t ret;
	ret = read(m_fd, buffer, size);
	if (ret == -1) {
		if (errno == EINTR) {
			goto again;
		} else {
			Close();
			dolog(1, "read failed, errno %d (%s)",
				errno, strerror(errno));
			throw SerialException(errno);
		}
	}
	return ret;
}

void SerialPort::WriteData(const void* buffer, size_t size) {
	size_t sent = 0;
	int ret;

	const char* b = static_cast<const char*>(buffer);

	while (sent < size) {
		ret = write(m_fd, b + sent, size - sent);
			
		if (ret < 0) { 
			if (errno != EINTR) {
				Close();
				dolog(2, "write failed, errno %d (%s)",
					errno, strerror(errno));
				throw SerialException(errno);
			} else {
				continue;
			}
		} else if (ret == 0) {
			Close();
			dolog(2, "wrote 0 bytes to a port");
			throw SerialException(std::string("Error writing to port, 0 bytes writen"));
		}
		sent += ret;
	} 
}

void SerialPort::Open() 
{
	if (m_fd >= 0) 
		return;

	dolog(6, "Opening port: %s", m_path);

	m_fd = open(m_path, O_RDWR | O_NOCTTY | O_NONBLOCK, 0);
	
	if (m_fd == -1) {
		dolog(1, "nrsdmn: cannot open device %s, errno:%d (%s)", m_path,
			errno, strerror(errno));
		throw SerialException(errno);
	}
	
	struct termios ti;
	if (tcgetattr(m_fd, &ti) == -1) {
		dolog(1, "nrsdmn: cannot retrieve port settings, errno:%d (%s)",
				errno, strerror(errno));
		Close();
		throw SerialException(errno);
	}

	dolog(6, "setting port speed to %d", m_speed);
	switch (m_speed) {
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

	ti.c_oflag = 
	ti.c_iflag =
	ti.c_lflag = 0;

	ti.c_cflag |= CS8 | CREAD | CLOCAL;

	if (tcsetattr(m_fd, TCSANOW, &ti) == -1) {
		dolog(1, "Cannot set port settings, errno: %d (%s)",
			errno, strerror(errno));	
		Close();
		throw SerialException(errno);
	}
}

void SerialPort::Close() 
{
	if (m_fd < 0) 
		return;

	dolog(6, "Closing port: %s", m_path);

	close(m_fd);
	m_fd = -1;
}

Buffer::Buffer() : m_read_pointer(0) {}

void Buffer::SetChar(size_t idx, unsigned char c) {
	idx += m_read_pointer;

	m_values.at(idx) = c;
}

void Buffer::SetInt(size_t idx, unsigned int i) {
	idx += m_read_pointer;

	m_values.at(idx) = i >> 24;
	m_values.at(idx + 1) = (i >> 16) & 0xffu;
	m_values.at(idx + 2) = (i >> 8) & 0xffu;
	m_values.at(idx + 3) = i & 0xffu;
}

void Buffer::SetShort(size_t idx, unsigned short i) {
	idx += m_read_pointer;

	m_values.at(idx) = i >> 8;
	m_values.at(idx + 1) = i & 0xffu;
}

unsigned char Buffer::GetChar(size_t idx) const {
	size_t i = idx + m_read_pointer;
	return m_values.at(i);
}

unsigned int Buffer::GetInt(size_t idx) {
	size_t i = idx + m_read_pointer;

	unsigned int v = 0;
	v |= m_values.at(i) << 24;
	v |= m_values.at(i + 1) << 16;
	v |= m_values.at(i + 2) << 8;
	v |= m_values.at(i + 3);

	return v;
}

int Buffer::GetSInt(size_t idx) {
	size_t i = idx + m_read_pointer;

	int v = 0;
	v |= m_values.at(i) << 24;
	v |= m_values.at(i + 1) << 16;
	v |= m_values.at(i + 2) << 8;
	v |= m_values.at(i + 3);

	return v;
}

unsigned short Buffer::GetShort(size_t idx) {
	size_t i = idx + m_read_pointer;

	unsigned short v = 0;
	v |= m_values.at(i) << 8;
	v |= m_values.at(i + 1);

	return v;
}

short Buffer::GetSShort(size_t idx) {
	size_t i = idx + m_read_pointer;

	short v = 0;
	v |= m_values.at(i) << 8;
	v |= m_values.at(i + 1);

	return v;
}

void Buffer::PutChar(unsigned char c) {
	size_t s = Size();
	m_values.resize(s + 1 + m_read_pointer);
	SetChar(s, c);
}

void Buffer::PutInt(unsigned int i) {
	size_t s = Size();
	m_values.resize(s + 4 + m_read_pointer);
	SetInt(s, i);
}

void Buffer::PutShort(unsigned short i) {
	size_t s = Size();
	m_values.resize(s + 2 + m_read_pointer);
	SetShort(s, i);
}

unsigned int Buffer::ReadInt() {
	unsigned int v = GetInt(0);
	m_read_pointer += 4;
	return v;
}

int Buffer::ReadSInt() {
	int v = GetInt(0);
	m_read_pointer += 4;
	return v;
}

float Buffer::GetFloat(size_t idx) {
	float *af = (float*) &m_values[idx];
	return *af;
}

float Buffer::ReadFloat() {
	float f = GetFloat(0);
	m_read_pointer += 4;
	return f;
}

unsigned short Buffer::ReadShort() {
	unsigned short v = GetShort(0);
	m_read_pointer += 2;
	return v;
}

short Buffer::ReadSShort() {
	short v = GetSShort(0);
	m_read_pointer += 2;
	return v;
}

unsigned char Buffer::ReadChar() {
	unsigned char v = GetChar(0);
	m_read_pointer += 1;
	return v;
}

unsigned char* Buffer::Content() {
	return &m_values.at(m_read_pointer);
}

const unsigned char* Buffer::Content() const {
	return &m_values.at(m_read_pointer);
}


unsigned char& Buffer::at(size_t i) {
	return m_values.at(i + m_read_pointer);
}

size_t Buffer::Size() const {
	return m_values.size() - m_read_pointer;
}

void Buffer::Resize(size_t size) {
	m_values.resize(m_read_pointer + size);
}

#if 0
void Buffer::PutCharBack(unsigned char c) {
	m_values.at(--m_read_pointer) = c;
}
#endif

void Buffer::PutData(const Buffer& b) {
	size_t os = m_read_pointer + Size();
	m_values.resize(os + b.Size());
	std::copy(b.Content(), b.Content() + b.Size(), &m_values[os]);
}

unsigned long long Buffer::ReadLongLong() {
	unsigned long long l = 0;

	l = ReadChar();
	l = (l << 8) | ReadChar();
	l = (l << 8) | ReadChar();
	l = (l << 8) | ReadChar();
	l = (l << 8) | ReadChar();
	l = (l << 8) | ReadChar();
	l = (l << 8) | ReadChar();
	l = (l << 8) | ReadChar();

	return l;
}

long long Buffer::ReadSLongLong() {
	long long l = 0;

	l = ReadChar();
	l = (l << 8) | ReadChar();
	l = (l << 8) | ReadChar();
	l = (l << 8) | ReadChar();
	l = (l << 8) | ReadChar();
	l = (l << 8) | ReadChar();
	l = (l << 8) | ReadChar();
	l = (l << 8) | ReadChar();
		
	return l;
}

struct Param {
	int prec;
	int msw;
	int lsw;
	int x_m_conv;
	int y_m_conv;
	int x_a_conv;
	int y_a_conv;
	enum Type {
		DECIMAL, 
		FLOAT
	} type;
};

class SBUSUnit  {
	SerialPort *m_port;
	short *m_buffer;
	std::string m_id;
	int m_read_timeout;
	int m_max_read_attempts;
	std::map<unsigned short, Param> m_params;
	Buffer CreateQueryPacket(unsigned short start, int count);
	bool ReadResponse(int count, Buffer &dec);
	void SetParamsVals(unsigned short start, int count, Buffer& response);
	void QueryRange(unsigned short start, int count);
public:
	bool Configure(TUnit *unit, xmlNodePtr xmlunit, short *params, SerialPort *port);
	void QueryUnit();
};

bool SBUSUnit::Configure(TUnit *unit, xmlNodePtr xmlunit, short *params, SerialPort *port) {
	m_buffer = params;
	m_port = port;

	char *endptr;

	xmlChar* _id = xmlGetNsProp(xmlunit, BAD_CAST("id"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (_id == NULL) {
		dolog(0, "Attribbute sbus:id not present in unit element, line no (%ld)", xmlGetLineNo(xmlunit)); 
		return false;
	}
	m_id = (char*)_id;
	dolog(5, "Attribbute sbus:id is %s", m_id.c_str());
	m_id[0] -= '0';
	xmlFree(_id);

	m_read_timeout = 10;
	xmlChar* _read_timeout = xmlGetNsProp(xmlunit, BAD_CAST("read_timeout"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (_read_timeout != NULL) {
		m_read_timeout = strtoul((char*)_read_timeout, &endptr, 0);
		xmlFree(_read_timeout);
	}
	
	m_max_read_attempts = 1;
	xmlChar* _max_read_attempts = xmlGetNsProp(xmlunit, BAD_CAST("max_read_attempts"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (_max_read_attempts != NULL) {
		m_max_read_attempts = strtoul((char*)_max_read_attempts, &endptr, 0);
		xmlFree(_max_read_attempts);
	}

	xmlXPathContextPtr xp_ctx = xmlXPathNewContext(xmlunit->doc);
	xp_ctx->node = xmlunit;

	int ret;
	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "ipk", SC::S2U(IPK_NAMESPACE_STRING).c_str());
	assert(ret == 0);

	xmlXPathObjectPtr rset = xmlXPathEvalExpression(BAD_CAST "./ipk:param", xp_ctx);

	for (int i = 0; i < rset->nodesetval->nodeNr; ++i) {
		xmlNodePtr n = rset->nodesetval->nodeTab[i];
		xmlChar* _address = xmlGetNsProp(n,
				BAD_CAST("address"), 
				BAD_CAST(IPKEXTRA_NAMESPACE_STRING));

		if (_address == NULL) {
			dolog(0, "Error, attribute sbus:address missing in param definition, line(%ld)", xmlGetLineNo(n));
			return false;
		}

		unsigned short address = strtoul((char*)_address, &endptr, 0);
		if (*endptr != 0) {
			dolog(0, "Error, invalid value of parameter sbus:address in param definition, line(%ld)", xmlGetLineNo(n));
			return false;
		}

	       	int prec = 0;
		xmlChar* _prec = xmlGetNsProp(n,
				BAD_CAST("prec"), 
				BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
		if (_prec != NULL) {
			prec = strtol((char*)_prec, &endptr, 0);
			if (*endptr != 0) {
				dolog(0, "Error, invalid value of parameter sbus:prec in param definition, line(%ld)", xmlGetLineNo(n));
				return false;
			}
			xmlFree(_prec);
			dolog(5, "Got prec %d", prec);
		}

		bool is_msw = false;

		xmlChar* _is_msw = xmlGetNsProp(n,
				BAD_CAST("word"), 
				BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
		if (_is_msw && !xmlStrcmp(_is_msw, BAD_CAST("msw"))) 
			is_msw = true;
		xmlFree(_is_msw);

		bool has_type = false;
		Param::Type type = Param::DECIMAL;
		xmlChar* _type = xmlGetNsProp(n,
				BAD_CAST("type"),
				BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
		if (_type != NULL) {
			has_type = true;
			if (!xmlStrcmp(_type, BAD_CAST "float"))
				type = Param::FLOAT;
			xmlFree(_type);
		}

		xmlChar* _x_m_conv = xmlGetNsProp(n,
				BAD_CAST("x_m_conv"),
				BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
		xmlChar* _y_m_conv = xmlGetNsProp(n,
				BAD_CAST("y_m_conv"),
				BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
		xmlChar* _x_a_conv = xmlGetNsProp(n,
				BAD_CAST("x_a_conv"),
				BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
		xmlChar* _y_a_conv = xmlGetNsProp(n,
				BAD_CAST("y_a_conv"),
				BAD_CAST(IPKEXTRA_NAMESPACE_STRING));

		if (m_params.find(address) == m_params.end()) {
			Param a; 

			a.x_m_conv = 0;
			a.y_m_conv = 0;

			a.x_a_conv = 0;
			a.y_a_conv = 0;

			a.prec = 0;

			a.type = Param::DECIMAL;

			a.lsw = -1;
			a.msw = -1;

			m_params[address] = a;
		}

		Param& a = m_params[address];

		if (is_msw)
			a.msw = i;
		else
			a.lsw = i;

		if (prec)
			a.prec = prec;

		if (has_type)
			a.type = type;

		if (_x_m_conv)
			a.x_m_conv = atoi((char*) _x_m_conv);
		if (_y_m_conv)
			a.y_m_conv = atoi((char*) _y_m_conv);
		if (_x_a_conv)
			a.x_a_conv = atoi((char*) _x_a_conv);
		if (_y_a_conv)
			a.y_a_conv = atoi((char*) _y_a_conv);

		xmlFree(_x_a_conv);
		xmlFree(_y_a_conv);
		xmlFree(_x_m_conv);
		xmlFree(_y_m_conv);
	}

	return true;


}

Buffer EncodeB5Values(const Buffer &b) {
	Buffer r;

	if (b.Size() == 0)
		return r;

	//skip first  byte
	r.PutChar(b.GetChar(0));

	for (size_t i = 1; i < b.Size(); i++) {
		unsigned char c = b.GetChar(i);
		if (c == 0xb5u) {
			r.PutChar(0xc5u);
			r.PutChar(0x00u);
		} else if (c == 0xc5u) {
			r.PutChar(0xc5u);
			r.PutChar(0x01u);
		} else
			r.PutChar(c);
	}

	return r;

}

Buffer DecodeB5Values(const Buffer &b, size_t size) {
	Buffer r;

	if (size == 0)
		return r;

	//skip first  byte
	r.PutChar(b.GetChar(0));

	size_t i = 1;
	while (i < size) {
		unsigned char c = b.GetChar(i);
		if (c == 0xc5u) {
			unsigned c1 = r.GetChar(i + 1);
			if (c1 == 0x0)
				r.PutChar(0xb5u);
			else
				r.PutChar(0xc5u);
			i++;
		} else
			r.PutChar(c);
		i++;
	}

	return r;

} 

unsigned short CalculateCRC(const Buffer &b, size_t len) {
	unsigned short crc = 0;
	for (size_t i = 0; i < len; i++) {
		int index = (((crc >> 8) ^ b.GetChar(i)) & 0xff);
		crc = crc_table[index] ^ ((crc << 8) & 0xffff);
	}
	return crc;
}

Buffer SBUSUnit::CreateQueryPacket(unsigned short start, int count) {
	Buffer b;
	b.PutChar('\xb5');
	b.PutChar('\x00');
	b.PutChar(m_id.at(0));
	b.PutChar('\x06');
	b.PutChar(count - 1);
	b.PutShort(start);
	b.PutShort(CalculateCRC(b, b.Size()));
	Buffer enc = EncodeB5Values(b);
	return enc;
}

bool SBUSUnit::ReadResponse(int count, Buffer &dec) {
	size_t response_size = count * 4 
		+ 2 //checksum
		+ 1 //frame synchronizastion byte
		+ 1; //response code

	Buffer response;
	response.Resize(2 * response_size);
	size_t has_read = 0;

	if (!m_port->Wait(m_read_timeout)) {
		dolog(1, "Timeout while waiting for response for device %d", (int)m_id[0]);
		return false;
	}

	do {
		has_read += m_port->GetData(response.Content() + has_read, 2 - has_read);

		if (has_read == 2)
			break;

		if (m_port->Wait(1)) {
			dolog(1, "Timeout while waiting for character from device %d", (int)m_id[0]);
			return false;
		}

	} while (true);

	if (response.GetChar(1) != '\x01') {
		dolog(1, "Error while quering device %d, invalid response type %d", (int)m_id[0], (int) response.GetChar(1));
		return false;
	}

	while (has_read < 2 * response_size) {
		if (!m_port->Wait(1))
			break;
		has_read += m_port->GetData(response.Content() + has_read, 2 * response_size - has_read);
	}

	dec = DecodeB5Values(response, has_read);
	if (dec.Size() != response_size) {
		dolog(1, "Invalid lenght of response for device %d, expected:%zu, got:%zu", (int)m_id[0], response_size, dec.Size());
		return false;
	}

	unsigned short ccrc = CalculateCRC(dec, dec.Size() - 2);
	unsigned short pcrc = dec.GetShort(dec.Size() - 2);
	if (ccrc != pcrc) {
		dolog(1, "Invalid crc while querying device %d, calculated:%hu, received:%hu", (int)m_id[0], ccrc, pcrc);
		return false;
	}

	return true;
}

template<typename T> T mypow(T val, int e) {
	if (e > 0) 
		while (e--)
			val *= 10;
	else
		while (e++)
			val /= 10;

	return val;
}

void SBUSUnit::SetParamsVals(unsigned short start, int count, Buffer& response) {
	response.ReadChar();
	response.ReadChar();
	for (unsigned short i = start; i < start + count; i++) {
		assert(m_params.find(i) != m_params.end());
		Param& p = m_params[i];

		unsigned val = response.ReadInt();
		int rval;

		switch (p.type) {
			case Param::FLOAT: {
				float v = *((float*)&val);
				if (p.x_m_conv != 0 && p.y_m_conv != 0) {
					assert(p.x_m_conv != p.y_m_conv);
					assert(p.x_a_conv != p.y_a_conv);
	
					v = (v - p.x_m_conv) * (p.y_a_conv - p.x_a_conv) / (p.y_m_conv - p.x_m_conv) + p.x_a_conv;
				}
				rval = mypow(v, p.prec);	
				break;
			}
			case Param::DECIMAL:
				if (p.x_m_conv != 0 && p.y_m_conv != 0) {
					assert(p.x_m_conv != p.y_m_conv);
					assert(p.x_a_conv != p.y_a_conv);
	
					val = (val - p.x_m_conv) * (p.y_a_conv - p.x_a_conv) / (p.y_m_conv - p.x_m_conv) + p.x_a_conv;
				}
				rval = mypow(val, p.prec);
				break;
		}


		unsigned uval = *((unsigned*) &rval);

		if (p.msw != -1)
			m_buffer[p.msw] = uval >> 16;

		if (p.lsw != -1)
			m_buffer[p.lsw] = uval & 0xffffu;

	}

}


void SBUSUnit::QueryRange(unsigned short start, int count) {
	Buffer query = CreateQueryPacket(start, count);
	
	bool data_read = false;
	int no_of_tries = 0;	
	Buffer response;
	while (!data_read && no_of_tries++ < m_max_read_attempts) {
		m_port->Open();
		m_port->WriteData(query.Content(), query.Size());

		response = Buffer();
		if (ReadResponse(count, response) == false)
			m_port->Close();
		else
			data_read = true;

	}

	SetParamsVals(start, count, response);
}


void SBUSUnit::QueryUnit() {
	if (m_params.size() == 0)
		return;
	
	std::map<unsigned short, Param>::iterator i = m_params.begin();
	
	unsigned short start = i->first;
	int count = 1;

	while (++i != m_params.end()) {
		unsigned short next = i->first;
		if (next != start + count) {
			QueryRange(start, count);
			start = next;
			count = 1;
		} else 
			count++;

		if (count == 32) {
			QueryRange(start, count - 1);
			start = next;
			count = 1;
		}

	}
	QueryRange(start, count);
}

class SBUSDaemon {
	std::vector<SBUSUnit*> m_units;
	int m_read_freq;
	IPCHandler *m_ipc;
	SerialPort *m_port;
	void WarmUp();
public:
	bool Configure(DaemonConfig* cfg);
	void Start();
};

bool SBUSDaemon::Configure(DaemonConfig *cfg) {
	m_ipc = new IPCHandler(cfg);
	if (m_ipc->Init()) {
		dolog(0, "Intialization of IPCHandler failed");
		return false;
	}

	m_port = new SerialPort(cfg->GetDevicePath(),
		       cfg->GetSpeed());

	TDevice* dev = cfg->GetDevice();
	xmlNodePtr xdev = cfg->GetXMLDevice();

	m_read_freq = 10;

	xmlChar* _read_freq = xmlGetNsProp(xdev, BAD_CAST("read_freq"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (_read_freq != NULL) {
		char *endptr;
		m_read_freq = strtoul((char*)_read_freq, &endptr, 0);
		if (*endptr) {
			dolog(0, "Invalid value for sbus:read_freq attribbute device element, line no (%ld)", xmlGetLineNo(xdev)); 
			xmlFree(_read_freq);
			return false;
		}
		xmlFree(_read_freq);
	}

	xmlXPathContextPtr xp_ctx = xmlXPathNewContext(xdev->doc);
	xp_ctx->node = xdev;

	int ret;
	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "ipk", SC::S2U(IPK_NAMESPACE_STRING).c_str());
	assert(ret == 0);

	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "sbus", BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	assert(ret == 0);


	xmlXPathObjectPtr rset = xmlXPathEvalExpression(BAD_CAST ".//ipk:unit", xp_ctx);

	short* reads = m_ipc->m_read;

	int i = 0;
	for (TUnit *unit = dev->GetFirstRadio()->GetFirstUnit(); unit; unit = unit->GetNext(), i++) {
		assert(i < rset->nodesetval->nodeNr);

		SBUSUnit* u  = new SBUSUnit();
		if (u->Configure(unit, rset->nodesetval->nodeTab[i], reads, m_port) == false)
			return false;

		m_units.push_back(u);

		reads += unit->GetParamsCount();
	}

	return true;

}

void SBUSDaemon::WarmUp() {
	time_t now = time(NULL);
	int left = now % m_read_freq;
	if (left) {
		int s = m_read_freq - left;
		while (s)
			s = sleep(s);

	}
}

void SBUSDaemon::Start() {

	WarmUp();

	while (true) {
		dolog(6, "Beginning parametr fetching loop");

		m_port->Open();

		for (int i = 0; i < m_ipc->m_params_count; ++i) 
			m_ipc->m_read[i] = SZARP_NO_DATA;

		time_t st = time(NULL);

		for (std::vector<SBUSUnit*>::iterator i = m_units.begin();
				i != m_units.end();
				++i) {
			try {
				(*i)->QueryUnit();
			} catch (std::exception &e) {
				dolog(0, "Exception occured while quering %s unit", e.what());
				m_port->Close();
			}
		}

		m_ipc->GoParcook();

		int s = st + m_read_freq - time(NULL);
		while (s > 0) 
			s = sleep(s);

		dolog(6, "End of parametrs' fetching loop");
	}
}

int main(int argc, char *argv[]) {
	DaemonConfig* cfg = new DaemonConfig("sbusdmn");

	if (cfg->Load(&argc, argv))
		return 1;

	single = cfg->GetSingle() || cfg->GetDiagno();

	SBUSDaemon daemon;
	if (daemon.Configure(cfg) == false) 
		return 1;

	daemon.Start();

	return 1;
}



void dolog(int level, const char * fmt, ...) {
	va_list fmt_args;

	char *l;
	va_start(fmt_args, fmt);

	if (single) {
		vasprintf(&l, fmt, fmt_args);
		std::cout << l << std::endl;
		sz_log(level, "%s", l);
		free(l);
	} else
		vsz_log(level, "%s", fmt_args);

	va_end(fmt_args);
} 


