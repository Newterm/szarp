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

#include "ppset.h"

#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <time.h>
#ifndef MINGW32
#include <termios.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#else
#include <windows.h>
#endif

#include <cassert>
#include <locale.h>

#include "xmlutils.h"

#include "liblog.h"

#define X (unsigned char *)

namespace  PPSET {

using std::string;
using std::endl;
using std::getline;
using std::ifstream;
using std::ios_base;
using std::ostringstream;
using std::istringstream;
using std::vector;

const int max_communiation_attempts = 3;

#ifndef MINGW32
const int invalid_port_pd = -1;
#else 
const PD invalid_port_pd = PD();
#endif

std::string create_printable_string(const char *buffer, int size);

xmlDocPtr parse_settings_response(const char *response, int len)
{
	const char *end = response + len - 1;
	/* find the 3rd '\r' from the end; our checksum should be right behind it */
	for (int i = 3; end > response; end--)
		if (*end == '\r' && --i == 0)
			break;

	if (end <= response) {
		sz_log(2,
		       "Unable to parse response from the regulator - it is too short %d", len);
		return NULL;
	}

	int received_checksum = atoi(end + 1);
	int calculated_checksum = 0;
	for (const char *c = response; c <= end; c++)
		calculated_checksum += *c;
	calculated_checksum &= 0xFFFF;

	if (calculated_checksum != received_checksum) {
		sz_log(2,
		       "Unable to parse response from the regulator - wrong checksum %d %d",
		       calculated_checksum, received_checksum);
		return NULL;
	}

	xmlDocPtr result = xmlNewDoc(X "1.0");
	xmlNodePtr root_node = xmlNewNode(NULL, X "message");
	xmlDocSetRootElement(result, root_node);
	xmlSetProp(root_node, X "type", X "regulator_settings");

	istringstream is(string(response, end + 1));
	is.exceptions(ios_base::failbit | ios_base::badbit | ios_base::eofbit);

	try {
		string s;
		char c;
		//skip first 0xd character
		is.ignore(1);
		//skip date and time
		is.ignore(len, '\r');
		//skip ILOSTA string
		is.ignore(len, ' ');
		//get number of const params
		int const_params_count;
		is >> const_params_count;
		//skip next '\r'
		is.ignore(1);
		xmlNodePtr constants =
		    xmlNewChild(root_node, NULL, X "constants", NULL);
		for (int i = 0; i < const_params_count; ++i) {
			xmlNodePtr constant =
			    xmlNewChild(constants, NULL, X "param", NULL);
			//<nr>,<val>[0x0D]
			getline(is, s, ',');
			xmlSetProp(constant, X "number", X s.c_str());
			getline(is, s, '\r');
			xmlSetProp(constant, X "value", X s.c_str());
		}
		//skip ILOFUN string
		is.ignore(len, ' ');

		int time_packs_count;
		is >> time_packs_count;

		//skip '\r'
		is.ignore(1);

		string packs_type;
		getline(is, packs_type, '\r');

		xmlNodePtr packs =
		    xmlNewChild(root_node, NULL, X "packs", NULL);
		xmlSetProp(packs, X "type", X packs_type.c_str());

		bool week_packs = false;
		if (packs_type == "WEEK")
			week_packs = true;

		while (true) {
			try {
				c = is.peek();
			} catch (ios_base::failure) {
				if (is.exceptions() & ios_base::eofbit)
					//ok - end of message
					break;
				//sth wrong with the message
				throw;
			}

			xmlNodePtr pack =
			    xmlNewChild(packs, NULL, X "pack", NULL);

			string day, hour, minute;
			if (c == 'D') {
				//'DEFAULT' string
				is.ignore(len, '\r');
				day = "0";
				hour = "0";
				minute = "0";
			} else {
				if (week_packs) 
					//<dd>[0x20]
					getline(is, day, ' ');
				//<hh>:<mm>[0xD] 
				getline(is, hour, ':');
				getline(is, minute, '\r');
			}

			if (week_packs) 
				xmlSetProp(pack, X "day", X day.c_str());
			xmlSetProp(pack, X "hour", X hour.c_str());
			xmlSetProp(pack, X "minute", X minute.c_str());

			for (int i = 0; i < time_packs_count; ++i) {
				xmlNodePtr param =
				    xmlNewChild(pack, NULL, X "param", NULL);
				getline(is, s, ',');
				xmlSetProp(param, X "number", X s.c_str());
				getline(is, s, '\r');
				xmlSetProp(param, X "value", X s.c_str());
			}

		}
	} catch (ios_base::failure) {
		sz_log(2, "Malformed response from regulator");
		xmlFreeDoc(result);
		return NULL;
	}

	return result;

}

xmlDocPtr create_error_message(const string & type, const string & reason)
{
	xmlDocPtr result = xmlNewDoc(X "1.0");
	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST "message");
	xmlDocSetRootElement(result, root_node);
	xmlSetProp(root_node, X "type", BAD_CAST type.c_str());
	xmlNewChild(root_node, NULL, BAD_CAST "reason",
		    BAD_CAST reason.c_str());
	return result;
}

serial_connection::serial_connection(char *path, int speed) : m_path(path), m_speed(speed) {
#ifndef MINGW32 
	m_fd = -1;
#endif
}

#ifndef MINGW32 
bool serial_connection::open(xmlDocPtr * err_msg, DaemonStopper &stopper)
{
	if (stopper.StopDaemon(err_msg) == false)
		return false;

	if ((m_fd = ::open(m_path.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK, 0)) < 0) {
		*err_msg =
		    create_error_message("error", "Unable to open port");
		return false;
	}

	struct termios ti;
	tcgetattr(m_fd, &ti);
	sz_log(5, "setting port speed to %d", m_speed);
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

	ti.c_cflag |= CS8 | CREAD | CLOCAL ;

	tcsetattr(m_fd, TCSANOW, &ti);

	return true;
}
#else
bool serial_connection::prepare_device(char *path, int speed, xmlDocPtr * err_msg, DaemonStopper &stopper)
{
	if (stopper.StopDaemon(err_msg) == false)
		return false;

	m_fd.h = CreateFile(path,  
		GENERIC_READ | GENERIC_WRITE, 
		0, 
		0, 
		OPEN_EXISTING,
		0,
		0);

	if (m_fd.h == INVALID_HANDLE_VALUE) {
		*err_msg =
		    create_error_message("error", "Unable to open port");
		return false;

	}


	DCB dcb;
	FillMemory(&dcb, sizeof(dcb), 0);
	dcb.DCBlength = sizeof(dcb);

	std::stringstream dcs;
	dcs << "baud=" << speed << " parity=n data=8 stop=1";
	if (!BuildCommDCB(dcs.str().c_str(), &dcb)) {
		*err_msg =
		    create_error_message("error", "Unable to set port settings");
		m_fd.h = INVALID_HANDLE_VALUE;
		pdclose(m_fd);
		return false;
	}

	if (!SetCommState(fd.h, &dcb)) {
		*err_msg =
		    create_error_message("error", "Unable to set port settings");
		m_fd.h = INVALID_HANDLE_VALUE;
		return false;
	}

	COMMTIMEOUTS comm_timeouts;
	comm_timeouts.ReadIntervalTimeout = 1000;
	comm_timeouts.ReadTotalTimeoutMultiplier = 1;
	comm_timeouts.ReadTotalTimeoutConstant = 5000;
	comm_timeouts.WriteTotalTimeoutMultiplier = 1;
	comm_timeouts.WriteTotalTimeoutConstant = 5000;

	if (SetCommTimeouts(fd.h, &comm_timeouts) == 0) 
		m_fd.h = INVALID_HANDLE_VALUE;


	return true;
}
#endif

void serial_connection::close() {
#ifndef MINGW32
	if (m_fd >= 0)
		::close(m_fd);
#else
	if (pdvalid(m_fd.h))
		CloseHandle(fd.h);
	m_fd = m_fd();
#endif
}

tcp_connection::tcp_connection(char *address, int port) : m_address(address), m_port(port) {}

bool tcp_connection::open(xmlDocPtr * err_msg, DaemonStopper &stopper) {

	struct sockaddr_in addr;
	long on = 1;

	if (inet_pton(AF_INET, m_address.c_str(), &addr.sin_addr) != 1) {
		*err_msg =
		    create_error_message("error", "Invalid address");
		return false;
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(m_port);

	m_fd = socket(PF_INET, SOCK_STREAM, 0);
#ifdef MINGW32
	if (ioctlsocket(m_fd, FIONBIO, &on)) {
#else
	if (ioctl(m_fd, FIONBIO, &on)) {
#endif
		*err_msg =
		    create_error_message("error", "Failed to set non-blocking mode on socket");
		goto error;
	}


	if (::connect(m_fd, (struct sockaddr*)&addr, sizeof(addr) != 0)) {
		fd_set set, exset;
		FD_ZERO(&set);
		FD_SET(m_fd, &set);
#ifdef MINGW32 
		FD_SET(m_fd, &exset);
#endif

		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 500000;

again:
		int ret = select(m_fd + 1, NULL, &set, &exset, &tv);
		if (ret == -1) {
			if (errno == EINTR)
				goto again;
			else {
				*err_msg =
				    create_error_message("error", "Failed to connect to host");
				goto error;
			}
		}

		if (ret == 0) {
			*err_msg =
			    create_error_message("error", "Failed to connect to host");
			goto error;
		}

#ifdef MINGW32
		if (FD_ISSET(sock, &exset)) {
			*err_msg =
			    create_error_message("error", "Failed to connect to host");
			goto error;
		}
#endif

	}


	return true;
error:
	::close(m_fd);
	m_fd = -1;
	return false;

}

bool descriptor_write(int fd, const char *buffer, int size) {
	bool ret = false;

	const int seconds = 10;
	time_t t1 = time(NULL);

	sz_log(2, "Writing to port:%s", create_printable_string(buffer, size).c_str());

	int pos = 0;

	while (pos < size) {
		struct timeval tv;

		tv.tv_sec = seconds - (time(NULL) - t1);
		tv.tv_usec = 0;

		if (tv.tv_sec < 0)
			break;

		fd_set set;
		FD_ZERO(&set);
		FD_SET(fd, &set);
		int ret = select(fd + 1, NULL, &set, NULL, &tv);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			sz_log(2, "Error while writing to port %s",
			       strerror(errno));
			break;
		} else if (ret == 0)
			continue;
again:
		ret = write(fd, buffer + pos, size - pos);
		if (ret < 0) {
			if (errno == EINTR)
				goto again;
			sz_log(2, "Error while writing to port %s",
			       strerror(errno));
			break;
		}

		pos += ret;
	}

	if (pos == size)
		ret = true;

	return ret;
}

bool descriptor_read(int fd, char** buffer, int *size) {
	const int start_buffer_size = 512;
	const int max_buffer_size = start_buffer_size * 4;

	string r;

	int buffer_size = start_buffer_size;
	*buffer = (char *)malloc(start_buffer_size);

	int pos = 0;

	bool first_wait = true;

	while (true) {
		struct timeval tv;

		tv.tv_sec = first_wait ? 5 : 1;
		tv.tv_usec = 0;

		fd_set set;
		FD_ZERO(&set);
		FD_SET(fd, &set);
		int ret = select(fd + 1, &set, NULL, NULL, &tv);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			sz_log(2, "Error while reading from port: %s",
			       strerror(errno));
			goto error;
		} else if (ret == 0) {
			if (pos == 0)
				sz_log(2,
				       "Timeout while waiting for respone from regulator");
			break;
		}
	      again:
		ret = ::read(fd, *buffer + pos, buffer_size - pos);
		if (ret < 0) {
			if (errno == EINTR)
				goto again;
			sz_log(2, "Error while reading from port %s",
			       strerror(errno));
			goto error;
		}

		pos += ret;
		if (pos == buffer_size) {
			buffer_size *= 2;
			*buffer = (char *)realloc(*buffer, buffer_size);
		}

		if (buffer_size >= max_buffer_size) {
			sz_log(2, "The response from regulator is too large!");
			goto error;
		}

		first_wait = false;
	}
	*size = pos;

	
	sz_log(4, "Response read from port: %s", create_printable_string(*buffer, pos).c_str());

	return true;
error:
	free(*buffer);
	*buffer = NULL;
	return false;
}

bool tcp_connection::write(const char* buffer, int size) {
	return descriptor_write(m_fd, buffer, size);
}

bool tcp_connection::read(char** buffer, int *size) {
	return descriptor_read(m_fd, buffer, size);
}

void tcp_connection::close() {
	if (m_fd >= 0) {
		::close(m_fd);
		m_fd = -1;
	}
}
	
std::string create_printable_string(const char *buffer, int size) {
	string r(buffer, size);
	for (size_t i = 0; i < r.size(); i++) {
		switch (r[i]) {
			case '0'...'9':
			case 'A'...'Z':
			case 'a'...'z':
			case '$':
			case ':':
			case ',':
			case '\n':
				break;
			default:
				r[i] = ' ';
		}
	}

	return r;
}


#ifndef MINGW32
bool serial_connection::write(const char *buffer, int size)
{
	bool ret = false;

	const int seconds = 10;
	time_t t1 = time(NULL);

	sz_log(2, "Writing to port:%s", create_printable_string(buffer, size).c_str());

	int pos = 0;

	while (pos < size) {
		struct timeval tv;

		tv.tv_sec = seconds - (time(NULL) - t1);
		tv.tv_usec = 0;

		if (tv.tv_sec < 0)
			break;

		fd_set set;
		FD_ZERO(&set);
		FD_SET(m_fd, &set);
		int ret = select(m_fd + 1, NULL, &set, NULL, &tv);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			sz_log(2, "Error while writing to port %s",
			       strerror(errno));
			break;
		} else if (ret == 0)
			continue;
again:
		ret = ::write(m_fd, buffer + pos, size - pos);
		if (ret < 0) {
			if (errno == EINTR)
				goto again;
			sz_log(2, "Error while writing to port %s",
			       strerror(errno));
			break;
		}

		pos += ret;
	}

	if (pos == size)
		ret = true;

	return ret;
}
#else
bool serial_connection::write(const char *buffer, int size)
{
	return descriptor_write(m_fd, buffer, size);
}
#endif

#ifndef MINGW32
bool serial_connection::read(char **buffer, int *size)
{
	return descriptor_read(m_fd, buffer, size);
}
#else
bool serial_connection::read(char **buffer, int *size) {
	const int start_buffer_size = 512;
	const int max_buffer_size = start_buffer_size * 4;

	int buffer_size = start_buffer_size;
	*buffer = (char *)malloc(start_buffer_size);

	bool first = true;
	DWORD last_read;
	int was_read = 0;
	string r;

	while (true) {
		if (!first) {
			COMMTIMEOUTS comm_timeouts;
			comm_timeouts.ReadIntervalTimeout = 1000;
			comm_timeouts.ReadTotalTimeoutMultiplier = 1;
			comm_timeouts.ReadTotalTimeoutConstant = 1000;
			comm_timeouts.WriteTotalTimeoutMultiplier = 1;
			comm_timeouts.WriteTotalTimeoutConstant = 5000;
			SetCommTimeouts(m_fd.h, &comm_timeouts);
		}

		if (!ReadFile(m_fd.h, *buffer + was_read, buffer_size - was_read, &last_read, NULL))
			goto error;

		first = false;

		if (last_read == 0)
			break;

		was_read += last_read;
		if (was_read == buffer_size) {
			buffer_size *= 2;
			*buffer = (char *)realloc(*buffer, buffer_size);
		}

		if (buffer_size >= max_buffer_size) {
			sz_log(2, "The response from regulator is too large!");
			goto error;
		}

	}
	*size = was_read;
	sz_log(4, "Response read from port: %s", create_printable_string(*buffer, *size).c_str());
	return true;

error:
	free(*buffer);
	return false;
}

#endif

bool wait_for_ok(connection& conn)
{
	static const char OK[] = "\x0dOK\x0d";
	char *data;
	int size;

	if (conn.read(&data, &size) == false)
		return false;

	bool ret = false;

	int ok_len = sizeof(OK) - 1;

	if (size == ok_len 
		&& !strncmp(data, OK, ok_len))
		ret = true;

	free(data);
	return ret;
}

bool parse_regulator_id(const char *buffer, size_t size, regulator_id& rid) {

	istringstream is(string(buffer, size));
	is.exceptions(ios_base::failbit | ios_base::badbit | ios_base::eofbit);

	std::map<std::string, std::string> ids;
	std::string key, val;

	try {
		//skip first 0xd character
		is.ignore(1);

		while (true) {
			//get key 
			getline(is, key, ' ');
			//get actual value
			getline(is, val, '\x0d');

			ids[key] = val;
		}
	
	} catch (ios_base::failure) {
		if (!(is.exceptions() & ios_base::eofbit)) {
			sz_log(2, "Malformed identification response from regulator");
			return false;
		}
	}

	rid.ne = ids["nE"];
	rid.nl = ids["nL"];
	rid.nb = ids["nb"];
	rid.np = ids["nP"];

	return true;
}

void append_regualtor_id(regulator_id &rid, xmlDocPtr doc) {
	xmlNodePtr root_node = xmlDocGetRootElement(doc);

	xmlNodePtr unit_node = xmlNewChild(root_node, NULL, X "unit", NULL);
	xmlSetProp(unit_node, X "nE", X rid.ne.c_str());
	xmlSetProp(unit_node, X "nP", X rid.np.c_str());
	xmlSetProp(unit_node, X "nL", X rid.nl.c_str());
	xmlSetProp(unit_node, X "nB", X rid.nb.c_str());

}

xmlDocPtr read_regulator_id(xmlDocPtr doc, connection &conn, char *id) {
	for (int i = 0; i < max_communiation_attempts; i++) {
		string command = string("\x11\x02V") + id + "\x03";
		if (conn.write(command.c_str(), command.size()) == false) {
			xmlFreeDoc(doc);
			doc =
			    create_error_message("error",
						 "Regulator communication error");
			return doc;
		}

		char *buffer;
		int size;

		if (conn.read(&buffer, &size) == false) {
			xmlFree(doc);
			doc =
			    create_error_message("error",
						 "Regulator communication error");
			return doc;
		}

		regulator_id rid;
		if (parse_regulator_id(buffer, size, rid) == true) {
			free(buffer);
			append_regualtor_id(rid, doc);
			return doc;
		}
		free(buffer);
	}

	xmlFree(doc);
	doc =
	    create_error_message("error",
		 "Regulator communication error");
	return doc;

}

xmlDocPtr read_regulator_settings(connection& conn, char *id, bool* ok)
{
	xmlDocPtr result;
	for (int i = 0; i < max_communiation_attempts; i++) {
		string command = string("\x02Q") + id + "\x03";
		if (conn.write(command.c_str(), command.size()) == false) {
			result =
			    create_error_message("error",
						 "Regulator communication error");
			if (ok)
				*ok = false;
			return result;
		}

		char *buffer;
		int size;

		if (conn.read(&buffer, &size) == false) {
			result =
			    create_error_message("error",
						 "Regulator communication error");
			if (ok)
				*ok = false;
			return result;
		}

		result = parse_settings_response(buffer, size);
		free(buffer);

		if (result != NULL) {
			if (ok)
				*ok = true;
			return result;
		}
	}

	result = create_error_message("error", "Regulator communication error");
	if (ok)
		*ok = false;

	return result;

}

bool get_packs_type(xmlDocPtr request, PACKS * packs)
{
	bool ret = false;
	xmlXPathContextPtr xpath_ctx = xmlXPathNewContext(request);
	xmlChar *packs_type =
	    uxmlXPathGetProp(BAD_CAST "//message/packs/@type", xpath_ctx);
	if (packs_type == NULL) {
		sz_log(2,
		       "Invalid message from client - packs type attribute not present");
		goto end;
	}
	if (!xmlStrcmp(packs_type, BAD_CAST "WEEK")) {
		*packs = WEEK;
		ret = true;
	} else if (!xmlStrcmp(packs_type, BAD_CAST "DAY")) {
		*packs = DAY;
		ret = true;
	} else {
		sz_log(2,
		       "Invalid message from client - packs type attribute invalid %s",
		       (char *)packs_type);
	}
	xmlFree(packs_type);
end:
	xmlXPathFreeContext(xpath_ctx);

	return ret;
}

bool reset_packs(connection &conn, char *id, xmlDocPtr& error) {

	string command = string("\x11\x02R") + id + "ST\x03";

	for (int i = 0; i < max_communiation_attempts; ++i) {

		if (conn.write(command.c_str(), command.size()) == false) {
			error =
			    create_error_message("error",
						 "Regulator communication error");
			return false;
		}

		if (wait_for_ok(conn)) 
			return true;
	}

	error =
	    create_error_message("error", "Regulator does not respond");

	return false;
}


xmlDocPtr handle_set_packs_type_cmd(xmlDocPtr request, connection &conn, char *id, DaemonStopper& stopper)
{
	xmlDocPtr result = NULL;
	string command;
	char c = 0;
	bool ok = false;
	PACKS packs;

	if (get_packs_type(request, &packs) == false) {
		result =
		    create_error_message("invalid_message",
					 "Invalid packs type specification");
		goto end;
	}

	switch (packs) {
	case WEEK:
		c = 'W';
		break;
	case DAY:
		c = 'D';
		break;
	}

	command = string("\x11\x02W") + id + c + "X" + c + "\x03";

	if (!conn.open(&result, stopper))
		goto end;

	if (reset_packs(conn, id, result) == false) 
		goto end;

	for (int i = 0; i < max_communiation_attempts; i++) {

		if (conn.write(command.c_str(), command.size()) == false) {
			result =
			    create_error_message("error",
						 "Regulator communication error");
			goto end;
		}

		if (wait_for_ok(conn) == true) {
			ok = true;
			break;
		}
	}

	if (ok) {
		result = read_regulator_settings(conn, id, &ok);
		if (ok)
			result = read_regulator_id(result, conn, id);
	} else
		result =
		    create_error_message("error", "Regulator does not respond");

end:
	conn.close();
	stopper.StartDaemon();

	return result;

}

xmlDocPtr handle_reset_packs_cmd(xmlDocPtr request, connection &conn, char *id, DaemonStopper &stopper)
{
	xmlDocPtr result = NULL;
	if (conn.open(&result, stopper) == false)
		goto end;

	if (reset_packs(conn, id, result) == false) 
		goto end;

	bool ok;
	result = read_regulator_settings(conn, id, &ok);
	if (ok)
		result = read_regulator_id(result, conn, id);

	conn.close();
end:
	stopper.StartDaemon();

	return result;
}

xmlDocPtr parse_report(const char* response, int len) {
	const char *end = response + len - 1;
	/* find the 3rd '\r' from the end; our checksum should be right behind it */
	for (int i = 3; end > response; end--)
		if (*end == '\r' && --i == 0)
			break;

	if (end <= response) {
		sz_log(2,
		       "Unable to parse response from the regulator - it is too short %d", len);
		return NULL;
	}

	int received_checksum = atoi(end + 1);
	int calculated_checksum = 0;
	for (const char *c = response; c <= end; c++)
		calculated_checksum += *c;
	calculated_checksum &= 0xFFFF;

	if (calculated_checksum != received_checksum) {
		sz_log(2,
		       "Unable to parse response from the regulator - wrong checksum %d %d",
		       calculated_checksum, received_checksum);
		return NULL;
	}

	xmlDocPtr result = xmlNewDoc(X "1.0");
	xmlNodePtr root_node = xmlNewNode(NULL, X "message");
	xmlDocSetRootElement(result, root_node);
	xmlSetProp(root_node, X "type", X "report");

	istringstream is(string(response, end + 1));
	is.exceptions(ios_base::failbit | ios_base::badbit | ios_base::eofbit);

	try {
		std::string year, month, day, hour, minute;
		std::vector<std::string> vi;
		//skip header, 0xd char and first 
		is.ignore(6);
		std::getline(is, year, '/');
		xmlSetProp(root_node, X "year", X year.c_str());
		std::getline(is, month, '/');
		xmlSetProp(root_node, X "month", X month.c_str());
		std::getline(is, day, ' ');
		xmlSetProp(root_node, X "day", X day.c_str());
		std::getline(is, hour, ':');
		xmlSetProp(root_node, X "hour", X hour.c_str());
		std::getline(is, minute, '\r');
		xmlSetProp(root_node, X "minute", X minute.c_str());

		while (true) {
			try {
				char c;
				c = is.peek();
			} catch (ios_base::failure) {
				if (is.exceptions() & ios_base::eofbit && vi.size() > 0)
					//ok - end of message
					break;
				//sth wrong with the message
				throw;
			}
			std::string s;
			std::getline(is, s, '\r');
			if (s.empty())
				break;
			vi.push_back(s);
		}

		for (size_t i = 0; i < vi.size() - 1; i++) {
			xmlNodePtr param = xmlNewChild(root_node, NULL, X "param", NULL);
			xmlSetProp(param, X "value", X vi[i].c_str());
		}

	} catch (ios_base::failure) {
		sz_log(2, "Malformed response from regulator");
		xmlFreeDoc(result);
		return NULL;
	}

	return result;

}

bool parse_regulator_parameters(const char* buf, int len, xmlDocPtr result) {
	xmlNodePtr root_node = xmlDocGetRootElement(result);
	istringstream is(string(buf, buf + len));
	size_t i = 0;

	std::string vals[8];

	is.exceptions(ios_base::failbit | ios_base::badbit | ios_base::eofbit);
	try {

		is.ignore(1);
		for (i = 0; i < 8; i++)
			getline(is, vals[i], i % 2 ? '\r' : ' ');
		
	} catch (ios_base::failure) { }

	switch (i) {
		default:
			return false;
		case 8:
			xmlSetProp(root_node, X "nE", X vals[1].c_str());
			xmlSetProp(root_node, X "nL", X vals[3].c_str());
			xmlSetProp(root_node, X "nb", X vals[5].c_str());
			xmlSetProp(root_node, X "np", X vals[7].c_str());
			return true;
		case 4:
			xmlSetProp(root_node, X "nE", X vals[1].c_str());
			xmlSetProp(root_node, X "np", X vals[3].c_str());
			return true;
	}

}

void add_regulator_parameters_to_report(connection &conn, char *id, bool *ok, xmlDocPtr result) {
	for (int i = 0; i < max_communiation_attempts; i++) {
		string command = string("\x11\x02V") + id + "\x03\n";
		if (conn.write(command.c_str(), command.size()) == false) {
			xmlFreeDoc(result);
			result =
			    create_error_message("error",
						 "Regulator communication error");
			if (ok)
				*ok = false;
			return;
		}

		char *buffer;
		int size;

		if (conn.read(&buffer, &size)) {
			if (parse_regulator_parameters(buffer, size, result))
				return ;
		}
	}
}

xmlDocPtr read_report(connection &conn, char *id, bool *ok) {
	xmlDocPtr result;
	for (int i = 0; i < max_communiation_attempts; i++) {
		string command = string("\x11\x02P") + id + "\x03\n";
		if (conn.write(command.c_str(), command.size()) == false) {
			result =
			    create_error_message("error",
						 "Regulator communication error");
			if (ok)
				*ok = false;
			return result;
		}

		char *buffer;
		int size;

		if (conn.read(&buffer, &size) == false) {
			result =
			    create_error_message("error",
						 "Regulator communication error");
			if (ok)
				*ok = false;
			return result;
		}

		result = parse_report(buffer, size);
		free(buffer);

		if (result != NULL) {
			if (ok)
				*ok = true;
			add_regulator_parameters_to_report(conn, id, ok, result);
			return result;
		}
	}

	result = create_error_message("error", "Regulator communication error");
	if (ok)
		*ok = false;

	return result;

}

xmlDocPtr handle_get_report_cmd(xmlDocPtr request, connection &conn, char *id, DaemonStopper &stopper) {
	xmlDocPtr result;
	if (conn.open(&result, stopper) == false)
		goto end;

	result = read_report(conn, id, NULL);
	conn.close();
end:
	stopper.StartDaemon();

	return result;
}

xmlDocPtr handle_get_values_cmd(xmlDocPtr request, connection &conn, char *id, DaemonStopper &stopper)
{
	xmlDocPtr result;
	if (conn.open(&result, stopper) == false)
		goto end;

	bool ok;
	result = read_regulator_settings(conn, id, &ok);
	if (ok)
		result = read_regulator_id(result, conn, id);

	conn.close();
end:
	stopper.StartDaemon();

	return result;
}

bool send_set_command(connection &conn, char *id, string vals_type, string vals, xmlDocPtr * err)
{
	vals += '\x03';

	int checksum = 0;
	for (size_t i = 1; i < vals.size(); ++i)
		checksum += vals[i];
	checksum &= 0xFFFF;

	ostringstream oss;
	oss << "\x11\x02" << vals_type << id << checksum << vals;

	for (int i = 0; i < max_communiation_attempts; ++i) {
		if (conn.write(oss.str().c_str(), oss.str().size()) == false) {
			*err =
			    create_error_message("invalid_message",
						 "Regulator communiation error");
			return false;
		}

		if (wait_for_ok(conn) == true)
			return true;
	}
	*err =
	    create_error_message("error", "Regulator does not respond");
	return false;
}

bool get_param_data(xmlNodePtr node, string & number, string & value)
{
	char *c = (char *)xmlGetProp(node, BAD_CAST "number");
	if (c == NULL)
		return false;
	number = c;
	xmlFree(c);

	c = (char *)xmlGetProp(node, BAD_CAST "value");
	if (c == NULL)
		return false;
	value = c;
	xmlFree(c);
	return true;
}

bool prepare_constants_string(xmlDocPtr request, string & result)
{
	bool ret = false;
	int constants_count = 0;

	xmlXPathObjectPtr constants = NULL;
	xmlXPathContextPtr xpath_ctx = xmlXPathNewContext(request);

	constants =
	    xmlXPathEvalExpression(BAD_CAST "//constants/param", xpath_ctx);
	assert(constants);
	if (constants->nodesetval)
		for (int i = 0; i < constants->nodesetval->nodeNr; ++i) {
			xmlNodePtr node = constants->nodesetval->nodeTab[i];
			string number, value;

			if (get_param_data(node, number, value) == false) {
				sz_log(2,
				       "Error parsing request: invalid constant specifiation");
				goto end;
			}
			result += ",";
			result += number;
			result += ",";
			result += value;

			constants_count++;
		}
	if (constants_count == 0) {
		sz_log(2, "Error parsing request: no constans defined");
		goto end;
	}

	ret = true;
      end:
	xmlXPathFreeObject(constants);
	xmlXPathFreeContext(xpath_ctx);

	return ret;
}

xmlDocPtr handle_set_constans_cmd(xmlDocPtr request, connection& conn, char *id) {
	string command;
	xmlDocPtr result = NULL;

	if (prepare_constants_string(request, command) == false) {
		result =
		    create_error_message("invalid_message", "Invalid request");
		goto end;
	}

	if (send_set_command(conn, id, "C", command, &result) == false) {
		conn.close();
		goto end;
	}

	bool ok;
	result = read_regulator_settings(conn, id, &ok);
	if (ok)
		result = read_regulator_id(result, conn, id);
end:
	return result;
}


xmlDocPtr handle_set_constans_cmd(xmlDocPtr request, connection& conn, char *id, DaemonStopper &stopper)
{
	xmlDocPtr result = NULL;

	if (conn.open(&result, stopper)) {
		result = handle_set_constans_cmd(request, conn, id, stopper);
		conn.close();
	}
	stopper.StartDaemon();

	return result;
}

bool prepare_packs_string(xmlDocPtr request, vector<string> & results)
{
	int packs_count = 0;
	int param_count = 0;
	PACKS packs_type;
	xmlNodePtr node;
	ostringstream oss;
	xmlXPathObjectPtr packs = NULL;
	xmlXPathObjectPtr param_vals = NULL;
	xmlXPathContextPtr xpath_ctx = xmlXPathNewContext(request);
	char *c;
	bool ret = false;
	string day, hour, minute;
	string result;

	if (get_packs_type(request, &packs_type) == false) {
		sz_log(2,
		       "Error parsing set packs request: packs type specification missing");
		goto end;
	}


	packs = xmlXPathEvalExpression(BAD_CAST "//packs/pack", xpath_ctx);
	assert(packs);
	if (packs->nodesetval) for (int i = 0; i < packs->nodesetval->nodeNr; ++i) {
		oss.str(string());
		param_count = 0;

		node = packs->nodesetval->nodeTab[i];
		if (packs_type == WEEK) {
			c = (char *)xmlGetProp(node, BAD_CAST("day"));
			if (c == NULL) {
				sz_log(2,
				       "Error parsing request: attribute week is missing in definition of pack");
				goto end;
			}
			day = c;
			xmlFree(c);
		}

		c = (char *)xmlGetProp(node, BAD_CAST("hour"));
		if (c == NULL) {
			sz_log(2,
			       "Error parsing request: attribute hour is missing in definition of pack");
			goto end;
		}
		hour = c;
		xmlFree(c);

		c = (char *)xmlGetProp(node, BAD_CAST("minute"));
		if (c == NULL) {
			sz_log(2,
			       "Error parsing request: attribute minute is missing in definition of pack");
			goto end;
		}
		minute = c;
		//result += c;
		xmlFree(c);

		oss << "//packs/pack[position()=" << (i + 1) << "]/param";
		if (param_vals) {
			xmlXPathFreeObject(param_vals);
			param_vals = NULL;
		}
		param_vals =
		    xmlXPathEvalExpression(BAD_CAST oss.str().c_str(),
					   xpath_ctx);
		assert(param_vals);

		if (param_vals->nodesetval) for (int j = 0;
				     j < param_vals->nodesetval->nodeNr; ++j) {
			xmlNodePtr node =
				param_vals->nodesetval->nodeTab[j];
			string number, value;

			if (get_param_data(node, number, value) == false) {
				sz_log(2,
				       "Error parsing request: invalid pack specifiation");
				goto end;
			}

			if (result.size() > 200) {
				result += "$";
				results.push_back(result);
				result.clear();
			}

			if (result == "" || j == 0 ) {
				if (result == "")
					result = ",";
				else
					result += "$";

				if (packs_type == WEEK) {
					result += day;
					result += " ";
				}
				result += hour;
				result += ":";
				result += minute;
			}

			result += ",";
			result += number;
			result += ",";
			result += value;
			
			param_count++;

		} 
		if (param_count == 0) {
			sz_log(2,
			       "Error parsing request: invalid pack specifiation");
			goto end;
		}

		packs_count++;
	}

	if (packs_count == 0) {
		sz_log(2, "Invalid request, missing packs definitons");
		goto end;
	}

	result += "$";
	results.push_back(result);

	ret = true;

end:
	if (packs)
		xmlXPathFreeObject(packs);
	if (param_vals)
		xmlXPathFreeObject(param_vals);
	if (xpath_ctx)
		xmlXPathFreeContext(xpath_ctx);

	return ret;
}

xmlDocPtr handle_set_packs_cmd(xmlDocPtr request, connection& conn, char *id, DaemonStopper &stopper)
{
	xmlDocPtr result = NULL;
	xmlDocPtr current_settings = NULL;
	string command;

	vector<string> commands;
	if (prepare_packs_string(request, commands) == false) {
		result =
		    create_error_message("invalid_message", "Invalid request");
		goto end;
	}

	if (conn.open(&result, stopper) == false)
		goto end;

	current_settings = read_regulator_settings(conn, id); 
	if (current_settings == NULL) {
		    result = create_error_message("error", "Cannot read current settings from regulator");
		    conn.close();
		    goto end;
	}

	if (reset_packs(conn, id, result) == false)  {
		conn.close();
		goto end;
	}

	for (size_t i = 0; i < commands.size(); ++i) 
		if (send_set_command(conn, id, "T", commands[i], &result) == false) {
			conn.close();
			goto end;
		}

	result = handle_set_constans_cmd(current_settings, conn, id);

	conn.close();
end:
	if (current_settings)
		xmlFreeDoc(current_settings);
	stopper.StartDaemon();

	return result;
}

xmlDocPtr handle_command(xmlDocPtr request, DaemonStoppersFactory &stopper_factory)
{
	xmlDocPtr result = NULL;
	char *c;
	char *msg_type = NULL;
	char *path = NULL;
	char *ip_address = NULL;
	char *id = NULL;
	int speed;
	int port;
	xmlXPathContextPtr xpath_ctx = NULL;
	xmlNodePtr device;
	DaemonStopper *stopper = NULL;
	connection* conn = NULL;

	c = (char *)xmlGetProp(request->children, BAD_CAST "type");
	if (c == NULL) {
		sz_log(2,
		       "Invalid message from client - message type attribute missing");
		result =
		    create_error_message("invalid_message",
					 "Invalid message - message type attribute missing");
		goto end;
	}
	msg_type = c;

	xpath_ctx = xmlXPathNewContext(request);
	device = uxmlXPathGetNode(BAD_CAST "//device", xpath_ctx);
	if (device == NULL) {
		sz_log(2, "Invalid message from client - device node missing");
		result =
		    create_error_message("invalid_message",
					 "Invalid message - device missing");
		goto end;
	}

	c = (char *)xmlGetProp(device, BAD_CAST "path");
	if (c != NULL) {
		path = c;

		c = (char *)xmlGetProp(device, BAD_CAST "speed");
		if (c == NULL) {
			sz_log(2,
			       "Invalid message from client - speed attribute missing");
			result =
			    create_error_message("invalid_message",
						 "Invalid message - speed attribute missing");
			goto end;
		}
		speed = atoi(c);
		xmlFree(c);

		conn = new serial_connection(path, speed);
	} else if ((c = (char*) xmlGetProp(device, BAD_CAST "ip-address"))) {
		ip_address = c;
		c = (char *)xmlGetProp(device, BAD_CAST "port");
		if (c == NULL) {
			sz_log(2,
			       "Invalid message from client - port attribute missing");
			result =
			    create_error_message("invalid_message",
						 "Invalid message - port attribute missing");
			goto end;
		}
		port = atoi(c);
		xmlFree(c);

		conn = new tcp_connection(ip_address, port); 
	} else {
		sz_log(2,
		       "Invalid message from client - neither path nor ip-address attribute present");
		result =
		    create_error_message("invalid_message",
					 "Invalid message - netither path nor ip-address attribute present");
		goto end;
	}

	c = (char *)xmlGetProp(device, BAD_CAST "id");
	if (c == NULL) {
		sz_log(2, "Invalid message from client - id attribute missing");
		result =
		    create_error_message("invalid_message",
					 "Invalid message - id attribute missing");
		goto end;
	}
	id = c;


	stopper = stopper_factory.CreateDaemonStopper(path != NULL ? path : "");

	if (!strcmp(msg_type, "set_packs_type"))
		result = handle_set_packs_type_cmd(request, *conn, id, *stopper);
	else if (!strcmp(msg_type, "reset_packs"))
		result = handle_reset_packs_cmd(request, *conn, id, *stopper);
	else if (!strcmp(msg_type, "set_packs"))
		result = handle_set_packs_cmd(request, *conn, id, *stopper);
	else if (!strcmp(msg_type, "set_constants"))
		result = handle_set_constans_cmd(request, *conn, id, *stopper);
	else if (!strcmp(msg_type, "get_values"))
		result = handle_get_values_cmd(request, *conn, id, *stopper);
	else if (!strcmp(msg_type, "get_report"))
		result = handle_get_report_cmd(request, *conn, id, *stopper);
	else {
		sz_log(2,
		       "Invalid message from client - unknown message type %s",
		       msg_type);
		result =
		    create_error_message("invalid_message",
					 string("Invalid message type ") +
					 msg_type);
	}

      end:
	if (xpath_ctx)
		xmlXPathFreeContext(xpath_ctx);
	delete stopper;
	delete conn;
	xmlFree(msg_type);
	xmlFree(id);
	xmlFree(path);
	xmlFree(ip_address);

	return result;
}

}

