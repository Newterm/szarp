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

/** Daemon for querying energy meters.*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(HAVE_BOOST) && defined(HAVE_LIBLBER)

#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <map>
#include <typeinfo>

#include <boost/variant.hpp>

#include <stdarg.h>

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/ioctl.h>

#include <lber.h>

#include "liblog.h"
#include "dmncfg.h"
#include "ipchandler.h"
#include "tokens.h"
#include "xmlutils.h"
#include "conversion.h"

bool g_single;

void dolog(int level, const char * fmt, ...)
  __attribute__ ((format (printf, 2, 3)));

template<typename T> int get_bit(T val, unsigned i) {
	return ((val & (1 << i))) != 0 ? 1 : 0;
}

unsigned char byte(unsigned b7, unsigned b6, unsigned b5, unsigned b4, unsigned b3, unsigned b2, unsigned b1, unsigned b0) {
	return (b7 << 7) | (b6 << 6) | (b5 << 5) | (b4 << 4) | (b3 << 3) | (b2 << 2) | (b1 << 1) | b0;
}

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

class SerialException : public std::runtime_error {
	public:		
	SerialException(int err) : std::runtime_error(strerror(err)) {};
	SerialException(const std::string &err) : std::runtime_error(err) {};
};

class InvalidHDLCFrame : public std::runtime_error {
	public:		
	InvalidHDLCFrame(std::string error) : std::runtime_error(error) {};
};

class InvalidHDLCPacketCRC : public std::runtime_error {
	public:		
	InvalidHDLCPacketCRC() : std::runtime_error("") {};
};

class DeviceResponseTimeout : public std::runtime_error {
	public:		
	DeviceResponseTimeout() : std::runtime_error("") {};
};

class IECException : public std::runtime_error {
	public:		
	IECException(const char *err) : std::runtime_error(err) {};
};

class COSEMException : public std::runtime_error {
	public:	
	COSEMException(std::string err) : std::runtime_error(err) {};
};

/**Class handling comunication with serial port.*/
class SerialPort {
public:
	/**Constructor 
	 * @param path path to a serial port device
	 * to arrive on a port*/
	SerialPort(const char* path) : 
		m_fd(-1), m_path(path), m_ispeed(-1), m_ospeed(-1)
	{};
	/**Closes the port*/
	void Close();
	/**Opens the port throws @see SerialException 
	* if it was unable to open a port or set connection
	* parameters. Attempts to open a port only if it
	* is not already opened or is opened with another speed
	* @param speed - speed of opened port connection
	*/
	void Open(int ispeed, int ospeed = -1);

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
	int m_ispeed;		
	int m_ospeed;
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

namespace {
	speed_t speed_to_speed_t(int speed) {
		switch (speed) {
			case 300:
				return B300;
			case 600:
				return B600;
			case 1200:
				return B1200;
			case 2400:
				return B2400;
			case 4800:
				return B4800;
			case 9600:
				return B9600;
			case 19200:
				return B19200;
			case 38400:
				return B38400;
			default:
				return B9600;
		}

	}

}

void SerialPort::Open(int ispeed, int ospeed) 
{
	if (ospeed == -1)
		ospeed = ispeed;	

	if (m_fd >= 0 && ispeed == m_ispeed && ospeed == m_ospeed) 
		return;

	m_ispeed = ispeed;
	m_ospeed = ospeed;

	if (m_fd < 0) {
	
		dolog(6, "Opening port: %s", m_path);

		//m_fd = open(m_path, O_RDWR | O_NOCTTY | O_NONBLOCK, 0);
		m_fd = open(m_path, O_RDWR | O_NOCTTY, 0);
	
		if (m_fd == -1) {
			dolog(1, "dlmsdmn: cannot open device %s, errno:%d (%s)", m_path,
				errno, strerror(errno));
			throw SerialException(errno);
		}
	}

#if 0
#if 1
	int status;

	if (ioctl(m_fd, TIOCMGET, &status) == -1) {
		dolog(1, "Failed to get modem lines status %d(%s)", errno, strerror(errno));	
		return;
	}

#if 0
	if ((status & (TIOCM_DTR | TIOCM_RTS)) != (TIOCM_RTS | TIOCM_DTR)) {
#endif
	//status |= TIOCM_DTR | TIOCM_RTS | TIOCM_CTS | TIOCM_DSR;
	status |= TIOCM_DTR | TIOCM_RTS;
	if (ioctl(m_fd, TIOCMSET, &status) == -1)
		dolog(1, "Failed to set modem lines status %d(%s)", errno, strerror(errno));	
#if 0
	}
#endif
#endif
#endif
	
	struct termios ti;
	if (tcgetattr(m_fd, &ti) == -1) {
		dolog(1, "nrsdmn: cannot retrieve port settings, errno:%d (%s)",
				errno, strerror(errno));
		Close();
		throw SerialException(errno);
	}

	dolog(6, "setting port speed to %d %d", m_ispeed, m_ospeed);

	ti.c_oflag = 
	ti.c_iflag =
	ti.c_lflag = 0;

//	ti.c_cflag |= CREAD | CS8 | CLOCAL;
	ti.c_cflag |= CREAD | CS8 | CLOCAL;

	cfsetospeed(&ti, speed_to_speed_t(m_ospeed));
	cfsetispeed(&ti, speed_to_speed_t(m_ispeed));

	if (tcsetattr(m_fd, TCSAFLUSH, &ti) == -1) {
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
	unsigned char GetChar(size_t idx);
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
	unsigned ReadTag() { return ReadChar(); };
	unsigned ReadLength();
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

Buffer::Buffer() : m_read_pointer(0) {}

void Buffer::SetChar(size_t idx, unsigned char c) {
	idx += m_read_pointer;

	m_values.at(idx) = c;
}

unsigned Buffer::ReadLength() {
	unsigned char l = ReadChar();
	if ((l & 0x80u) == 0)
		return l;

	unsigned lc = 0;
	for (unsigned i = 0; i < (l & 0x7fu); i++)
		lc = (lc << 8) | ReadChar();

	return lc;
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

unsigned char Buffer::GetChar(size_t idx) {
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

class IECConnector {
	SerialPort *m_port;
	std::string m_address;
	void AddParityBit(std::vector<unsigned char>& b);
public:
	IECConnector(SerialPort *port, std::string m_address);
	int Connect();	
};

IECConnector::IECConnector(SerialPort* port, std::string address) : m_port(port), m_address(address) {}

void IECConnector::AddParityBit(std::vector<unsigned char> &b) {
	for (size_t i = 0; i < b.size(); i++) {
		int mask = 1;
		bool even = false;
		for (size_t j = 0; j < 7; j++, mask <<= 1) 
			if (b[i] & mask)
				even = !even;

		if (even) 
			b[i] |= 128;
	}
}

int IECConnector::Connect() {

	m_port->Open(300, 300);

	std::ostringstream qs1;
	qs1 << "/?" << m_address << "!\r\n";
	std::string qs1s = qs1.str();
	std::vector<unsigned char> query(qs1s.length());
	std::copy(qs1s.begin(), qs1s.end(), query.begin());
	AddParityBit(query);

	m_port->WriteData(&query[0], query.size());

	unsigned char read_buffer[100];
	ssize_t read_pos = 0;

	do {
		if (!m_port->Wait(2))
			throw DeviceResponseTimeout();

		read_pos += m_port->GetData(read_buffer + read_pos, sizeof(read_buffer) - read_pos);

		if (read_pos == sizeof(read_buffer)) {
			m_port->Close();
			throw IECException("Response form meter too large");
		}

	} while (read_pos < 7 || (read_buffer[read_pos - 2] & 0x7f) != '\r' || (read_buffer[read_pos - 1] & 0x7f) != '\n');

	for (ssize_t i = 0; i < read_pos; i++)
		read_buffer[i] &= 0x7f;
	
	int speed;
	switch (read_buffer[4]) {
		case '0':
			speed = 300;
			break;
		case '1':
			speed = 600;
			break;
		case '2':
			speed = 1200;
			break;
		case '3':
			speed = 2400;
			break;
		case '4':
			speed = 4800;
			break;
		case '5':
			speed = 9600;
			break;
		case '6':
			speed = 19200;
			break;
		default:
			m_port->Close();
			throw IECException("Unsupported iec speed speficication");
	}

	std::ostringstream qs2;
	qs2 << "\0062" << char(read_buffer[4]) << "2\r\n";
	std::string qs2s = qs2.str();
	query.resize(qs2s.length());
	std::copy(qs2s.begin(), qs2s.end(), query.begin());
	AddParityBit(query);

	m_port->WriteData(&query[0], query.size());

//this is completly ugly workaround, we close the port 
//and flush the answer down the toilet, but the response
//we receive is usually either distorted or not present at all
//so we just continue assuming that meter has switched to proper speed 
//i have no idea why this happens, under MSW everything works
//like charm...
	sleep(1);
	m_port->Close();
	m_port->Open(speed, speed);

#if 0
	read_pos = 0;
	do {
		if (!m_port->Wait(2))
			throw DeviceResponseTimeout();

		read_pos += m_port->GetData(read_buffer + read_pos, sizeof(read_buffer) - read_pos);

	} while ((size_t)read_pos < query.size());


	//if (query != std::vector<unsigned char>(read_buffer, read_buffer + read_pos)) {
	if (query.size() != std::vector<unsigned char>(read_buffer, read_buffer + read_pos).size()) {
		dolog(0, "Invalid response received from meter  - %s", tohex(read_buffer, read_pos).c_str());
		throw IECException("Failed to enter HDLC mode");
	}
#endif

	return speed;

}

enum HDLCFrameType {
	I,
	RR,
	SNRM,
	DISC,
	UA,
	DM,
	FRMR,
	RNR,
	UI
};

struct HDLCFrame {
	unsigned m_format;
	bool m_segmentation_flag;
	unsigned m_length;
	unsigned m_src_address;
	unsigned m_dst_address;
	HDLCFrameType m_frame_type;
	bool m_poolfinal;
	unsigned m_ssn;
	unsigned m_rsn;
	Buffer m_content;
};

/**HDLC && LLC layer*/
class HDLCConnection {
	SerialPort *m_port;
	int m_speed;
	unsigned m_destination_address;
	int m_dest_addr_length;
	unsigned m_source_address;
	bool m_connected;
	unsigned m_unit_length;
	unsigned m_rsn;
	unsigned m_ssn;
	IECConnector *m_iec;
	unsigned int m_window_size;
	unsigned int m_frame_length;
	unsigned EncodeDestinationAddress(unsigned address);
	void CalculateChecksum(Buffer& b, unsigned short& fcs, unsigned short &hcs, unsigned short start_idx, unsigned short frame_length);
	unsigned DecodeDestinationAddress(unsigned address);
	Buffer StartHDLCFrame(HDLCFrameType ft, bool pool);
	Buffer ConstructConnectFrame();
	Buffer ReceiveFrame();
	void SendFrame(Buffer& b);
	Buffer GetResponseToFrame(Buffer &b);
	void CheckCRC(Buffer& b, unsigned short start_idx, unsigned short frame_lenth);
	void SkipLLCHeader(HDLCFrame &f);
	HDLCFrame ParseFrame(Buffer& b, bool skip_llc);
	void ParseConnectFrame(Buffer &b);
	unsigned short UpdateFrameLength(Buffer& b);
	/**Updates frame checksum*/
	void SetFrameChecksum(Buffer &b, unsigned short frame_length);
	/**ceates disconnec frame*/
	Buffer ConstructDisconnectFrame();
	void PutLLCHeader(Buffer& b);
public:
	HDLCConnection(SerialPort *port, int speed, int destination_address, int destination_address_length = 4, int source_address = 0x21, IECConnector* iec = NULL);
	bool IsConnected();
	void Connect();
	void Disconnect();
	size_t GetMaxContentLenght();
	Buffer SendRequest(const Buffer& b);

};

HDLCConnection::HDLCConnection(SerialPort *port, int speed, int destination_address, int destination_address_length, int source_address, IECConnector *iec) 
	: m_port(port), m_speed(speed), m_destination_address(destination_address), m_dest_addr_length(destination_address_length), m_source_address(source_address), m_connected(false), m_unit_length(0), m_iec(iec)
{}

bool HDLCConnection::IsConnected() {
	return m_connected;
}

unsigned HDLCConnection::EncodeDestinationAddress(unsigned address) {

	unsigned p0 = address & 0x7fu;
	unsigned p1 = (address & (0x7fu << 7));
	unsigned p2 = (address & (0x7fu << 14));
	unsigned p3 = (address & (0x7fu << 21));

	return (p3 << 4) | (p2 << 3) | (p1 << 2) | (p0 << 1) | 1;
}

void HDLCConnection::CalculateChecksum(Buffer& b, unsigned short& hcs, unsigned short &fcs, unsigned short start_idx, unsigned short frame_length) {

	static const unsigned short fcstab[256] = {
	      0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
	      0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
	      0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
	      0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
	      0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
	      0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
	      0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
	      0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
	      0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
	      0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
	      0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
	      0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
	      0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
	      0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
	      0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
	      0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
	      0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
	      0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
	      0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
	      0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
	      0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
	      0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
	      0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
	      0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
	      0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
	      0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
	      0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
	      0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
	      0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
	      0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
	      0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
	      0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
	};

	size_t hcsidx = 8 + start_idx;
	size_t start = start_idx;
	size_t fcsidx = start_idx + frame_length - 2;

	fcs = 0xffffu;

	for (size_t i = start; i < hcsidx; i++)
		fcs = (fcs >> 8) ^ fcstab[(fcs ^ b.at(i)) & 0xffu];

	hcs = fcs;
	hcs ^= 0xffff;
	
	fcs = (fcs >> 8) ^ fcstab[(fcs ^ (hcs & 0xff)) & 0xffu];
	fcs = (fcs >> 8) ^ fcstab[(fcs ^ (hcs >> 8)) & 0xffu];

	for (size_t i = hcsidx + 2; i < fcsidx; ++i)
		fcs = (fcs >> 8) ^ fcstab[(fcs ^ b.at(i)) & 0xffu];

	fcs ^= 0xffff;
	
}


unsigned HDLCConnection::DecodeDestinationAddress(unsigned address) {

	unsigned p0 = address & 0xfeu;
	unsigned p1 = address & (0xfeu << 8);
	unsigned p2 = address & (0xfeu << 16);
	unsigned p3 = address & (0xfeu << 24);

	return (p3 >> 4) | (p2 >> 3) | (p1 >> 2) | (p0 >> 1);
}

Buffer HDLCConnection::StartHDLCFrame(HDLCFrameType ft, bool pool) {
	Buffer b;

	//separator
	b.PutChar(0x7eu);

	//format type - 3
	b.PutChar(byte(1,0,1,0,0,0,0,0));
	b.PutChar(0);

	//destination address
	unsigned enc = EncodeDestinationAddress(m_destination_address);
	switch (m_dest_addr_length) {
		case 4:
			b.PutChar(enc >> 24);
		case 3:
			b.PutChar((enc >> 16) & 0xffu);
		case 2:
			b.PutChar((enc >> 8) & 0xffu);
		case 1:
			b.PutChar(enc & 0xffu);
	}

	//source adddress
	b.PutChar(m_source_address);

	unsigned char control = 0;

	switch (ft) {
		case I:
			control = (m_rsn << 5) | (m_ssn << 1);
			break;
		case RR:
			control = (m_rsn << 5) | 0x1u;
			break;
		case SNRM:
			control = (0x4u << 5) | 0x3u;
			break;
		case DISC:
			control = (0x2u << 5) | 0x3u;
			break;
		case UI:
			control = (0x2u << 5) | 0x3u;
			break;
		case DM:
		case UA:
		case FRMR:
		case RNR:
			dolog(0, "Not supported frame type");
			assert(false);
			break;
	}

	if (pool)
		control |= 0x10u;

	b.PutChar(control);

	//HCS, empty 
	b.PutChar(0u);
	b.PutChar(0u);

	return b;

}

Buffer HDLCConnection::ConstructConnectFrame() {
	/**see IEC 62056-46 6.4.4.3.2 (HDLC parameter negotation during the conneciton phase)
	 * for explanation of these*/
	Buffer b = StartHDLCFrame(SNRM, true);

	//format identifier
	b.PutChar(0x81u);

	//group identifier
	b.PutChar(0x80u);
	//group length - 18 octets
	b.PutChar(0x12u);

	//param identifier - maximum information field length, trasmit - 128b
	b.PutChar(0x05u);
	//value length
	b.PutChar(0x01u);
	//value
	b.PutChar(0x80u);

	//param identifier - maximum information field length, receive - 128b
	b.PutChar(0x06u);
	//value length
	b.PutChar(0x01u);
	//value
	b.PutChar(0x80u);

	//param identifier - window size, transmit - 1 packet
	b.PutChar(0x07u);
	//value length
	b.PutChar(0x04u);
	//value
	//b.PutChar(0x0); b.PutChar(0x0); b.PutChar(0x0); b.PutChar(0x7u);
	b.PutChar(0x0); b.PutChar(0x0); b.PutChar(0x0); b.PutChar(0x1u);

	//param identifier - window size, receive - 1 packet
	b.PutChar(0x08u);
	//value length
	b.PutChar(0x04u);
	//value
	//b.PutChar(0x0); b.PutChar(0x0); b.PutChar(0x0); b.PutChar(0x7u);
	b.PutChar(0x0); b.PutChar(0x0); b.PutChar(0x0); b.PutChar(0x1u);

	//FCS
	b.PutChar(0);
	b.PutChar(0);

	//terminator
	b.PutChar(0x7eu);

	unsigned short frame_length = UpdateFrameLength(b);
	SetFrameChecksum(b, frame_length);

	return b;
}

void HDLCConnection::Connect() {

	if (m_iec)
		m_speed = m_iec->Connect();

	m_port->Open(m_speed);

	Buffer b = ConstructConnectFrame();
	Buffer r = GetResponseToFrame(b);

	ParseConnectFrame(r);

	m_connected = true;

	m_rsn = m_ssn = 0;
}

void HDLCConnection::SendFrame(Buffer& b) {
	dolog(6, "Sending HDLC frame, %s", tohex(b.Content(), b.Size()).c_str());
	m_port->WriteData(b.Content(), b.Size());
}

void HDLCConnection::CheckCRC(Buffer& b, unsigned short start_idx, unsigned short frame_length) {
	unsigned short chcs, cfcs;

	CalculateChecksum(b, chcs, cfcs, start_idx, frame_length); 

	unsigned short fhcs, ffcs;

	fhcs = b.GetChar(start_idx + 9) << 8 | b.GetChar(start_idx + 8);
	ffcs = b.GetChar(start_idx + frame_length - 1) << 8 | b.GetChar(start_idx + frame_length - 2);

	if (fhcs != chcs || ffcs != cfcs) {
		dolog(1, "Invalid frame crc. Calculated hcs:%hx, frame hcs:%hx. Calculated fcs:%hx, frame fcs:%hx", 
			chcs, fhcs, cfcs, ffcs);
		throw InvalidHDLCPacketCRC();
	}
}

Buffer HDLCConnection::ReceiveFrame() {
	Buffer b;

	//read header
	b.Resize(11);
	ssize_t r = 0;
	ssize_t to_read = 9;
	while (r < to_read) {
		if (!m_port->Wait(5))
			throw DeviceResponseTimeout();

		r += m_port->GetData(b.Content() + r, b.Size() - r);

		while (b.Size() && b.at(0) == 0x7eu) {
			b.ReadChar();
			r--;
		}

	}

	//read content
	size_t length = (b.at(0) & 0x07u) << 8 | b.at(1);
	b.Resize(length);

	to_read = length;

	dolog(6, "Got frame header, frame length %zu", length);

	while (r < to_read) {
		if (!m_port->Wait(5))
			throw DeviceResponseTimeout();

		r += m_port->GetData(b.Content() + r, b.Size() - r);
	}

	dolog(6, "Got frame, %s", tohex(b.Content(), b.Size()).c_str());

	return b;
}

void HDLCConnection::SkipLLCHeader(HDLCFrame &f) {
	//skip src address
	f.m_content.ReadChar();
	//skip dst address
	f.m_content.ReadChar();
	//skip 'quality' field
	f.m_content.ReadChar();
}

HDLCFrame HDLCConnection::ParseFrame(Buffer& b, bool skip_llc) {

	HDLCFrame f;

	unsigned char b1, b2;
	b1 = b.at(0);
	b2 = b.at(1);
	
	f.m_format = b1 >> 4;
	f.m_segmentation_flag = b1 & 0x08;
	dolog(6, "Parsing HDLC frame: segmentation flag %s", f.m_segmentation_flag ? "present" : "absent");
	f.m_length = (b1 & 0x7u) << 8 | b2;
	dolog(6, "Parsing HDLC frame: length %u", f.m_length);

	CheckCRC(b, 0, f.m_length);
	b.ReadChar(); b.ReadChar();

	f.m_dst_address = b.ReadChar();
	dolog(6, "Parsing HDLC frame, src address %hhx", f.m_dst_address);

	f.m_src_address = DecodeDestinationAddress((b.ReadChar() << 24) | (b.ReadChar() << 16) | (b.ReadChar() << 8) | b.ReadChar());
	dolog(6, "Parsing HDLC frame, dst address %x", f.m_src_address);

	unsigned char control = b.ReadChar();
	f.m_poolfinal = control & (1 << 4);

	unsigned char u3 = control >> 5;
	unsigned char l3 = (control >> 1) & 0x7u;

	if ((control & 1) == 0) {
		f.m_frame_type = I;
		f.m_rsn = u3;
		f.m_ssn = l3;
		dolog(6, "Parsing HDLC frame, frame type I, rsn:%u , ssn:%u", (unsigned) f.m_rsn, (unsigned)f.m_ssn);
	} else if (l3 == 0) {
		f.m_frame_type = RR;
		f.m_rsn = u3;
		dolog(6, "Parsing HDLC frame, frame type RR, rsn:%u", (unsigned)f.m_rsn);
	} else if (l3 == 2) {
		f.m_frame_type = RNR;
		f.m_rsn = u3;
		dolog(6, "Parsing HDLC frame, frame type RNR, rsn:%u", (unsigned)f.m_rsn);
	} else if (l3 == 1) {
		if (u3 == 4) {
			f.m_frame_type = SNRM;
			dolog(6, "Parsing HDLC frame, frame type SNRM");
		} else if (u3 == 2) {
			f.m_frame_type = DISC;
			dolog(6, "Parsing HDLC frame, frame type DISC");
		} else if (u3 == 3) {
			f.m_frame_type = UA;
			dolog(6, "Parsing HDLC frame, frame type UA");
		} else {
			dolog(0, "Unrecognized frame type");
			throw InvalidHDLCFrame("Unrecognized frame type");
		}
	} else if (l3 == 7 && u3 == 0) {
		f.m_frame_type = DM;
		dolog(6, "Parsing HDLC frame, frame type DM");
	} else if (l3 == 3 && u3 == 4) {
		f.m_frame_type = FRMR;
		dolog(6, "Parsing HDLC frame, frame type FRMR");
	} else if (l3 == 3 && u3 == 8) {
		f.m_frame_type = UI;
		dolog(6, "Parsing HDLC frame, frame type UI");
	} else {
		dolog(0, "Unrecognized frame type");
		throw InvalidHDLCFrame("Unrecogized frame type");
	}

	//swallow hcs
	b.ReadShort();

	//swallow fcs
	b.Resize(b.Size() - 2);

	f.m_content = b;

	if (skip_llc)
		SkipLLCHeader(f);


	return f;
}

Buffer HDLCConnection::GetResponseToFrame(Buffer &b) {
	while (true) {
		SendFrame(b);
		try {
			Buffer r = ReceiveFrame();
			return r;
		} catch (InvalidHDLCPacketCRC) {
			//again
		}
	}
}

void HDLCConnection::ParseConnectFrame(Buffer &bf) {

	HDLCFrame f = ParseFrame(bf, false);

	if (f.m_frame_type != UA) {
		dolog(0, "Invalid frame type in response to SNRM frame");
		throw InvalidHDLCFrame("Not UA frame in response to SNRM frame");
	}

	Buffer &b = f.m_content;

	unsigned format_id = b.ReadTag();
	if (format_id != 0x81u) {
		dolog(0, "Invalid format id, expected %hhx got %hhx",  0x81u, format_id); 
		throw InvalidHDLCFrame("Invalid format type in HDLC UA frame");
	}

	unsigned group_id = b.ReadTag();
	if (group_id != 0x80u) {
		dolog(0, "Invalid group id, expected %hhx got %hhx",  0x80u, group_id); 
		throw InvalidHDLCFrame("Invalid group id in HDLC UA frame");
	}

	//skip length
	b.ReadChar();

	bool got_window_size = false;
	bool got_frame_length = false;

	try {

		do {
			unsigned char param_id = b.ReadChar();
			unsigned char param_val_length = b.ReadChar();
			unsigned param_val = 0;
			for (unsigned i = 0; i < param_val_length; i++)
				param_val = param_val << 8 | b.ReadChar();

			if (param_id == 0x06u) {
				got_frame_length = true;
				m_frame_length = param_val;
			} else if (param_id == 0x08u) {
				got_window_size = true;
				m_window_size = param_val;
			} 

		} while (got_window_size == false || got_frame_length == false);

	} catch (std::exception) {
		if (got_window_size == false) 
			dolog(0, "Invalid UA frame, missing window size");

		if (got_frame_length == false)
			dolog(0, "Invalid UA frame, missing maximum frame size");

		throw InvalidHDLCFrame("Invalid UA frame");
	}

	dolog(6, "Received window size %x", m_window_size);
	dolog(6, "Received max frame size %x", m_frame_length);
}

unsigned short HDLCConnection::UpdateFrameLength(Buffer& b) {
	//set frame length and 'or' higher byte of length with format type
	unsigned short frame_length = b.Size() - 2;
	b.at(2) = frame_length % 256;
	b.at(1) = b.at(1) | frame_length / 256;

	return frame_length;
}

void HDLCConnection::SetFrameChecksum(Buffer &b, unsigned short frame_length) {
	//set frame checksum
	unsigned short hcs, fcs;
	CalculateChecksum(b, hcs, fcs, 1, frame_length);
	b.SetChar(10, hcs >> 8);
	b.SetChar(9, hcs & 0xffu);

	if (b.Size() - 3 != 9) {
		b.SetChar(b.Size() - 2, fcs >> 8);
		b.SetChar(b.Size() - 3, fcs & 0xffu);
	}
}

Buffer HDLCConnection::ConstructDisconnectFrame() {
	Buffer b = StartHDLCFrame(DISC, true);

	//add terminator
	b.PutChar(0x7eu);

	unsigned short fl = UpdateFrameLength(b);
	SetFrameChecksum(b, fl);
	return b;
}


void HDLCConnection::Disconnect() {
	if (m_connected == false)
		return;

	Buffer s = ConstructDisconnectFrame();
	Buffer r = GetResponseToFrame(s);

	HDLCFrame f = ParseFrame(r, false);

	if (f.m_frame_type != UA 
			&& f.m_frame_type != DM)
		dolog(0, "Wrong packet received in response to disconnection attempt. Assuming connection as closed anyway");

	m_port->Close();

	m_connected = false;
}

void HDLCConnection::PutLLCHeader(Buffer& b) {
	b.PutChar(0xe6u);
	b.PutChar(0xe6u);
	b.PutChar(0);
}

Buffer HDLCConnection::SendRequest(const Buffer& b) {
	Buffer s = StartHDLCFrame(I, true);

	m_ssn = (m_ssn + 1) % 8;

	PutLLCHeader(s);

	s.PutData(b);

	//add fcs field
	s.PutChar(0);
	s.PutChar(0);

	//add terminator
	s.PutChar(0x7eu);

	bool finish = false;
	bool first_packet = true;

	Buffer r;

	while (!finish) {
		unsigned short frame_length = UpdateFrameLength(s);
		SetFrameChecksum(s, frame_length);
		Buffer rf = GetResponseToFrame(s);

		HDLCFrame f = ParseFrame(rf, first_packet);
		if (f.m_frame_type != I) {
			dolog(0, "Invalid HDLC frame type received in response to I frame");
			throw InvalidHDLCFrame("Not an I frame received to our frame");
		}

		m_rsn = (f.m_ssn + 1) % 8;

		r.PutData(f.m_content);

		if (f.m_segmentation_flag == true) {
			dolog(6, "Got frame with segmentation flag set. Issuing RR packet.");

			s = StartHDLCFrame(RR, true);

			//add terminator
			s.PutChar(0x7eu);
		} else
			finish = true;

		first_packet = false;
	}

	return r;
}

size_t HDLCConnection::GetMaxContentLenght() {
	return m_frame_length
		- 2	//frame terminator
		- 2	//HCS
		- 3	//LLC
		- 2	//FCS
		- 2	//length
		- 1	//control field
		- 5	//addresses
		;
}

#if 0
class OBIS {
	public:
	static std::string GetDescription(unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned char e, unsigned char f) {
		return "";
	}
};
#endif

struct Structure;
struct Vector;

struct Error {
	Error() {}
	Error(const std::string &s) : error(s) {}
	std::string error;
};

struct Null {};

struct Bitstring {
	std::vector<bool> v;
};


typedef boost::make_recursive_variant<
        std::string,
	unsigned char,
	unsigned short,
	unsigned int,
	unsigned long long,
	long long,
	short,
	float,
	int,
	Null,
	Error,
	Bitstring,
	boost::recursive_wrapper<Structure>,
	std::vector<boost::recursive_variant_> >::type Value; 

struct Structure {
	std::vector<Value> f;
};


class COSEMConnection {
	HDLCConnection *m_hdlc;
	bool m_connected;
	void ApplicationContextName(BerElement *ber);
	void DLMSInitiateRequest(BerElement *ber);
	void ParseAAREApplicationContextName(BerElement *ber);
	void ParseDLMSInitiateResponse(BerElement *ber);
	Buffer CreateAARQ();
	void ParseAARE(Buffer &b);
	bool CheckAAREResultValue(BerElement *ber);
	void GetAARESourceDiagnostics(BerElement *ber);
	Value ParseValue(Buffer &b);

	Value ParseDataAccessResult(Buffer &b);
	Value ParseReadResponseElement(Buffer& b);
	std::vector<Value> ParseResponse(Buffer &b);

	unsigned short m_max_pdu_size;
	unsigned short m_base_sn_name;

	size_t MaxFrameLength();
public:
	COSEMConnection(HDLCConnection *hdlc) : m_hdlc(hdlc) {}; 
	std::vector<Value> ReadRequest(const std::vector<unsigned short>& values);
	Value GetUnitParams();
	void Connect();
	void Query(Buffer &buffer);
	Buffer ReadResponse();
	void Disconnect();
};


void COSEMConnection::ApplicationContextName(BerElement *ber) {
	ber_start_seq(ber, 0xa1);

	//see docs for measning of this value
	unsigned char oid[] = { 0x60, 0x85, 0x74, 0x05, 0x08, 0x01, 
		//0x1 - logical name referencing, no ciphering
		//0x2 - short name referencing, no ciphering
		//0x3 - long name name referencing, ciphering
		//0x4 - short name name referencing, ciphering
		//and we choose short name refrencing, without ciphering
		0x2u };
	ber_printf(ber, "to", 0x06, oid, sizeof(oid));

	ber_put_seq(ber);
}

void COSEMConnection::DLMSInitiateRequest(BerElement *ber) {
	ber_start_seq(ber, 0xbeu); 

	//xDLMS cosem extension encoded in A-XDR
	Buffer b;
	b.PutChar(0x01);	//sequence tag
	b.PutChar(0x0); 	//no dedicated key component
	b.PutChar(0x0); 	//no response allowed compoent
	b.PutChar(0x0); 	//no proposed-quality-of-service
	b.PutChar(0x06);	//cosem protocol version - 6

	b.PutChar(0x5f);	//conformance block tag
	b.PutChar(0x4);		//length of value 
	b.PutChar(0x0);		//number of unused bits
	b.PutChar(byte(0, 0, 0, 1, 1, 1, 0, 0));		//the contents of the conformance block
	b.PutChar(byte(0, 0, 0, 0, 0, 0, 1, 1));		//reuest for READ, WRITE, UNCONFIRMED WRITE,  parametrized access and multiple references support
	b.PutChar(byte(0, 1, 0, 0, 0, 0, 1, 1));
	b.PutShort(0xffffu);

	ber_printf(ber, "o", b.Content(), b.Size());

	ber_put_seq(ber);

}

Buffer COSEMConnection::CreateAARQ() {

	BerElement *ber = ber_alloc_t(LBER_USE_DER);
	ber_start_seq(ber, 0x60);

	ApplicationContextName(ber);
	DLMSInitiateRequest(ber);

	ber_put_seq(ber);

	struct berval *bv;
	ber_flatten(ber, &bv);
	ber_free(ber, 1);

	Buffer r;
	r.Resize(bv->bv_len);

	std::copy((unsigned char*) bv->bv_val, (unsigned char*) bv->bv_val + bv->bv_len, r.Content());
	ber_bvfree(bv);

	return r;
}

void COSEMConnection::Connect() {
	m_hdlc->Connect();

	Buffer r = m_hdlc->SendRequest(CreateAARQ());
	ParseAARE(r);

	m_connected = true;
}

bool is_bit_set(unsigned char *c, unsigned bit, unsigned max_bits) {
	bool _set = false;
	if (bit < max_bits) {
		unsigned _byte = bit / 8;
		unsigned _bit = bit % 8;
		_set = c[_byte] & (1 << _bit);
	}
	return _set;
}

void COSEMConnection::ParseAAREApplicationContextName(BerElement *ber) {

	ber_tag_t tag;
	ber_scanf(ber, "T", &tag);
	if (tag != 0xa1) {
		dolog(0, "Invalid application context name tag in AARE. Received tag %u",(unsigned) tag);
		throw COSEMException("Invalid application context name tag content in AARE.");
	}

	Buffer b;
	b.Resize(8);
	ber_len_t cs = b.Size();

	tag = ber_get_stringb(ber, (char*) b.Content(), &cs);
	if (tag != 6) {
		throw COSEMException("Invalid application context tag in AARE, should be OID.");
	}

	//just check if short name referencing is used
	if (b.at(6) != 2) {
		dolog(0, "Unsupported referencing scheme in application context name, short name referencing expected");	
		throw COSEMException("Unsupported referencing scheme in application context name, short name referencing expected");	
	}

}

void COSEMConnection::ParseDLMSInitiateResponse(BerElement *ber) {

	ber_tag_t tag;
	ber_len_t cs;
	ber_scanf(ber, "l", &cs);
	ber_scanf(ber, "T", &tag);
	if (tag != 0xbe) {
		dolog(0, "Invalid user-information component tag in AARE. Received tag %u",(unsigned) tag);
		throw COSEMException("Invalid application context name tag content in AARE.");
	}

	Buffer b;
	b.Resize(cs);

	tag = ber_get_stringb(ber, (char*) b.Content(), &cs);
	if (tag != 4) {
		dolog(0, "Invalid xDLMS-Initiate.response component content in in AARE. Expected content: string");
		throw COSEMException("Invalid xDLMS-Initiate.response component content in in AARE. Expected content string");
	}

	//skip negotatied quality of service
	if (b.ReadChar())
		b.ReadChar();

	if (b.ReadChar() != 6) {
		dolog(0, "Unsupported version of COSEM protocol version, we support version 6.");
		throw COSEMException("Unsupported version of COSEM protocol version, we support version 6.");
	}

	//ok, lets get down to the conformance block
	//skip the tag
	if (b.ReadTag() != 0x5fu) {
		dolog(0, "Unrecognized conformance block tag.");
		throw COSEMException("Unrecognized conformance block tag.");
	}

	unsigned value_length = b.ReadLength();
	unsigned unused_bits = b.ReadChar();

	size_t bits = 8 * value_length - unused_bits;

#define IS_BIT_SET(_b)	\
		is_bit_set(b.Content(), _b, bits) 

	if (IS_BIT_SET(3))
		dolog(4, "Read service is supported");
	else {
		dolog(0, "Error: read service in not supported, unable to coninue");
		throw COSEMException("Error: read service is nt supported, unable co continue");
	}

	if (IS_BIT_SET(14))
		dolog(4, "Multiple references are supported");
	else
		dolog(4, "Multiple references are not supported");

	if (IS_BIT_SET(18))
		dolog(4, "Parametrized access is supported");
	else
		dolog(4, "Parametrized access is not supported");

	while (--value_length)
		b.ReadChar();

	m_max_pdu_size = b.ReadChar() << 8 | b.ReadChar();
	dolog(4, "Maximum pdu size is %hx", m_max_pdu_size);

	m_base_sn_name = b.ReadChar() << 8 | b.ReadChar();
	dolog(4, "Base name of association SN %hx", m_base_sn_name);

#undef IS_BIT_SET
}

bool COSEMConnection::CheckAAREResultValue(BerElement *ber) {
	ber_tag_t tag;

	ber_scanf(ber, "T", &tag);
	if (tag != 0xa2) {
		dolog(0, "AARE does not contatin result component.");
		throw COSEMException("AARE does not contain result component.");
	}

	ber_int_t result;
	ber_scanf(ber, "i", &result);

	if (result == 0) {
		dolog(4, "Ok, successful association");
		return true;
	}

	switch (result) {
		case 1:
			dolog(0, "AARQ rejected with code 'rejected-permanent'.");
			break;
		case 2:
			dolog(0, "AARQ rejected with code 'rejected-transient'.");
			break;
		default:
			dolog(0, "Not understood AARE response code -  %d.", result);
			throw COSEMException("Not understood AARE response code.");
			break;
	}
	return false;

}

void COSEMConnection::GetAARESourceDiagnostics(BerElement *ber) {
	ber_tag_t tag;

	ber_scanf(ber, "T", &tag);
	if (tag != 0xa3) {
		dolog(0, "AARE does not contatin source diagnostic component.");
		throw COSEMException("AARE does not contatin source diagnostic component.");
	}

	ber_scanf(ber, "T", &tag);
	if (tag == 0xa1) {
		dolog(4, "Got acse-service-user source, the diagnostics is:");
		
		ber_int_t asu;
		ber_scanf(ber, "i", &asu);

		switch (asu) {
			case 0:
				dolog(4, "Null diagnostics.");
				break;
			case 1:
				dolog(4, "No reason given.");
				break;
			case 2:
				dolog(4, "Application context name not supported.");
				break;
			case 11:
				dolog(4, "Application mechanism name not recognized.");
				break;
			case 12:
				dolog(4, "Authentication mechanism name required.");
				break;
			case 13:
				dolog(4, "Authentication failure");
				break;
			case 14:
				dolog(4, "Authentication required");
				break;
		}
	} else if (tag == 0xa2)  {
		dolog(4, "Got acse-service-provider source diagnostics, the diagnostics is:");
		
		ber_int_t asp;
		ber_scanf(ber, "i", &asp);

		switch (asp) {
			case 0:
				dolog(4, "Null diagnostics.");
				break;
			case 1:
				dolog(4, "No reason given.");
				break;
			case 2:
				dolog(4, "No common acse version.");
		}
	} else 
		assert(false);

}
	
void COSEMConnection::ParseAARE(Buffer &b) {
	struct berval bv;
	bv.bv_val = (char*) b.Content();
	bv.bv_len = b.Size();

	BerElement* ber = ber_init(&bv);

	try {

		ber_tag_t tag;
		ber_len_t len;

		ber_scanf(ber, "Tl", &tag, &len);

		if (tag != 0x61) {
			dolog(0, "AARE dos not start with proper tag, expexted 0x61 got %x", (unsigned) tag);
			throw COSEMException("AARE does start with proper tag");
		} else
			dolog(6, "Ok, got proper AARE tag");

		ParseAAREApplicationContextName(ber);

		bool connected = CheckAAREResultValue(ber);
		GetAARESourceDiagnostics(ber);

		if (connected == false)
			throw COSEMException("Unable to connect, see logs for details");

		ParseDLMSInitiateResponse(ber);

	} catch (std::exception&) {
		ber_free(ber, 1);
		throw;
	}

	ber_free(ber, 1);

}

void COSEMConnection::Disconnect() {
	m_hdlc->Disconnect();
}

class ValueParser {
	Value ParseNull(Buffer &b);
	Value ParseArray(Buffer &b);
	Value ParseSeqence(Buffer &b);
	Value ParseBoolean(Buffer &b);
	Value ParseBitString(Buffer &b);
	Value ParseDoubleLong(Buffer &b);
	Value ParseDoubleLongUnsigned(Buffer &b);
	Value ParseOctetString(Buffer &b);
	Value ParseVisibleString(Buffer &b);
	Value ParseBCD(Buffer &b);
	Value ParseInteger(Buffer &b);
	Value ParseLong(Buffer &b);
	Value ParseUnsigned(Buffer &b);
	Value ParseUnsignedLong(Buffer &b);
	Value ParseCompactArray(Buffer &b);
	Value ParseLongLong(Buffer &b);
	Value ParseUnsignedLongLong(Buffer &b);
	Value ParseEnum(Buffer &b);
	Value ParseFloat(Buffer &b);
	Value ParseDouble(Buffer &b);
	Value ParseDateTime(Buffer &b);
	Value ParseDate(Buffer &b);
	Value ParseTime(Buffer &b);
	Value ParseStructure(Buffer &b);
public:
	Value ParseValue(Buffer &b);
};

Value ValueParser::ParseNull(Buffer &b) {
	dolog(6, "Received type null");
	return Value(Null());
}

Value ValueParser::ParseArray(Buffer &b) {
	dolog(6, "Received type array");
	unsigned len = b.ReadLength();

	std::vector<Value> v;
	for (unsigned i = 0; i < len; ++i) {
		v.push_back(ParseValue(b));
	}
	dolog(6, "End of array values");

	return Value(v);
}

Value ValueParser::ParseStructure(Buffer &b) {
	dolog(6, "Received type structure");
	unsigned len = b.ReadLength();
	Structure s;
	for (size_t i = 0; i < len; ++i)
		s.f.push_back(ParseValue(b));

	dolog(6, "End of structure values");
	return Value(s);
}

Value ValueParser::ParseBoolean(Buffer &b) {
	dolog(6, "Received type boolean");
	bool r = b.ReadChar() != 0;
	return Value(r);
}

Value ValueParser::ParseBitString(Buffer &b) {
	dolog(6, "Received type bit string");

	unsigned len = b.ReadLength();
	unsigned char np = b.ReadChar();

	Bitstring bs;
	for (unsigned i = 0; i < len * 8 - np; ++i) {
		size_t byte = i / 8;
		size_t bi = i & 8;
		bs.v.push_back((b.at(byte) & (1 << bi)) != 0);
	}
	return Value(bs);
}

Value ValueParser::ParseDoubleLong(Buffer &b) {
	dolog(6, "Received type double long");
	return Value(b.ReadSInt());
}

Value ValueParser::ParseDoubleLongUnsigned(Buffer &b) {
	dolog(6, "Received type double long unsigned");
	return Value(b.ReadInt());
}

Value ValueParser::ParseOctetString(Buffer &b) {
	dolog(6, "Received type octet string");

	unsigned len = b.ReadLength();

	std::string v;
	for (unsigned i = 0; i < len; ++i) { 
		v += (char) b.ReadChar();
	}
	return Value(v);

}

Value ValueParser::ParseVisibleString(Buffer &b) {
	return ParseOctetString(b);
}

Value ValueParser::ParseBCD(Buffer &b) {
	b.ReadChar();
	return Error("Received not implemented type - BCD");
}

Value ValueParser::ParseLong(Buffer &b) {
	dolog(6, "Received type long");
	return b.ReadSShort();
}

Value ValueParser::ParseUnsignedLong(Buffer &b) {
	dolog(6, "Received type unsigned long");
	return b.ReadShort();
}

Value ValueParser::ParseCompactArray(Buffer &b) {
	dolog(0, "Received not implemented type - compact array");
	throw COSEMException("Received not implemented type - compact array");
}

Value ValueParser::ParseLongLong(Buffer &b) {
	dolog(6, "Received type long long");
	return Value(b.ReadSLongLong());
}

Value ValueParser::ParseUnsignedLongLong(Buffer &b) {
	dolog(6, "Received type unsigned long long");
	return Value(b.ReadLongLong());
}

Value ValueParser::ParseEnum(Buffer &b) {
	dolog(6, "Received type enum");
	return b.ReadChar();
}

Value ValueParser::ParseFloat(Buffer &b) {
	dolog(6, "Received type float");
	return Value(b.ReadFloat());
}

Value ValueParser::ParseDouble(Buffer &b) {
	b.ReadFloat(); b.ReadFloat();
	return Error("Received not implemented type - double");
}

Value ValueParser::ParseDateTime(Buffer &b) {
	dolog(0, "Received not implemented type - datetime");
	throw COSEMException("Received not implemented type - datetime");
}

Value ValueParser::ParseDate(Buffer &b) {
	dolog(0, "Received not implemented type - byte");
	throw COSEMException("Received not implemented type - byte");
}

Value ValueParser::ParseTime(Buffer &b) {
	dolog(0, "Received not implemented type - time");
	throw COSEMException("Received not implemented type - time");
}

Value ValueParser::ParseSeqence(Buffer &b) {
	return ParseArray(b);
}

Value ValueParser::ParseInteger(Buffer &b) {
	dolog(6, "Received type integer");
	return Value(b.ReadChar());
}

Value ValueParser::ParseUnsigned(Buffer &b) {
	dolog(6, "Received type unsigned");
	return Value(b.ReadChar());
}

Value ValueParser::ParseValue(Buffer& b) {
	unsigned char type = b.ReadChar();
	switch (type) {
		case 0: 
			return ParseNull(b);
		case 1: 
			return ParseArray(b);
		case 2:
			return ParseStructure(b);
		case 3:
			return ParseBoolean(b);
		case 4:
			return ParseBitString(b);
		case 5:
			return ParseDoubleLong(b);
		case 6:
			return ParseDoubleLongUnsigned(b);
		case 9:
			return ParseOctetString(b);
		case 10:
			return ParseVisibleString(b);
		case 13:
			return ParseBCD(b);
		case 15:
			return ParseInteger(b);
		case 16:
			return ParseLong(b);
		case 17:
			return ParseUnsigned(b);
		case 18:
			return ParseUnsignedLong(b);
		case 19:
			return ParseCompactArray(b);
		case 20:
			return ParseLongLong(b);
		case 21:
			return ParseUnsignedLongLong(b);
		case 22:
			return ParseEnum(b);
		case 23:
			return ParseFloat(b);
		case 24:
			return ParseDouble(b);
		case 25:
			return ParseDateTime(b);
		case 26:
			return ParseDate(b);
		case 27:
			return ParseTime(b);
		default:
			dolog(0, "Unknown type %hhx in read response", type);
			throw COSEMException("Unknown type in read response");
			assert(false);
	}


};

Value COSEMConnection::ParseValue(Buffer &b) {
	ValueParser vp;
	return vp.ParseValue(b);
}

Value COSEMConnection::ParseDataAccessResult(Buffer &b) {
	unsigned char c = b.ReadChar();

	Error e;

	switch (c) {
		case 0:
			e.error = "Success";
			break;
		case 1:
			e.error = "Hardware fault";
			break;
		case 2:
			e.error = "Temporary failure";
			break;
		case 3:
			e.error = "Read-Write Denied";
			break;
		case 4:
			e.error = "Object undefined";
			break;
		case 9:
			e.error = "Object class inconsitent";
			break;
		case 11:
			e.error = "Object unavialable";
			break;
		case 12:
			e.error = "Type unmatched";
			break;
		case 13:
			e.error = "Scope of access violated";
			break;
		case 14:
			e.error = "Data block unavailable";
			break;
		case 15:
			e.error = "Long get aborted";
			break;
		case 16:
			e.error = "No long get in progress";
			break;
		case 17:
			e.error = "Long get aborted";
			break;
		case 18:
			e.error = "No long set in progress";
			break;
	}

	return Value(e);

}
			
Value COSEMConnection::ParseReadResponseElement(Buffer& b) {
	unsigned char choice = b.ReadChar();

	switch (choice) {
		case 0:
			dolog(6, "Got data element");
			return ParseValue(b);
		case 1:
			dolog(6, "Got data access result element");
			return ParseDataAccessResult(b);
		default:
			dolog(0, "Unsupported choice type of response element, %hhx", choice);
			throw COSEMException("Unspported choice vale is response element");
	}
}

std::vector<Value> COSEMConnection::ParseResponse(Buffer &b) {
	unsigned char tag = b.ReadChar();
	unsigned char len = b.ReadChar();

	if (tag != 0x0c) {
		dolog(0, "Invalid tag received in response to read request");
		throw COSEMException("Invalid tag received in response to read request");
	}

	dolog(10, "Number of elements in response: %u", len);

	std::vector<Value> v;
	for (unsigned i = 0; i < len; ++i) {
		v.push_back(ParseReadResponseElement(b));
	}

	return v;

}

std::vector<Value> COSEMConnection::ReadRequest(const std::vector<unsigned short>& values) {
	std::vector<Value> v;

	size_t sent = 0;
	while (sent < values.size()) {
		size_t to_send = std::min((MaxFrameLength() - 2) / 3, values.size() - sent);

		Buffer b;

		b.PutChar(0x5u); //read request tag
		b.PutChar(to_send);

		for (size_t i = sent; i < sent + to_send; ++i) { 
			b.PutChar(0x2);	//short
			b.PutShort(values[i]);
		}

		Buffer t = m_hdlc->SendRequest(b);
		std::vector<Value> rv = ParseResponse(t);

		for (size_t i = 0; i < rv.size(); i++)
			v.push_back(rv[i]);

		sent += to_send;
	}

	return v;
}

Value COSEMConnection::GetUnitParams() {
	std::vector<unsigned short> us;
	us.push_back(m_base_sn_name + 8);
	return ReadRequest(us).at(0);
}

size_t COSEMConnection::MaxFrameLength() {
	return std::min(size_t(m_max_pdu_size), m_hdlc->GetMaxContentLenght());
}


struct OBISObject {
	enum CID {
		DATA = 0,
		REGISTER,
		EXTENDED_REGISTER,
		DEMAND_REGISTER,
		LAST_CID,
	} class_id;

	enum ATTRIBUTE {
		VALUE = 0,
		STATUS,		
		CURRENT_AVERAGE_VALUE,
		LAST_AVERAGE_VALUE,
		SCALER_UNIT,
		LAST_ATTRIBUTE
	};

	struct Value {
		Value() : prec(0), msw(-1), lsw(-1) {}
		int prec;
		int msw;
		int lsw;
	};

	std::map<ATTRIBUTE, Value> values;
};

const int ShortAttributeAddress[OBISObject::LAST_CID][OBISObject::LAST_ATTRIBUTE] =  {
	{ 8, -1, -1, -1, -1, },
	{ 8, -1, -1, -1, 16, },
	{ 8, 24, -1, -1, 16, },
	{  -1, 32, 8, 16, 24, }
};

class COSEMUnit {
	class ParamValueConverter : public boost::static_visitor<> {
		short *m_params;
		const std::map<unsigned short, OBISObject>& m_map;
		std::map<unsigned short, OBISObject>::const_iterator m_current_object;
		std::map<OBISObject::ATTRIBUTE, OBISObject::Value>::const_iterator m_current_attribute;
	public:
		ParamValueConverter(short *params, const std::map<unsigned short, OBISObject>& map) : m_params(params), m_map(map) {}

		template<typename T> void operator() (T &op);
		void InsertValues(std::vector<Value>& vals);

	};

	short* m_params;
	unsigned m_address;
	std::map<unsigned short, OBISObject> m_map;

public:
	COSEMConnection *m_cosem;
	void DumpParamsUnits();
	void QueryUnit();
	bool Configure(TUnit *unit, xmlNodePtr xmlu, short *params, SerialPort *port, int speed);
};

void COSEMUnit::ParamValueConverter::InsertValues(std::vector<Value>& vals) {
	m_current_object = m_map.begin();
	if (m_current_object != m_map.end())
		m_current_attribute = m_current_object->second.values.begin();

	for (std::vector<Value>::iterator i = vals.begin();
			i != vals.end() && m_current_object != m_map.end();
			i++) {
		boost::apply_visitor(*this, *i);

		++m_current_attribute;
		while (m_current_attribute == m_current_object->second.values.end()) {
			if (++m_current_object != m_map.end())
				m_current_attribute = m_current_object->second.values.begin();
		}
	}
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

template<typename T> void COSEMUnit::ParamValueConverter::operator() (T &op) {
	dolog(0, "Received type %s for value of param %hx, attribute %d. This data type is not supported. Setting no data.", 
			typeid(T).name(), m_current_object->first, m_current_attribute->first);

	const OBISObject::Value& i = m_current_attribute->second; 

	if (i.msw != -1)
		m_params[i.msw] = SZARP_NO_DATA;
	if (i.lsw != -1)
		m_params[i.lsw] = SZARP_NO_DATA;
}

template<> void COSEMUnit::ParamValueConverter::operator() (Error &op) {
	dolog(0, "Error while reading data: %s for value of param %hx, attribute %d. Setting no data",
			op.error.c_str(), m_current_object->first, m_current_attribute->first);

	const OBISObject::Value& i = m_current_attribute->second; 

	if (i.msw != -1)
		m_params[i.msw] = SZARP_NO_DATA;
	if (i.lsw != -1)
		m_params[i.lsw] = SZARP_NO_DATA;
}

template<> void COSEMUnit::ParamValueConverter::operator() (short &val) {
	const OBISObject::Value& i = m_current_attribute->second; 

	if (i.prec != 0)
		dolog(0, "Value of param %hx attribute %d, has short type, precision ignored", m_current_object->first, m_current_attribute->first);

	if (i.lsw != -1)
		m_params[i.lsw] = val;
	else
		dolog(0, "Value of param %hx attribute %d, has short type, and no lsw specified, no data set for param", m_current_object->first, m_current_attribute->first);

	if (i.msw != -1) {
		m_params[i.msw] = 0;
		dolog(0, "Value of param %hx attribute %d has short type, msw set to zero", m_current_object->first, m_current_attribute->first);
	}
	
	dolog(6, "Got short type for param %hx attribute %d, value: %hd", m_current_object->first, m_current_attribute->first, val);

}

template<> void COSEMUnit::ParamValueConverter::operator() (unsigned short &val) {
	short sv = (short) val;
	this->operator()(sv);
}

template<> void COSEMUnit::ParamValueConverter::operator() (float &val) {
	const OBISObject::Value& i = m_current_attribute->second; 

	float fval = mypow(val, i.prec);
	int ival = int(fval);

	short lsw = ival & 0xffffu;
	short msw = ival >> 16;

	if (i.lsw != -1)
		m_params[i.lsw] = lsw;
	else
		dolog(0, "Value of param %hx attribute %d has float type, and no lsw parcook param specified", m_current_object->first, m_current_attribute->first);

	if (i.msw != -1)
		m_params[i.msw] = msw;
	else
		dolog(0, "Value of param %hx attribute %d has float type, and no msw parcook param defined", m_current_object->first, m_current_attribute->first);

	dolog(6, "Got float type for param %hx attribute %d, value: %f", m_current_object->first, m_current_attribute->first, (double)fval);
}

template<> void COSEMUnit::ParamValueConverter::operator() (unsigned long long &val) {
	const OBISObject::Value& i = m_current_attribute->second; 

	long long mval = mypow(val, i.prec);
	int ival = int(mval);

	short lsw = ival & 0xffffu;
	short msw = ival >> 16;

	if (i.lsw != -1)
		m_params[i.lsw] = lsw;
	else
		dolog(0, "Value of param %hx attribute %d has long long type, and no lsw parcook param specified", m_current_object->first, m_current_attribute->first);

	if (i.msw != -1)
		m_params[i.msw] = msw;
	else
		dolog(0, "Value of param %hx attribute %d has long long type, and no msw parcook param defined", m_current_object->first, m_current_attribute->first);

	dolog(6, "Got long long type for param %hx attribute %d, value: %llu", m_current_object->first, m_current_attribute->first, val);
}

template<> void COSEMUnit::ParamValueConverter::operator() (long long &val) {
	unsigned long long ul = (unsigned long long) val;
	this->operator()(ul);
}

template<> void COSEMUnit::ParamValueConverter::operator() (int &val) {
	const OBISObject::Value& i = m_current_attribute->second; 

	int ival = mypow(val, i.prec);

	short lsw = ival & 0xffffu;
	short msw = ival >> 16;

	if (i.lsw != -1) {
		if (ival < 0 && i.msw == -1)
			lsw = lsw | (1 << 15);

		m_params[i.lsw] = lsw;
	} else
		dolog(0, "Value of param %hx attribute %d has int type, and no lsw parcook param defined", m_current_object->first, m_current_attribute->first);

	if (i.msw != -1)
		m_params[i.msw] = msw;
	else
		dolog(4, "Value of param %hx attribute %d has int type, and no msw parcook param defined", m_current_object->first, m_current_attribute->first);

	dolog(6, "Got int type for param %hx attribute %d, value: %d", m_current_object->first, m_current_attribute->first, ival);

}

template<> void COSEMUnit::ParamValueConverter::operator() (unsigned int &val) {
	int ul = (int) val;
	this->operator()(ul);
}

bool COSEMUnit::Configure(TUnit *unit, xmlNodePtr xunit, short *params, SerialPort *port, int speed) {
	char *endptr;

	m_params = params;

	bool use_iec = false;
	xmlChar* _use_iec = xmlGetNsProp(xunit, BAD_CAST "use_iec", BAD_CAST IPKEXTRA_NAMESPACE_STRING);
	if (_use_iec && !xmlStrcmp(_use_iec, BAD_CAST "true"))
		use_iec = true;
	xmlFree(_use_iec);


	IECConnector *iec = NULL;
	if (use_iec) {
		xmlChar* _iec_address = xmlGetNsProp(xunit, BAD_CAST "iec_address", BAD_CAST IPKEXTRA_NAMESPACE_STRING);
		if (_iec_address == NULL) {
			dolog(0, "Attribbute dlms:iec_address not present in unit element, line no (%ld)", xmlGetLineNo(xunit)); 
			return false;
		}

		iec = new IECConnector(port, (const char*) _iec_address);
		xmlFree(_iec_address);
	}

	xmlChar* _addr = xmlGetNsProp(xunit, BAD_CAST("address"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (_addr == NULL) {
		dolog(0, "Attribbute dlms:address not present in unit element, line no (%ld)", xmlGetLineNo(xunit)); 
		return false;
	}

	unsigned address = strtoul((char*)_addr, &endptr, 0);
	if (*endptr != 0) {
		dolog(0, "Error, invalid value of parameter dlms:sn in param definition, line(%ld)", xmlGetLineNo(xunit));
		return false;
	}
	xmlFree(_addr);

	xmlXPathContextPtr xp_ctx = xmlXPathNewContext(xunit->doc);
	xp_ctx->node = xunit;

	int ret;
	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "ipk", SC::S2U(IPK_NAMESPACE_STRING).c_str());
	assert(ret == 0);
#if 0
	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "dlms", BAD_CAST IPKEXTRA_NAMESPACE_STRING);
	assert (ret == 0);
#endif

	xmlXPathObjectPtr rset = xmlXPathEvalExpression(BAD_CAST "./ipk:param", xp_ctx);

	for (int i = 0; i < rset->nodesetval->nodeNr; ++i) {
		xmlNodePtr n = rset->nodesetval->nodeTab[i];
		xmlChar* _sn = xmlGetNsProp(n,
				BAD_CAST("sn"), 
				BAD_CAST(IPKEXTRA_NAMESPACE_STRING));

		if (_sn == 0) {
			dolog(0, "Error, attribute dlms:sn missing in param definition, line(%ld)", xmlGetLineNo(n));
			return false;
		}

		unsigned short sn = strtoul((char*)_sn, &endptr, 0);
		if (*endptr != 0) {
			dolog(0, "Error, invalid value of parameter dlms:sn in param definition, line(%ld)", xmlGetLineNo(n));
			return false;
		}
		xmlFree(_sn);

		xmlChar* _attribute = xmlGetNsProp(n,
				BAD_CAST("attribute"), 
				BAD_CAST(IPKEXTRA_NAMESPACE_STRING));

		if (_attribute == 0) {
			dolog(0, "Error, missing value of dlms:attribute in param definition, line(%ld)", xmlGetLineNo(n));
			return false;
		}
		OBISObject::ATTRIBUTE attribute;
		if (!xmlStrcmp(_attribute, BAD_CAST "value"))
			attribute = OBISObject::VALUE;
		else if (!xmlStrcmp(_attribute, BAD_CAST "status"))
			attribute = OBISObject::STATUS;
		else if (!xmlStrcmp(_attribute, BAD_CAST "current_average_value"))
			attribute = OBISObject::CURRENT_AVERAGE_VALUE;
		else if (!xmlStrcmp(_attribute, BAD_CAST "last_average_value"))
			attribute = OBISObject::LAST_AVERAGE_VALUE;
		else {
			dolog(0, "Error, unsupported attribute value (%s), line(%ld)", (char*) _attribute, xmlGetLineNo(n));
			xmlFree(_attribute);
			return false;
		}
		xmlFree(_attribute);
		
	       	int prec = 0;
		xmlChar* _prec = xmlGetNsProp(n,
				BAD_CAST("prec"), 
				BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
		if (_prec != NULL) {
			prec = strtol((char*)_prec, &endptr, 0);
			if (*endptr != 0) {
				dolog(0, "Error, invalid value of parameter dlms:prec in param definition, line(%ld)", xmlGetLineNo(n));
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


		if (m_map.find(sn) != m_map.end()) {

			if (m_map[sn].values.find(attribute) == m_map[sn].values.end() || prec != 0)
				m_map[sn].values[attribute].prec = prec;

			OBISObject::CID cid = m_map[sn].class_id;
			if (ShortAttributeAddress[cid][attribute] == -1) {
				dolog(0, "Error, unsupported attribute (%d) for class %d, line(%ld)", attribute, cid, xmlGetLineNo(n));
				return false;
			}

			if (is_msw) 
				m_map[sn].values[attribute].msw = i;
			else
				m_map[sn].values[attribute].lsw = i;

		} else {
			xmlChar* _class_type = xmlGetNsProp(n,
					BAD_CAST("class_type"), 
					BAD_CAST(IPKEXTRA_NAMESPACE_STRING));

			if (_class_type == 0) {
				dolog(0, "Error, attribute dlms:class_type missing in param definition, line(%ld)", xmlGetLineNo(n));
				return false;
			}

			OBISObject nv; 

			if (!xmlStrcmp(_class_type, BAD_CAST "data"))
				nv.class_id = OBISObject::DATA;
			else if (!xmlStrcmp(_class_type, BAD_CAST "register"))
				nv.class_id = OBISObject::REGISTER;
			else if (!xmlStrcmp(_class_type, BAD_CAST "extended_register"))
				nv.class_id = OBISObject::EXTENDED_REGISTER;
			else if (!xmlStrcmp(_class_type, BAD_CAST "demand_register"))
				nv.class_id = OBISObject::DEMAND_REGISTER;
			else {
				dolog(0, "Error, unsupported class (%s), line(%ld)", (char*) _class_type, xmlGetLineNo(n));
				return false;
			}

			if (ShortAttributeAddress[nv.class_id][attribute] == -1) {
				dolog(0, "Error, unsupported attribute (%d) for class %s, line(%ld)", attribute, (char*)_class_type, xmlGetLineNo(n));
				return false;
			}

			xmlFree(_class_type);

			m_map[sn] = nv;

			m_map[sn].values[attribute].prec = prec;
			if (is_msw) 
				m_map[sn].values[attribute].msw = i;
			else
				m_map[sn].values[attribute].lsw = i;

		}

	}

	xmlXPathFreeObject(rset);
	xmlXPathFreeContext(xp_ctx);

	HDLCConnection *hdlc = new HDLCConnection(port, speed, address, 4, 0x21, iec);
	m_cosem = new COSEMConnection(hdlc);

	return true;

}

void COSEMUnit::QueryUnit() {
	std::vector<unsigned short> addresses;

	for (std::map<unsigned short, OBISObject>::iterator i = m_map.begin();
			i != m_map.end();
			i++) {
		const unsigned short& base_addr = i->first;
		OBISObject& oo = i->second;

		for (std::map<OBISObject::ATTRIBUTE, OBISObject::Value>::iterator j = oo.values.begin();
				j != oo.values.end();
				j++) {
			int shift = ShortAttributeAddress[oo.class_id][j->first];
			if (shift == -1) {
				dolog(1, "Invalid object, attriute combinantion for object %hu, attribute %d not present", base_addr, j->first);
				assert(false);
			} else
				addresses.push_back(i->first + shift);
		}
	}
	
	std::vector<Value> vals;
	try {
		m_cosem->Connect();
		vals = m_cosem->ReadRequest(addresses);
	} catch (std::exception&) {
		m_cosem->Disconnect();
		throw;
	}
	m_cosem->Disconnect();

	ParamValueConverter pvc(m_params, m_map);
	pvc.InsertValues(vals);
}

struct UnitName {
	unsigned char id;
	const char *name;
	const char *description;
};

const UnitName unit_names[] = {
	{ 1, "a", "year" },
	{ 2, "mo", "month" },
	{ 3, "wk", "week" },
	{ 4, "d", "day" },
	{ 5, "h", "hour" },
	{ 6, "min.", "minute" },
	{ 7, "s", "second" },
	{ 8, "o", "degree" },
	{ 9, "oC", "temperature" }, 
	{ 10, "currency", "currency" }, 
	{ 11, "m", "length" }, 
	{ 12, "m/s", "speed" }, 
	{ 13, "m3", "volume" }, 
	{ 14, "m3", "corrected volume" }, 
	{ 15, "m3/h", "volume flux" }, 
	{ 16, "m3/h", "corrected volume flux" }, 
	{ 17, "m3/d", "volume flux" }, 
	{ 18, "m3/d", "corrected volume flux" }, 
	{ 19, "l", "volume(litre)" }, 
	{ 20, "kg", "mass" },
	{ 21, "N", "force" },
	{ 22, "Nm", "energy" },
	{ 23, "Pa", "pressure" },
	{ 24, "bar", "pressure" },
	{ 25, "J", "energy newtonmeter" },
	{ 26, "J/h", "thermal power" },
	{ 27, "W", "active power watt" },
	{ 28, "VA", "apparent power volt-ampere" },
	{ 29, "var", "reactive power" },
	{ 30, "Wh", "active energy watt-hour" },
	{ 31, "VAh", "apparent energy watt-hour" },
	{ 32, "varh", "rective energy var-hour" },
	{ 33, "A", "current" },
	{ 34, "C", "electrical charge (Coulomb)" },
	{ 35, "V", "voltage" },
	{ 36, "V/m", "electric field strength" },
	{ 37, "F", "capacitance" },
	{ 38, "ohm", "resistance" },
	{ 39, "ohm*m2/m", "resisitivity" },
	{ 40, "Wb", "magnetic flux" },
	{ 41, "T", "magnetic flux density" },
	{ 42, "A/m", "magnetic field strength" },
	{ 43, "H", "inductane (Henry)" },
	{ 44, "Hz", "frequency" },
	{ 45, "1/(Wh)", "active enery constant or pulse value" },
	{ 46, "1/(varh)", "reactive enery constant or pulse value" },
	{ 47, "1/(VAh)", "apparent enery constant or pulse value" },
	{ 48, "V2h", "volt squared hour" },
	{ 49, "A2h", "ampere squared hour" },
	{ 50, "kg/s", "mass flux" },
	{ 51, "S", "conductance" },
	{ 52, "K", "temperature" }
};


void COSEMUnit::DumpParamsUnits() {
	std::vector<unsigned short> attr_addrs, objs_addrs;
	for (std::map<unsigned short, OBISObject>::iterator i = m_map.begin();
			i != m_map.end();
			i++) {
		int sn = ShortAttributeAddress[i->second.class_id][OBISObject::SCALER_UNIT];
		if (sn == -1)
			continue;
		attr_addrs.push_back(i->first + sn);
		objs_addrs.push_back(i->first);
	}
	
	std::vector<Value> vals;
	try {
		m_cosem->Connect();
		vals = m_cosem->ReadRequest(attr_addrs);
	} catch (std::exception&) {
		m_cosem->Disconnect();
		throw;
	}
	m_cosem->Disconnect();

	assert(vals.size() == attr_addrs.size());

	for (size_t i = 0; i < vals.size(); ++i) {

		try {
			std::cout << "Param address: " << std::hex << std::setw(4) << objs_addrs[i] << " ";

			Structure& s = boost::get<Structure>(vals[i]);

			std::cout << "Scaler:" << unsigned(boost::get<unsigned char>(s.f.at(0))) << " ";
			unsigned char id = boost::get<unsigned char>(s.f.at(1));

				bool found = false;
			for (size_t i = 0; i < sizeof(unit_names) / sizeof(UnitName); ++i) {
				if (unit_names[i].id != id)
					continue;
	
				found = true;
	
				std::cout << unit_names[i].name << " ";
				std::cout << unit_names[i].description << std::endl;
	
				break;
			}

			if (!found)
				std::cout << "Unit info(id:" << unsigned(id) << ") not found, consult ch.5 of IEC-62056-62 for details" << std::endl;

		} catch (boost::bad_get) {
			std::cout << "Skipping param" << std::endl;
		}

	}

}

class COSEMDaemon { 
	std::vector<COSEMUnit*> m_units;
	std::vector<std::pair<int, int> > m_inactivity_periods;
	IPCHandler *m_ipc;
	SerialPort *m_port;

	bool ShallWait();
	bool ParseInactivityPeriods(xmlNodePtr node);
public:
	void DumpParamsUnits();
	bool Configure(DaemonConfig* cfg);
	void Start();
};

bool COSEMDaemon::ShallWait() {
	time_t t = time(NULL);
	struct tm* _tm = localtime(&t);
	int m = _tm->tm_hour * 60 + _tm->tm_min;

	for (std::vector<std::pair<int, int> >::iterator i = m_inactivity_periods.begin(); 
			i != m_inactivity_periods.end(); 
			++i) {
		int &ms = i->first;
		int &ml = i->second;

		if (m > ms && m < ms + ml)
			return true;
	}

	return false;
}

bool COSEMDaemon::ParseInactivityPeriods(xmlNodePtr node) {
	xmlChar* _ip = xmlGetNsProp(node, BAD_CAST("inactive"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (_ip == NULL)
		return true;

	bool ret = true;
	char **toks;
	int tokc = 0;

	tokenize_d((char*) _ip, &toks, &tokc, ";");
	for (int i = 0; i < tokc; i++) {
		int h, m, l;
		if (sscanf(toks[i], "%d:%d %d", &h, &m, &l) == 3) { 
			m_inactivity_periods.push_back(std::make_pair(h * 60 + m, l));	
		} else {
			dolog(0, "Invalid format of inactivity period");
			ret = false;
			break;
		}
	}
	tokenize_d(NULL, &toks, &tokc, ";");

	xmlFree(_ip);
	return ret;
}


bool COSEMDaemon::Configure(DaemonConfig *cfg) {
	m_ipc = new IPCHandler(cfg);
	if (m_ipc->Init()) {
		dolog(0, "Intialization of IPCHandler failed");
		return false;
	}

	m_port = new SerialPort(cfg->GetDevicePath());

	TDevice* dev = cfg->GetDevice();
	xmlNodePtr xdev = cfg->GetXMLDevice();
	if (ParseInactivityPeriods(xdev) == false)
		return false;

	xmlXPathContextPtr xp_ctx = xmlXPathNewContext(xdev->doc);
	xp_ctx->node = xdev;

	int ret;
	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "ipk", SC::S2U(IPK_NAMESPACE_STRING).c_str());
	assert(ret == 0);

	xmlXPathObjectPtr rset = xmlXPathEvalExpression(BAD_CAST ".//ipk:unit", xp_ctx);

	short* reads = m_ipc->m_read;

	int i = 0;
	for (TUnit *unit = dev->GetFirstRadio()->GetFirstUnit(); unit; unit = unit->GetNext(), i++) {
		assert(i < rset->nodesetval->nodeNr);

		COSEMUnit* u  = new COSEMUnit();

		if (u->Configure(unit, rset->nodesetval->nodeTab[i], reads, m_port, cfg->GetSpeed()) == false)
			return false;

		m_units.push_back(u);

		reads += unit->GetParamsCount();
	}

	return true;

}

void COSEMDaemon::Start() {

	while (true) {
		dolog(6, "Beginning parametr fetching loop");

		for (int i = 0; i < m_ipc->m_params_count; ++i) 
			m_ipc->m_read[i] = SZARP_NO_DATA;

		time_t st = time(NULL);

		if (ShallWait() == false) 
			for (std::vector<COSEMUnit*>::iterator i = m_units.begin();
					i != m_units.end();
					++i) {
				try {
					(*i)->QueryUnit();
				} catch (std::exception &e) {
					dolog(0, "Exception occured while quering %s unit", e.what());
				}
			}

		m_ipc->GoParcook();

		int s = st + 10 - time(NULL);
		while (s > 0) 
			s = sleep(s);

		dolog(6, "End of parametrs' fetching loop");
	}
}

void COSEMDaemon::DumpParamsUnits() {
	for (std::vector<COSEMUnit*>::iterator i = m_units.begin();
			i != m_units.end();
			++i) {
		try {
			std::cout << "Units of params:" << std::endl;
			(*i)->DumpParamsUnits();
			std::cout << std::endl;
		} catch (std::exception &e) {
			dolog(0, "Exception occured while quering unit: %s", e.what());
		}
	}
}

void print_unit_params(Value &v) {
	try {
		std::vector<Value>& ss = boost::get<std::vector<Value> >(v);
		for (std::vector<Value>::iterator i = ss.begin();
				i != ss.end();
				i++) {
			Structure& s = boost::get<Structure>(*i);

			short base_addr = boost::get<short>(s.f.at(0));
			unsigned short _class = boost::get<unsigned short>(s.f.at(1));
			unsigned char id = boost::get<unsigned char>(s.f.at(2));

			std::cout << "Param address: " << std::hex << std::setw(4) << base_addr << " ";
			std::cout << "Class : " << std::hex << std::setw(4) << _class << " ";
			std::cout << "Ver: " << std::hex << std::setw(2) << unsigned(id) << " ";

			std::ostringstream oss;
			std::string& obis = boost::get<std::string>(s.f.at(3));
			for (size_t i = 0; i < obis.size(); i++) 
				oss << unsigned((unsigned char) (obis[i])) << " ";
			
			std::cout << "OBIS: " << oss.str() << std::endl;
				
		}
	} catch (boost::bad_get &e) {
		std::cout << "Failed to fetch params " << e.what() << std::endl;
	}
}

void fetch_params(DaemonConfig *cfg, unsigned short address, std::string iec_address) {
	SerialPort* port = new SerialPort(cfg->GetDevicePath());
	IECConnector *iec = NULL;
	if (iec_address != std::string())
		iec = new IECConnector(port, iec_address);

	HDLCConnection *hdlc = new HDLCConnection(port, cfg->GetSpeed(), address, 4, 0x21, iec);
	COSEMConnection *cosem = new COSEMConnection(hdlc);

	cosem->Connect();
	Value v = cosem->GetUnitParams();
	cosem->Disconnect();

	print_unit_params(v);

	delete cosem;
	delete hdlc;
	delete port;
}

void scan_units(DaemonConfig *cfg, int scan_start, int scan_end) {
	SerialPort* port = new SerialPort(cfg->GetDevicePath());
	std::cout << "Starting device scan from addres " << scan_start << " to address " << scan_end << std::endl;
	for (int i = scan_start; i <= scan_end; i++) {
		HDLCConnection *hdlc = new HDLCConnection(port, cfg->GetSpeed(), i);
		try {
			std::cout << "Attempting connection to " << i << std::endl;
			hdlc->Connect();
			std::cout << "Successfully connected to " << i << std::endl;
		} catch (...) {
			std::cout << "Failed to to connect to " << i << std::endl;
		}
		delete hdlc;
	}

}

int main(int argc, char *argv[]) {
	DaemonConfig* cfg = new DaemonConfig("dlmsdmn");

	bool fetch_mode = false;
	bool dump_units_mode = false;
	bool scan_units_mode = false;
	int start_scan = 0;
	int end_scan = 0;
	unsigned short address = 0;
	std::string iec_address;

	cfg->SetUsageHeader("DLMSDMN.\nDaemon for quering enerty meters over COSEM/HDLC protocol.\n\n"
"Extra options supported by daemon:\n"
"	--fetch-parameters=<address>	enters special mode - queries device for measuered parameters.\n"
"After this operation completes, daemon terminates.\n"
"	--iec-address=<address> IEC protocol address of a meter, used only in fetch mode, if not present\n"
" protocol IEC won't be used.\n"
"	--dump-units			prints units and precisions of parameters defined in configuration\n"
);

	for (int i = 0; i < argc;) {
		int shift = 0;
		if (!strncmp(argv[i], "--fetch-parameters=", 19)) {
			char *endptr;
			address = strtoul(argv[i] + 19, &endptr, 0);
			if (*endptr != 0) {
				std::cerr << "Invalid device address specification";
				return 1;
			} else
				std::cout << "Quering device: " << std::hex << address << std::endl;

			fetch_mode = true;
			shift = 1;
		} else if (!strcmp("--dump-units", argv[i])) {
			dump_units_mode = true;	
			shift = 1;
		} else if (!strcmp("--scan-units", argv[i])) {
			if (i + 2 >= argc)
				std::cerr << "Missing units range";
			start_scan = atoi(argv[i + 1]);
			end_scan = atoi(argv[i + 2]);
			scan_units_mode = true;
			shift = 3;
		} else if (!strncmp("--iec-address=", argv[i], 14)) {
			iec_address = argv[i] + 14;
			shift = 1;
		}

		if (shift) {
			int j = i;
			for (int k = j + shift; k < argc; ++k, ++j)
				argv[j] = argv[k];
			argc -= shift;
		} else {
			i++;
		}
	}

	if (cfg->Load(&argc, argv))
		return 1;

	g_single = cfg->GetSingle() || cfg->GetDiagno();

	if (fetch_mode)
		fetch_params(cfg, address, iec_address);
	else if (scan_units_mode)
		scan_units(cfg, start_scan, end_scan);
	else {
		COSEMDaemon daemon;
		if (daemon.Configure(cfg) == false) 
			return 1;

		if (!dump_units_mode) 
			daemon.Start();
		else
			daemon.DumpParamsUnits();
	}
	
}

void dolog(int level, const char * fmt, ...) {
	va_list fmt_args;

	if (g_single) {
		char *l;
		va_start(fmt_args, fmt);
		vasprintf(&l, fmt, fmt_args);
		va_end(fmt_args);

		std::cout << l << std::endl;
		sz_log(level, "%s", l);
		free(l);
	} else {
		va_start(fmt_args, fmt);
		vsz_log(level, fmt, fmt_args);
		va_end(fmt_args);
	}

} 

#else

#include <iostream>

int main(int argc, char *argvp[]) {
	std::cerr << "SZARP need to be compiled with boost and lber library for this porogram to work" << std::endl;
	return 1;
}

#endif
