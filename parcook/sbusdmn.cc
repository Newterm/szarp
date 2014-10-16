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
#include "serialport.h"
#include "utils.h"

bool single;

void dolog(int level, const char * fmt, ...)
  __attribute__ ((format (printf, 2, 3)));

xmlChar* get_device_node_prop(xmlXPathContextPtr xp_ctx, const char* prop) {
	xmlChar *c;
	char *e;
	asprintf(&e, "./@%s", prop);
	assert (e != NULL);
	c = uxmlXPathGetProp(BAD_CAST e, xp_ctx);
	free(e);
	return c;
}

xmlChar* get_device_node_extra_prop(xmlXPathContextPtr xp_ctx, const char* prop) {
	xmlChar *c;
	char *e;
	asprintf(&e, "./@extra:%s", prop);
	assert (e != NULL);
	c = uxmlXPathGetProp(BAD_CAST e, xp_ctx);
	free(e);
	return c;
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
	/** Clears whole buffer */
	void Clear();
};

unsigned int speed2const(int speed) {
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
			dolog(0, "WARNING: setting default port speed: 9600");
			return B9600;
	}
}

Buffer::Buffer() : m_read_pointer(0) {}

void Buffer::Clear() {
	m_read_pointer = 0;
	m_values.clear();
}

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

enum PROTOCOL_TYPE { SBUS, SBUS_PCD };

class BaseSBUSDaemon {
protected:
	std::string m_str_id;		/**< ID of given daemon */
public:
	virtual void UnitFinished() = 0;
	const char* GetId() const {
		return m_str_id.c_str();
	}
};

class SBUSUnit: public ConnectionListener  {
	static const int MAX_WAIT_FOR_CONFIG_MS = 5000;

	enum PARITY { MARK, SPACE };
	BaseSerialPort *m_port;
	struct termios m_termios;	/**< serial port configuration */
	short *m_buffer;
	unsigned char m_id;
	int m_read_timeout;
	int m_max_read_attempts;
	std::map<unsigned short, Param> m_params;
	PROTOCOL_TYPE m_protocol;

	std::map<unsigned short, Param>::iterator m_curr_param;
	unsigned short m_start_address;
					/**< starting address of current query */
	int m_param_count;		/**< count of params in current query */		
	int m_read_attempts;		/**< number of read attempts for current address range */
	Buffer m_response;		/**< buffer for response from device */
	size_t m_expected_response_size;/**< expected size of response from device */
	struct event m_ev_timer;	/**< event timer for calling QueryTimerCallback */
	
	typedef enum {IDLE, INIT_QUERY, START_QUERY, PCD_INIT, PCD_INITIALIZED, SEND_QUERY,
		WAIT_RESPONSE, PROCESSING_RESPONSE} CommState;
	CommState m_state;
	BaseSBUSDaemon *m_daemon;	/**< callback for telling daemon that unit finished querying */

	Buffer CreateQueryPacket(unsigned short start, int count);
	void SetParamsVals(unsigned short start, int count, Buffer& response);

	void ScheduleNext(unsigned int wait_ms=0);
	void Do();
	void InitSingleQuery();
	void PCDInitStart();
	void PCDInitContinue();
	void PCDInitFinish();
	void SendQuery();
	void ProcessResponse();
	void BadResponse();
	void SetPortParity(PARITY parity);
	const char* GetId() const {
		return m_daemon->GetId();
	}
public:
	SBUSUnit(BaseSBUSDaemon *daemon)
		:m_port(NULL), m_buffer(NULL), m_start_address(0),
		m_param_count(0), m_read_attempts(0), m_state(IDLE),
		m_daemon(daemon)
	{}
	bool Configure(PROTOCOL_TYPE protocol, TUnit *unit, xmlNodePtr xmlunit,
		short *params, BaseSerialPort *port, int speed);
	static void TimerCallback(int fd, short event, void* thisptr);
	void QueryAll();

	/** Serial port listener callbacks */
	virtual void ReadData(const std::vector<unsigned char>& data);
	virtual void ReadError(short int event);
	virtual void SetConfigurationFinished();
};

void SBUSUnit::ScheduleNext(unsigned int wait_ms)
{
	evtimer_del(&m_ev_timer);
	const struct timeval tv = ms2timeval(wait_ms);
	evtimer_add(&m_ev_timer, &tv);
}

void SBUSUnit::TimerCallback(int fd, short event, void* thisptr)
{
        reinterpret_cast<SBUSUnit*>(thisptr)->Do();
}

void SBUSUnit::SetPortParity(PARITY parity)
{
	struct termios ti = m_termios;
	switch (parity) {
		case MARK:
			ti.c_cflag |= PARENB | CMSPAR | PARODD;
			break;	
		case SPACE:
			ti.c_cflag |= PARENB | CMSPAR;
			break;
	}
	m_port->SetConfiguration(&ti);
}

bool SBUSUnit::Configure(PROTOCOL_TYPE protocol, TUnit *unit,
		xmlNodePtr xmlunit, short *params, BaseSerialPort *port, int speed) {
	m_protocol = protocol;
	m_buffer = params;
	m_port = port;

	m_termios.c_cflag = speed2const(speed);
	dolog(6, "setting port speed to %d", speed);
	m_termios.c_oflag = 0;
	m_termios.c_iflag = 0;
	m_termios.c_lflag = 0;
	m_termios.c_cflag |= CS8 | CREAD | CLOCAL;

	char *endptr;

	if (protocol != SBUS_PCD) {
		xmlChar* _id = xmlGetNsProp(xmlunit, BAD_CAST("id"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
		if (_id == NULL) {
			dolog(0, "%s: Attribbute sbus:id not present in unit element, line no (%ld)",
				GetId(), xmlGetLineNo(xmlunit)); 
			return false;
		}

		m_id = atoi((char*)_id);
		dolog(5, "%s: Attribbute sbus:id is %d", GetId(), int(m_id));
		xmlFree(_id);
	} else {
		m_id = 0;
	}
	

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
			dolog(0, "%s: Error, attribute sbus:address missing in param definition, line(%ld)", GetId(), xmlGetLineNo(n));
			return false;
		}

		unsigned short address = strtoul((char*)_address, &endptr, 0);
		if (*endptr != 0) {
			dolog(0, "%s: Error, invalid value of parameter sbus:address in param definition, line(%ld)",
				GetId(), xmlGetLineNo(n));
			return false;
		}

	       	int prec = 0;
		xmlChar* _prec = xmlGetNsProp(n,
				BAD_CAST("prec"), 
				BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
		if (_prec != NULL) {
			prec = strtol((char*)_prec, &endptr, 0);
			if (*endptr != 0) {
				dolog(0, "%s: Error, invalid value of parameter sbus:prec in param definition, line(%ld)",
					GetId(), xmlGetLineNo(n));
				return false;
			}
			xmlFree(_prec);
			dolog(5, "%s: Got prec %d", GetId(), prec);
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

	evtimer_set(&m_ev_timer, TimerCallback, this);
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

unsigned short CalculateCRC(const Buffer &b, size_t len, bool addfe) {
	unsigned short crc = 0;
	if (addfe) {
		int index = (((crc >> 8) ^ 0xfe) & 0xff);
		crc = crc_table[index] ^ ((crc << 8) & 0xffff);
	}
	for (size_t i = 0; i < len; i++) {
		int index = (((crc >> 8) ^ b.GetChar(i)) & 0xff);
		crc = crc_table[index] ^ ((crc << 8) & 0xffff);
	}
	return crc;
}

void SBUSUnit::Do() {
	switch (m_state) {
	case IDLE:
		break;
	case INIT_QUERY:
		InitSingleQuery();
		break;
	case START_QUERY:
		PCDInitStart();
		break;
	case PCD_INIT:
		PCDInitContinue();
		break;
	case PCD_INITIALIZED:
		PCDInitFinish();
		break;
	case SEND_QUERY:
		SendQuery();
		break;
	case WAIT_RESPONSE:
		if (m_response.Size() < 2) {
			dolog(8, "Timeout while waiting for response");
			BadResponse();
		} else {
			m_state = PROCESSING_RESPONSE;
			ProcessResponse();
		}
		break;
	case PROCESSING_RESPONSE:
		break;
	}
}

void SBUSUnit::QueryAll() {
	if (m_params.size() == 0)
		return;

	m_port->ClearListeners();
	m_port->AddListener(this);
	m_curr_param = m_params.begin();
	m_state = INIT_QUERY;
	ScheduleNext();
}

void SBUSUnit::InitSingleQuery() {
	if (m_curr_param == m_params.end()) {
		m_state = IDLE;
		m_port->Close();
		m_port->ClearListeners();
		m_daemon->UnitFinished();
		return;
	}
	m_start_address = m_curr_param->first;
	m_param_count = 1;

	for (++m_curr_param; m_curr_param != m_params.end(); ++m_curr_param) {
		unsigned short next_address = m_curr_param->first;
		if (next_address > m_start_address + m_param_count)
			break;
		if (m_param_count == 31)
			break;
		m_param_count++;
	}
	// m_curr_param now points to next param to be queried

	m_read_attempts = 0;
	m_state = START_QUERY;
	ScheduleNext();
}

void SBUSUnit::PCDInitStart() {
	try {
		m_port->Open();
		m_state = PCD_INIT;
		if (m_protocol == SBUS_PCD) {
			SetPortParity(MARK);
		} else {
			m_port->SetConfiguration(&m_termios);
		}
		ScheduleNext(MAX_WAIT_FOR_CONFIG_MS);
	} catch (const SerialPortException &ex) {
		dolog(0, "%s: Exception occured in PCDInitStart: %s", GetId(), ex.what());
		BadResponse();
	}
}

void SBUSUnit::PCDInitContinue() {
	try {
		m_state = PCD_INITIALIZED;
		if (!m_port->Ready()) {
			dolog(0, "%s: Error: port isn't ready in PCDInitContinue", GetId());
			BadResponse();
		}
		if (m_protocol == SBUS_PCD) {
			m_port->LineControl(0, 1);
			unsigned char fe = 0xfe;
			m_port->WriteData(&fe, 1);
			ScheduleNext(1000);
		} else {
			ScheduleNext();
		}
	} catch (const SerialPortException &ex) {
		dolog(0, "%s: Exception occured in PCDInitContinue: %s", GetId(), ex.what());
		BadResponse();
	}
}

void SBUSUnit::PCDInitFinish() {
	try {
		m_state = SEND_QUERY;
		if (m_protocol == SBUS_PCD) {
			SetPortParity(SPACE);
			ScheduleNext(MAX_WAIT_FOR_CONFIG_MS);
		} else {
			ScheduleNext();
		}
	} catch (const SerialPortException &ex) {
		dolog(0, "%s: Exception occured in PCDInitFinish: %s", GetId(), ex.what());
		BadResponse();
	}
}

void SBUSUnit::SendQuery() {
	try {
		if (!m_port->Ready()) {
			dolog(0, "%s: Error: port isn't ready in SendQuery", GetId());
			BadResponse();
		}
		m_response.Clear();
		Buffer query = CreateQueryPacket(m_start_address, m_param_count);
		m_port->WriteData(query.Content(), query.Size());
		m_expected_response_size = m_param_count * 4 
						+ 2;	//checksum
		if (m_protocol != SBUS_PCD)
			m_expected_response_size += 1	//frame synchronization byte
						+ 1;	//response code
		m_state = WAIT_RESPONSE;
		dolog(8, "Waiting for data");
		ScheduleNext(m_read_timeout * 1000);	// timer used as timeout
	} catch (const SerialPortException &ex) {
		dolog(0, "%s: Exception occured in SendQuery: %s", GetId(), ex.what());
		BadResponse();
	}
}

void SBUSUnit::ReadData(const std::vector<unsigned char>& data) {
	if (m_state != WAIT_RESPONSE) {
		dolog(4, "Data arrived outside WAIT_RESPONSE");
		return;
	}
	dolog(8, "Data arrived");

	for (auto c : data) {
		m_response.PutChar(c);
	}

	if ((m_response.Size() >= 2) && (m_protocol != SBUS_PCD)
			&& (m_response.GetChar(1) != '\x01')) {
		dolog(1, "%s: Error while quering device %d, invalid response type %d",
			GetId(), int(m_id), (int) m_response.GetChar(1));
		BadResponse();
		return;
	}

	if (m_response.Size() >= m_expected_response_size * 2) {
		m_state = PROCESSING_RESPONSE;
		ProcessResponse();
	} else {
		ScheduleNext(1000);	// single character timeout
	}
}

void SBUSUnit::ProcessResponse() {
	if (m_response.Size() > m_expected_response_size * 2)
		m_response.Resize(m_expected_response_size * 2);	// drop excessive bytes
	if (m_protocol != SBUS_PCD) {
		Buffer temp_buffer = m_response;
		try {
			m_response = DecodeB5Values(temp_buffer, temp_buffer.Size());
		} catch (...) {
			dolog(1, "%s: Error occurred while decoding response values", GetId());
			BadResponse();
			return;
		}
	}
	if (m_response.Size() != m_expected_response_size) {
		dolog(1, "%s: Invalid lenght of response for device %d, expected:%zu, got:%zu",
			GetId(), int(m_id), m_expected_response_size, m_response.Size());
		BadResponse();
		return;
	}

	unsigned short ccrc = CalculateCRC(m_response, m_response.Size() - 2, false);
	unsigned short pcrc = m_response.GetShort(m_response.Size() - 2);
	if (ccrc != pcrc) {
		dolog(1, "%s: Invalid crc while querying device %d, calculated:%hu, received:%hu", GetId(), int(m_id), ccrc, pcrc);
		BadResponse();
		return;
	}
	SetParamsVals(m_start_address, m_param_count, m_response);
	dolog(8, "Response parsed with success");
	m_state = INIT_QUERY;
	ScheduleNext();
}

void SBUSUnit::BadResponse() {
	m_port->Close();
	m_read_attempts++;
	if (m_read_attempts >= m_max_read_attempts) {
		m_state = INIT_QUERY;
	} else {
		m_state = START_QUERY;
	}
	ScheduleNext();
}

void SBUSUnit::ReadError(short int event) {
	dolog(8, "ReadError occured");
	BadResponse();
}

void SBUSUnit::SetConfigurationFinished() {
	ScheduleNext();
}

Buffer SBUSUnit::CreateQueryPacket(unsigned short start, int count) {
	Buffer b;
	if (m_protocol != SBUS_PCD) {
		b.PutChar('\xb5');
		b.PutChar('\x00');
		b.PutChar(m_id);
	}
	b.PutChar('\x06');
	b.PutChar(count - 1);
	b.PutShort(start);
	b.PutShort(CalculateCRC(b, b.Size(), m_protocol == SBUS_PCD));
	if (m_protocol != SBUS_PCD) 
		return EncodeB5Values(b);
	else
		return b;
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
	if (m_protocol != SBUS_PCD) {
		response.ReadChar();
		response.ReadChar();
	}
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

class SBUSDaemon: public BaseSBUSDaemon {
	std::vector<SBUSUnit*> m_units;
	int m_read_freq;
	IPCHandler *m_ipc;
	BaseSerialPort *m_port;

	std::string m_path;		/**< Serial port file descriptor path */
	std::string m_ip;		/**< SerialAdapter ip */
	int m_data_port;		/**< SerialAdapter data port number */
	int m_cmd_port;			/**< SerialAdapter command port number */

	std::vector<SBUSUnit*>::iterator m_current_unit;
	time_t m_start_cycle_time;

	typedef enum {IDLE, READY, QUERYING} DaemonState;
	DaemonState m_state;
	
	struct event m_ev_timer;	/**< event timer for calling QueryTimerCallback */

	void Do();
	void WarmUp();
	void StartCycle();
	void QueryUnit();
	void ScheduleNext(unsigned int wait_ms=0);
public:
	SBUSDaemon() :m_state(IDLE) {}
	bool Configure(DaemonConfig* cfg);
	void Start();
	virtual void UnitFinished();
	static void TimerCallback(int fd, short event, void* thisptr);
};

void SBUSDaemon::ScheduleNext(unsigned int wait_ms)
{
	evtimer_del(&m_ev_timer);
	const struct timeval tv = ms2timeval(wait_ms);
	evtimer_add(&m_ev_timer, &tv);
}

void SBUSDaemon::TimerCallback(int fd, short event, void* thisptr)
{
        reinterpret_cast<SBUSDaemon*>(thisptr)->Do();
}

bool SBUSDaemon::Configure(DaemonConfig *cfg) {
	m_ipc = new IPCHandler(cfg);
	if (m_ipc->Init()) {
		dolog(0, "Initialization of IPCHandler failed");
		return false;
	}

	TDevice* dev = cfg->GetDevice();
	xmlNodePtr xdev = cfg->GetXMLDevice();

	m_read_freq = 10;

	PROTOCOL_TYPE protocol = SBUS;

	xmlChar* _read_freq = xmlGetNsProp(xdev, BAD_CAST("read_freq"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (_read_freq != NULL) {
		char *endptr;
		m_read_freq = strtoul((char*)_read_freq, &endptr, 0);
		if (*endptr) {
			dolog(0, "Invalid value for sbus:read_freq attribute device element, line no (%ld)", xmlGetLineNo(xdev)); 
			xmlFree(_read_freq);
			return false;
		}
		xmlFree(_read_freq);
	}

	xmlChar* _protocol = xmlGetNsProp(xdev, BAD_CAST("protocol"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (_protocol && !xmlStrcmp(_protocol, BAD_CAST "PCD")) {
		protocol = SBUS_PCD;
		xmlFree(_protocol);
		dolog(2, "Using SBUS PCD protocol");
	} else {
		dolog(2, "Using SBUS serial protocol");
		protocol = SBUS;
	}

	xmlXPathContextPtr xp_ctx = xmlXPathNewContext(xdev->doc);
	xp_ctx->node = xdev;

	/* prepare xpath */
	assert (xp_ctx != NULL);
	int ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "ipk",
			SC::S2U(IPK_NAMESPACE_STRING).c_str());
	assert (ret == 0);
	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "extra",
			BAD_CAST IPKEXTRA_NAMESPACE_STRING);
	assert (ret == 0);

	xp_ctx->node = cfg->GetXMLDevice();

	xmlChar *c = get_device_node_extra_prop(xp_ctx, "tcp-ip");
	if (c == NULL) {
		xmlChar *path = get_device_node_prop(xp_ctx, "path");
		if (path == NULL) {
			dolog(0, "ERROR!: neither IP nor device path has been specified");
			return false;
		}
		m_path.assign((const char*)path);
		xmlFree(path);

		m_str_id = m_path;
	} else {
		m_ip.assign((const char*)c);
		xmlFree(c);

		xmlChar* tcp_data_port = get_device_node_extra_prop(xp_ctx, "tcp-data-port");
		if (tcp_data_port == NULL) {
			m_data_port = SerialAdapter::DEFAULT_DATA_PORT;
			dolog(2, "Unspecified tcp data port, assuming default port: %hu", m_data_port);
		} else {
			std::istringstream istr((char*) tcp_data_port);
			bool conversion_failed = (istr >> m_data_port).fail();
			if (conversion_failed) {
				dolog(0, "ERROR!: Invalid data port value: %s", tcp_data_port);
				return false;
			}
		}
		xmlFree(tcp_data_port);

		xmlChar* tcp_cmd_port = get_device_node_extra_prop(xp_ctx, "tcp-cmd-port");
		if (tcp_cmd_port == NULL) {
			m_cmd_port = SerialAdapter::DEFAULT_CMD_PORT;
			dolog(2, "Unspecified cmd port, assuming default port: %hu", m_cmd_port);
		} else {
			std::istringstream istr((char*) tcp_cmd_port);
			bool conversion_failed = (istr >> m_cmd_port).fail();
			if (conversion_failed) {
				dolog(0, "ERROR!: Invalid cmd port value: %s", tcp_cmd_port);
				return false;
			}
		}
		xmlFree(tcp_cmd_port);

		std::stringstream istr;
		std::string data_port_str;
		istr << m_data_port;
		istr >> data_port_str;
		m_str_id = m_ip + ":" + data_port_str;
	}
	int speed = cfg->GetSpeed();	// TODO if configured with IP, this returns -1
	if (m_ip.compare("") != 0) {
		SerialAdapter *client = new SerialAdapter();
		m_port = client;
		client->InitTcp(m_ip, m_data_port, m_cmd_port);
	} else {
		SerialPort *port = new SerialPort();
		m_port = port;
		port->Init(m_path);
	}

	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "ipk", SC::S2U(IPK_NAMESPACE_STRING).c_str());
	assert(ret == 0);

	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "sbus", BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	assert(ret == 0);


	xmlXPathObjectPtr rset = xmlXPathEvalExpression(BAD_CAST ".//ipk:unit", xp_ctx);

	short* reads = m_ipc->m_read;

	int i = 0;
	for (TUnit *unit = dev->GetFirstRadio()->GetFirstUnit(); unit; unit = unit->GetNext(), i++) {
		assert(i < rset->nodesetval->nodeNr);

		SBUSUnit* u  = new SBUSUnit(this);
		if (u->Configure(protocol, unit, rset->nodesetval->nodeTab[i], reads, m_port, speed) == false)
			return false;

		m_units.push_back(u);

		reads += unit->GetParamsCount();
	}


        evtimer_set(&m_ev_timer, TimerCallback, this);
	return true;
}

void SBUSDaemon::Start() {
	ScheduleNext();
	event_dispatch();
}

void SBUSDaemon::Do() {
	switch (m_state) {
	case IDLE:	// we never return to this state
		WarmUp();
		break;
	case READY:
		StartCycle();
		break;
	case QUERYING:
		QueryUnit();
		break;
	}
}

void SBUSDaemon::WarmUp() {
	time_t now = time(NULL);
	int left = now % m_read_freq;

	m_state = READY;
	if (left) {
		int s = m_read_freq - left;
		ScheduleNext(s * 1000);
	} else {
		ScheduleNext();
	}
}

void SBUSDaemon::StartCycle() {
	dolog(6, "Beginning parametr fetching loop");

	for (int i = 0; i < m_ipc->m_params_count; ++i) 
		m_ipc->m_read[i] = SZARP_NO_DATA;

	m_start_cycle_time = time(NULL);
	m_current_unit = m_units.begin();
	m_state = QUERYING;
	ScheduleNext();
}

void SBUSDaemon::QueryUnit() {
	if (m_current_unit == m_units.end()) {
		m_ipc->GoParcook();
		dolog(6, "End of parametrs' fetching loop");
		int sleep_seconds = m_start_cycle_time + m_read_freq - time(NULL);
		if (sleep_seconds < 0) {
			sleep_seconds = 0;
		}
		m_state = READY;
		dolog(6, "Schedule next in %ds", sleep_seconds);
		ScheduleNext(sleep_seconds * 1000);
	} else {
		(*m_current_unit)->QueryAll();
	}
}

void SBUSDaemon::UnitFinished() {
	++m_current_unit;
	QueryUnit();
}

int main(int argc, char *argv[]) {
	event_init();
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

	if (single) {
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


