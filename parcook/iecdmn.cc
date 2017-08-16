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
 @devices Various energy meters using protocol described in IEC 62056-21.
 @devices.pl Liczniki energii u¿ywaj±ce protoko³u opisanego w IEC 62056-21.
 @protocol IEC (IEC-62056-21)
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

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/ioctl.h>

#include <boost/algorithm/string.hpp>

#include "liblog.h"
#include "dmncfg.h"
#include "ipchandler.h"
#include "tokens.h"
#include "xmlutils.h"
#include "conversion.h"
#include "custom_assert.h"

bool g_single;

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

const char ACK = 0x06;
const char STX = 0x02;
const char ETX = 0x03;
const char EOT = 0x04;

class DeviceResponseTimeout : public std::runtime_error {
	public:
	DeviceResponseTimeout() : std::runtime_error("") {};
};

class IECException : public std::runtime_error {
	public:
	IECException(const char *err) : std::runtime_error(err) {};
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
	void Open(int ispeed, int ospeed = -1, bool optical = false);

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

	void AddParityBit(unsigned char* output, const unsigned char* input, size_t size);

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
		} else if (tv.tv_sec == 0) {
			dolog(2, "I cannot wait for zero seconds");
			return false;
		}
		tv.tv_usec = 0;
		FD_ZERO(&set);
		FD_SET(m_fd, &set);

		ret = select(m_fd + 1, &set, NULL, NULL, &tv);
		if (ret < 0) {
			if (errno == EINTR) {
				dolog(2, "select interrupted by signal (EINTR)");
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

	if (size && ret == 0) {
		dolog(1, "read failed, read returned 0");
		throw SerialException(EBADF);
	}
	for (ssize_t i = 0; i < ret; i++)
		((unsigned char*)buffer)[i] &= 0x7f;
	return ret;
}

void SerialPort::AddParityBit(unsigned char* output, const unsigned char* input, size_t size) {
	for (size_t i = 0; i < size; i++) {
		int mask = 1;
		bool even = false;
		for (size_t j = 0; j < 7; j++, mask <<= 1)
			if (input[i] & mask)
				even = !even;

		output[i] = input[i];
		if (even)
			output[i] |= 128;
	}

}

void SerialPort::WriteData(const void* buffer, size_t size) {
	size_t sent = 0;
	int ret;
	unsigned char b[size];
	AddParityBit(b, (const unsigned char*)buffer, size);

	while (sent < size) {
		ret = write(m_fd, (const unsigned char*)b + sent, size - sent);

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

void SerialPort::Open(int ispeed, int ospeed, bool optical)
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
			dolog(1, "iecdmn: cannot open device %s, errno:%d (%s)", m_path,
				errno, strerror(errno));
			throw SerialException(errno);
		}
	}

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

	ti.c_cflag |= CREAD | CS8 | CLOCAL;

	cfsetospeed(&ti, speed_to_speed_t(m_ospeed));
	cfsetispeed(&ti, speed_to_speed_t(m_ispeed));

	if (tcsetattr(m_fd, TCSAFLUSH, &ti) == -1) {
		dolog(1, "Cannot set port settings, errno: %d (%s)",
			errno, strerror(errno));
		Close();
		throw SerialException(errno);
	}

	if (optical) {
		dolog(6, "Optical set, setting RTS high");
		/* Set RTS high */
		int flags;
		ioctl(m_fd, TIOCMGET, &flags);
		flags &= ~TIOCM_RTS;
		//flags &= ~TIOCM_DTR;
		ioctl(m_fd, TIOCMSET, &flags);
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

class IECDaemon {
	static const int c_default_timeout;

	IPCHandler *m_ipc;
	SerialPort *m_port;

	struct Value {
		Value() : prec(0), msw(-1), lsw(-1), selector(-1) {}
		int prec;
		int msw;
		int lsw;
		int selector;
	};
	struct Unit {
		std::map<std::string, Value> values;
		std::string address;
	};
	std::vector<Unit> m_units;
	int m_speed;
	int m_timeout;
	int m_wait_timeout;
	int m_const_speed;
	bool m_optical;

	bool ConfigureUnit(TUnit *unit, xmlNodePtr xunit, int& param_index, xmlXPathContextPtr xp_ctx);
	void QueryUnit(IECDaemon::Unit &unit);
	void ParseParamValue(std::istream& istream, Unit& unit);
	void ParseResponse(IECDaemon::Unit& unit, const char* read_buffer, size_t buf_size);
public:
	void Start();
	bool Configure(DaemonConfig* cfg);
	void WaitForSilence();
	void Skip(size_t to_skip);

};

const int IECDaemon::c_default_timeout = 2;

bool IECDaemon::ConfigureUnit(TUnit *unit, xmlNodePtr xunit, int& param_index, xmlXPathContextPtr xp_ctx) {
	Unit dunit;
	xmlChar* _address = xmlGetNsProp(xunit, BAD_CAST "address", BAD_CAST IPKEXTRA_NAMESPACE_STRING);
	if (_address == NULL) {
		dolog(0, "Attribute iec:address not present in unit element, line no (%ld)", xmlGetLineNo(xunit));
		return false;
	}
	dunit.address = (char*)_address;
	xmlFree(_address);

	xmlChar* timeout = xmlGetNsProp(xunit, BAD_CAST("timeout"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (timeout == NULL) {
		sz_log(2, "No attribute timeout given, assuming default %d (line %ld)", c_default_timeout, xmlGetLineNo(xunit));
		m_timeout = c_default_timeout;
	}
	else {
		m_timeout = boost::lexical_cast<int>(timeout);
		xmlFree(timeout);
	}

	timeout = xmlGetNsProp(xunit, BAD_CAST("wait_timeout"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (timeout == NULL) {
		sz_log(2, "No attribute wait_timeout given, skipping disabled");
		m_wait_timeout = 0;
	}
	else {
		m_wait_timeout = boost::lexical_cast<int>(timeout);
		xmlFree(timeout);
	}

	xmlChar* const_speed = xmlGetNsProp(xunit, BAD_CAST("const_speed"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (const_speed != NULL) {
		m_const_speed = boost::lexical_cast<int>(const_speed);
		xmlFree(const_speed);
	} else {
		m_const_speed = -1;
	}

	xmlChar* optical = xmlGetNsProp(xunit, BAD_CAST("optical"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (optical != NULL) {
		std::string s = (char*)optical;
		if (s.compare(std::string("true")) == 0)
			m_optical = true;
		else
			m_optical = false;
		xmlFree(optical);
	} else {
		m_optical = false;;
	}

	xp_ctx->node = xunit;
	xmlXPathObjectPtr rset = xmlXPathEvalExpression(BAD_CAST "./ipk:param", xp_ctx);
	TParam* p = unit->GetFirstParam();
	for (int j = 0; j < rset->nodesetval->nodeNr; j++, p = p->GetNext()) {
		Value value;
		xmlNodePtr n = rset->nodesetval->nodeTab[j];

		xmlChar* _address = xmlGetNsProp(n, BAD_CAST("address"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
		if (_address == NULL) {
			dolog(0, "Attribute iec:address not present in param element, line no (%ld)", xmlGetLineNo(n));
			return false;
		}
		std::string address = (char*)_address;
		xmlFree(_address);

		int selector = -1;
		xmlChar* _selector = xmlGetNsProp(n, BAD_CAST("selector"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
		if (_selector != NULL) {
			selector = boost::lexical_cast<int>(_selector);
			xmlFree(_selector);
		}

		bool is_msw = false;
		xmlChar* _is_msw = xmlGetNsProp(n,
				BAD_CAST("word"),
				BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
		if (_is_msw && !xmlStrcmp(_is_msw, BAD_CAST("msw")))
			is_msw = true;
		xmlFree(_is_msw);
		int prec = pow10(p->GetPrec());
		if (dunit.values.find(address) != dunit.values.end()) {
			Value& value = dunit.values[address];
			if (selector != -1)  value.selector = selector;

			if (prec)
				value.prec = prec;
			if (is_msw)
				value.msw = param_index++;
			else
				value.lsw = param_index++;
		} else {
			Value value;
			value.prec = prec;
			if (selector != -1) value.selector = selector;

			if (is_msw)
				value.msw = param_index++;
			else
				value.lsw = param_index++;
			dunit.values[address] = value;
		}
	}
	m_units.push_back(dunit);
	xmlXPathFreeObject(rset);

	return true;
}

bool IECDaemon::Configure(DaemonConfig* cfg) {
	m_speed = cfg->GetSpeed();

	try {
		auto ipc_ = std::unique_ptr<IPCHandler>(new IPCHandler(*cfg));
		m_ipc = ipc_.release();
	} catch(...) {
		return 1;
	}

	m_port = new SerialPort(cfg->GetDevicePath());

	TDevice* dev = cfg->GetDevice();
	xmlNodePtr xdev = cfg->GetXMLDevice();

	xmlXPathContextPtr xp_ctx = xmlXPathNewContext(xdev->doc);
	xp_ctx->node = xdev;

	int ret;
	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "ipk", SC::S2U(IPK_NAMESPACE_STRING).c_str());
	ASSERT(ret == 0);

	xmlXPathObjectPtr uset = xmlXPathEvalExpression(BAD_CAST ".//ipk:unit", xp_ctx);
	int param_index = 0;
	int i = 0;
	for (TUnit *unit = dev->GetFirstRadio()->GetFirstUnit(); unit; unit = unit->GetNext(), i++) {
		ASSERT(i < uset->nodesetval->nodeNr);
		xmlNodePtr xunit = uset->nodesetval->nodeTab[i];
		if (!ConfigureUnit(unit, xunit, param_index, xp_ctx))
			return false;
	}
	xmlXPathFreeObject(uset);
	xmlXPathFreeContext(xp_ctx);

	return true;
}

void IECDaemon::WaitForSilence() {
	dolog(10, "Waiting for silence on the wire");
	while (m_port->Wait(m_wait_timeout)) {
		char buf[100];
		m_port->GetData(buf, 100);
	}
}

void IECDaemon::Skip(size_t to_skip) {
	while (to_skip) {
		if (!m_port->Wait(m_timeout))
			throw DeviceResponseTimeout();

		char buf[to_skip];

		to_skip -= m_port->GetData(buf, to_skip);
	}
}

void IECDaemon::QueryUnit(IECDaemon::Unit &unit) {
	m_port->Open(m_speed, -1, m_optical);

	if (m_wait_timeout != 0) WaitForSilence();

	std::ostringstream qs;
	qs << "/?" << unit.address << "!\r\n";

	m_port->WriteData((const unsigned char*) qs.str().c_str(), qs.str().size());

	if (m_wait_timeout != 0) this->Skip(qs.str().size());

	unsigned char read_buffer[10000];
	ssize_t read_pos = 0;
	do {
		if (!m_port->Wait(m_timeout))
			throw DeviceResponseTimeout();

		read_pos += m_port->GetData(read_buffer + read_pos, sizeof(read_buffer) - read_pos);

		if (read_pos == sizeof(read_buffer)) {
			m_port->Close();
			throw IECException("Response form meter too large");
		}

	} while (read_pos < 7 || (read_buffer[read_pos - 2] & 0x7f) != '\r' || (read_buffer[read_pos - 1] & 0x7f) != '\n');

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

	dolog(6, "Got speed %d", speed);

	qs.str(std::string());

	if (m_const_speed != -1) {
		dolog(6, "Const speed is %d", m_const_speed);

		switch (m_const_speed) {
			case 300:
				qs << ACK << "000\r\n";
				break;
			case 600:
				qs << ACK << "010\r\n";
				break;
			case 1200:
				qs << ACK << "020\r\n";
				break;
			case 2400:
				qs << ACK << "030\r\n";
				break;
			case 4800:
				qs << ACK << "040\r\n";
				break;
			case 9600:
				qs << ACK << "050\r\n";
				break;
			case 19200:
				qs << ACK << "060\r\n";
				break;
			default:
				qs << ACK << "000\r\n";
		}
		m_port->WriteData((const unsigned char*) qs.str().c_str(), qs.str().size());

	} else {
		qs << ACK << "0" << char(read_buffer[4]) << "0\r\n";
		dolog(6, "Speed is %d", speed);
		m_port->Open(speed, -1, m_optical);
	}

	if (m_wait_timeout != 0) this->Skip(qs.str().size());

	read_pos = 0;
	do {
		if (!m_port->Wait(m_timeout))
			throw DeviceResponseTimeout();

		read_pos += m_port->GetData(read_buffer + read_pos, sizeof(read_buffer) - read_pos);

		if (read_pos == sizeof(read_buffer)) {
			m_port->Close();
			throw IECException("Response form meter too large");
		}

		dolog(10, "Read so far:\n%*s", (int)read_pos, read_buffer);

	} while (read_pos < 4 || read_buffer[read_pos - 2] != ETX);

	ParseResponse(unit, (const char*)read_buffer, read_pos);
}

void IECDaemon::ParseParamValue(std::istream& istream, Unit& unit) {
	std::string address;
	std::string value;
	double dval;
	int ival;
	short lsw;
	short msw;

	dolog(10, "IECDaemon: ParseParamValue");
	std::getline(istream, address, '(');

	std::map<std::string, Value>::iterator i = unit.values.find(address);
	if (i == unit.values.end()) {
		/* Skip values for this address */
		std::string s;
		std::getline(istream, s, ')');
		while (istream.peek() == '(') {
			istream.ignore(1);
			std::getline(istream, s, ')');
		}
		dolog(8, "Address %s skipped, not defined in configuration", address.c_str());
		return;
	}

	bool done = false;
	int cnt = 0;
	do {
		std::string v;
		std::getline(istream, v, ')');

		if (i->second.selector > -1) {
			dolog(10, "IECDaemon: ParseParamValue: using selector %d", i->second.selector);
			if (cnt == i->second.selector) value = v;
		} else {
			dolog(10, "IECDaemon: ParseParamValue: searching for value");
			if (v.find('*') != std::string::npos) value = v.substr(0,v.find("*",0));
		}

		cnt = cnt + 1;

		if (istream.peek() == '(')
			istream.ignore(1);
		else
			done = true;

	} while (!done);

	try {
		boost::trim(value);
		dval = boost::lexical_cast<double>(value);
	} catch (boost::bad_lexical_cast& bdl) {
		dolog(1, "Invalid value(%s) received for address: %s, parsing stopped at: %s",
				value.c_str(), address.c_str(), bdl.what());
		return;
	}

	/*
	vs = value.c_str();
	while (*vs == ' ')
		vs++;
	dval = strtod(vs, &e);

	//if (e == vs || *e != '*') {
	if (e == vs) {
		dolog(1, "Invalid value(%s) received for address: %s, parsing stopped at: %s", vs, address.c_str(), e);
		goto end;
	}
	*/

	ival = int(dval * i->second.prec);
	lsw = ival & 0xffffu;
	msw = ival >> 16;

	dolog(6, "Got value %s for address: %s", value.c_str(), address.c_str());

	if (i->second.msw >= 0)
		m_ipc->m_read[i->second.msw] = msw;
	if (i->second.lsw >= 0)
		m_ipc->m_read[i->second.lsw] = lsw;
}

void IECDaemon::ParseResponse(IECDaemon::Unit& unit, const char* read_buffer, size_t buf_size) {

	dolog(10, "IECDaemon: ParseResponse");

	std::istringstream is(std::string(read_buffer, read_buffer + buf_size));
	is.exceptions(std::ios_base::failbit | std::ios_base::badbit | std::ios_base::eofbit);

	try {
		is.ignore(1);//skip STX
		while (true) switch (is.peek()) {
				case '!': //end of message
					dolog(10, "IECDaemon: ParseResponse: end of message");
					return;
				case '\r':
					dolog(10, "IECDaemon: ParseResponse: next value");
					is.ignore(2);
					break;
				default:
					ParseParamValue(is, unit);
					break;
			}
	} catch (std::ios_base::failure) {
		dolog(0, "Failed to parse response from meter");
	}
}

void IECDaemon::Start() {

	while (true) {
		dolog(6, "Starting cycle");

		for (int i = 0; i < m_ipc->m_params_count; i++)
			m_ipc->m_read[i] = SZARP_NO_DATA;

		time_t start = time(NULL);
		for (size_t i = 0; i < m_units.size(); i++)
			try {
				dolog(6, "Querying unit %s", m_units[i].address.c_str());
				QueryUnit(m_units[i]);
			} catch (std::runtime_error& re) {
				dolog(1, "Error while querying unit: %s", re.what());
			}

		int to_sleep;
		while ((to_sleep = (start + 10 - time(NULL))) > 0)
			sleep(to_sleep);
		dolog(6, "Cycle finish");
		m_ipc->GoParcook();
	}

}

int main(int argc, char *argv[]) {
	DaemonConfig* cfg = new DaemonConfig("iecdmn");
	if (cfg->Load(&argc, argv))
		return 1;
	g_single = cfg->GetSingle() || cfg->GetDiagno();
	IECDaemon daemon;
	if (!daemon.Configure(cfg))
		return 1;
	daemon.Start();
	return 1;

}


void dolog(int level, const char * fmt, ...) {
	va_list fmt_args;

	if (g_single) {
		char *l;
		va_start(fmt_args, fmt);
		if (vasprintf(&l, fmt, fmt_args) == -1) {
			sz_log(0, "Error occured when logging in single mode");
			return;
		}
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

