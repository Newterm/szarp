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
 * tensdmn - daemon for reading tensometring wight counters
 *
 * <device ... speed="19200">
 *  <unit id="1" .../>
 *
 * $Id: nrsdmn.cc 1 2009-06-24 15:09:25Z isl $
 */
/*
 @description_start
 @class 4
 @devices Tensometric weighing machine Rowag MWT 6.
 @devices.pl Waga tensometryczna Rowag MWT 6.
 @description_end
 */

#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
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
#include <assert.h>
#include <stdexcept>
#include <iterator>
#include <boost/tokenizer.hpp>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "conversion.h"
#include "dmncfg.h"
#include "ipchandler.h"
#include "liblog.h"
#include "tokens.h"
#include "xmlutils.h"

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
		m_fd(-1), m_path(path), m_speed(speed),
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
	void Open();
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
	int m_stopbits;		/**<number of stop bits (1 default)*/
	int m_timeout;		/**<timeout of read operation*/
};

void SerialPort::Close() 
{
	if (m_fd < 0) 
		return;
	close(m_fd);
	m_fd = -1;
}

void SerialPort::Open()
{
	if (m_fd >= 0)
		return;
	
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

	sz_log(8, "setting port speed to %d", m_speed);
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
	time_t t1, t2;
	ssize_t r = 0;
	time (&t1);
	t2 = t1;
	while (r < (ssize_t) size) {
		int delay;
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

	return r;
}

void SerialPort::WriteData(const char* buffer, size_t size) 
{
	size_t sent = 0;
	int ret;
	while (sent < size) {
		ret = write(m_fd, buffer + sent, size - sent);
			
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

/******************************************************************/

bool g_debug = false;

class Daemon {
#define BUF_SIZE 1000	/**<transmit buffer size*/
#define WAIT_TIME 4	/**<time we will wait for unit to answer (seconds)*/
#define CYCLE_TIME 10	/**<lenght of a full daemon cycle (seconds)*/
public:
	Daemon(DaemonConfig *cfg) : m_ipc(NULL) {
		if (ConfigureDaemon(cfg))
			throw std::runtime_error("Daemon initialization failed");
	}
	/*Main daemon's routine, never returns.
	 * Periodically sends queries to units via SerialPort class.
	 * Fetches data from sender and stores values retrieved from units
	 * into parcook shared memory segment.*/
	void Go(); 

	void ParseWeightResponse(const char* buffer, size_t size);

	void ParseFlowResponse(const char* buffer, size_t size);

private:
	int ConfigureDaemon(DaemonConfig *cfg);
	/**Object responsible for communication with sender and parcook*/
	IPCHandler *m_ipc;
	/**DaemonConfig - daemon configuration*/
	DaemonConfig *m_cfg;
	/**Object communicating with serial port*/
	SerialPort *m_port;
	/**Cycle length(in seconds)*/
	int m_cycle_duration;

	std::vector<int> m_weight_params, m_flow_params;

	size_t m_params_count;

	std::vector<int> m_param_precs;
};

int Daemon::ConfigureDaemon(DaemonConfig *cfg) 
{
	const char *device_name = strdup(cfg->GetDevicePath());
	g_debug = cfg->GetSingle() || cfg->GetDiagno();
	if (!device_name) {
		sz_log(0,"No device specified -- exiting");
		exit(1);
	}
	int speed = cfg->GetSpeed();
	int stopbits = cfg->GetNoConf() ? 1 : cfg->GetDevice()->GetStopBits();
	int wait_time = WAIT_TIME;
	m_cycle_duration = CYCLE_TIME;
	m_ipc = new IPCHandler(cfg);
	if (m_ipc->Init()) {
		sz_log(0, "Intialization of IPCHandler failed");
		return 1;
	}
	TDevice* dev = cfg->GetDevice();
	TUnit* unit = dev->GetFirstRadio()->GetFirstUnit();
	assert(unit);
	xmlDocPtr doc;
	doc = cfg->GetXMLDoc();
	xmlXPathContextPtr xp_ctx = xmlXPathNewContext(doc);
	int ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "ipk",
			SC::S2U(IPK_NAMESPACE_STRING).c_str());
	assert (ret == 0);
	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "tens",
			BAD_CAST IPKEXTRA_NAMESPACE_STRING);
	assert (ret == 0);
	xp_ctx->node = cfg->GetXMLDevice();
	int i = 0;
	for (TParam* p = unit->GetFirstParam(); p; p = p->GetNext(), i++) {
		char *expr;
	        asprintf(&expr, ".//ipk:unit[position()=1]/ipk:param[position()=%d]", i + 1);
		xmlNodePtr node = uxmlXPathGetNode(BAD_CAST expr, xp_ctx);
		assert(node);
		char* c = (char*) xmlGetNsProp(node, BAD_CAST("type"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
		if (c == NULL || !strcmp(c, "weight"))
			m_weight_params.push_back(i);
		else if (!strcmp(c, "flow"))
			m_flow_params.push_back(i);
		else {
			sz_log(0, "Unsupported param type: %s", c);
			return 1;
		}
		assert(node);
		free(expr);
		xmlFree(c);
		m_param_precs.push_back(pow10(p->GetPrec()));
	}
	m_params_count = unit->GetParamsCount();
	m_port = new SerialPort(device_name,
			speed, 
			stopbits,
			wait_time);
	try {
		m_port->Open();
	} catch (SerialException&) {
		sz_log(0, "Failed to open port:%s\n",device_name);
		return 1;
	}
	return 0;
}

void Daemon::ParseFlowResponse(const char* buffer, size_t size) {
	size_t i;
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

	if (g_debug) {
		std::cout << "Response:" << std::endl;
		std::cout << std::string(buffer, buffer + size);
	}

	tokenizer msg_tokenizer(std::string(buffer, buffer + size), boost::char_separator<char>("\r\n"));
	tokenizer::iterator msg_iter = msg_tokenizer.begin();
	for (i = 0; i < m_flow_params.size() && msg_iter != msg_tokenizer.end(); msg_iter++, i++) {
		tokenizer line_tokenizer(*msg_iter, boost::char_separator<char>(" "));
		tokenizer::iterator line_iter = line_tokenizer.begin();
		std::advance(line_iter, 8);
		if (line_iter == line_tokenizer.end()) {
			sz_log(1, "Invalid response format for param: %d", i);
			return;
		}
		if (g_debug)
			std::cout << "Param no:" << i << " val: " << atof((*line_iter).c_str()) << std::endl; 
		m_ipc->m_read[m_flow_params[i]] = atof((*line_iter).c_str()) * m_param_precs[m_flow_params[i]];
	}
	if (m_flow_params.size() != i) {
		sz_log(1, "Not enough params received from weigh, got %d, expected at least %d", i, m_flow_params.size());
		return;
	}

}

void Daemon::ParseWeightResponse(const char* buffer, size_t size) {
	size_t i;
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

	if (g_debug) {
		std::cout << "Response:" << std::endl;
		std::cout << std::string(buffer, buffer + size);
	}

	tokenizer msg_tokenizer(std::string(buffer, buffer + size), boost::char_separator<char>("\r\n"));
	tokenizer::iterator msg_iter = msg_tokenizer.begin();
	std::advance(msg_iter, 4);

	for (i = 0; i < m_weight_params.size() && msg_iter != msg_tokenizer.end(); msg_iter++, i++) {
		tokenizer line_tokenizer(*msg_iter, boost::char_separator<char>(" "));
		tokenizer::iterator line_iter = line_tokenizer.begin();
		std::advance(line_iter, 4);
		if (line_iter == line_tokenizer.end()) {
			sz_log(1, "Invalid response format for param: %d", i);
			return;
		}
		if (g_debug)
			std::cout << "Param no:" << i << " val: " << atof((*line_iter).c_str()) << std::endl; 
		m_ipc->m_read[m_weight_params[i]] = atof((*line_iter).c_str()) * m_param_precs[m_weight_params[i]];
	}
	if (m_weight_params.size() != i) {
		sz_log(1, "Not enough params received from weigh, got %d, expected at least %d", i, m_weight_params.size());
		return;
	}
}

void Daemon::Go() 
{
	char buffer[BUF_SIZE];
	while (true) {

		time_t t1,t2;
		time(&t1);

		if (m_ipc) for (size_t i = 0; i < m_params_count; i++)
			m_ipc->m_read[i] = SZARP_NO_DATA;

		try {
			m_port->Open();
			if (m_weight_params.size()) {
				m_port->WriteData("L\r", 2);
				ssize_t read;
				read = m_port->GetData(buffer, BUF_SIZE);
				if (read == 0) {
					m_port->Close();
					throw SerialException();
				}

				ParseWeightResponse(buffer, read);
			}

			if (m_flow_params.size()) {
				m_port->WriteData("P\r", 2);
				ssize_t read;
				read = m_port->GetData(buffer, BUF_SIZE);
				if (read == 0)
					m_port->Close();
				else
					ParseFlowResponse(buffer, read);
			}

		} catch (SerialException&) {
			if (m_ipc) for (size_t i = 0; i < m_params_count; i++)
				m_ipc->m_read[i] = SZARP_NO_DATA;
			
		}

		time(&t2);
		int rest = m_cycle_duration - (t2 - t1);
		while (rest > 0) {
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

int main(int argc, char *argv[]) {

	DaemonConfig* cfg = new DaemonConfig("tensdmn");

	if (cfg->Load(&argc, argv)) {
		return 1;
	}

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

