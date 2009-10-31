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

bool pdvalid(PD fd) {
#ifndef MINGW32
	return fd >= 0;
#else
	return fd.h != INVALID_HANDLE_VALUE;
#endif
}

void pdclose(PD fd) {
#ifndef MINGW32
	close(fd);
#else
	CloseHandle(fd.h);
	CloseHandle(fd.ovr.hEvent);
	CloseHandle(fd.ovw.hEvent);
#endif
}

	
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

#ifndef MINGW32 
PD prepare_device(char *path, int speed, xmlDocPtr * err_msg, DaemonStopper &stopper)
{
	PD fd;

	if (stopper.StopDaemon(err_msg) == false)
		return -1;

	if ((fd = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK, 0)) < 0) {
		*err_msg =
		    create_error_message("error", "Unable to open port");
		return -1;
	}

	struct termios ti;
	tcgetattr(fd, &ti);
	sz_log(5, "setting port speed to %d", speed);
	switch (speed) {
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

	tcsetattr(fd, TCSANOW, &ti);

	return fd;
}
#else
PD prepare_device(char *path, int speed, xmlDocPtr * err_msg, DaemonStopper &stopper)
{
	PD fd;

	if (stopper.StopDaemon(err_msg) == false)
		return fd;

	fd.h = CreateFile(path,  
		GENERIC_READ | GENERIC_WRITE, 
		0, 
		0, 
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED,
		0);

	if (fd.h == INVALID_HANDLE_VALUE) {
		*err_msg =
		    create_error_message("error", "Unable to open port");
		return fd;

	}


	DCB dcb;
	FillMemory(&dcb, sizeof(dcb), 0);
	dcb.DCBlength = sizeof(dcb);

	std::stringstream dcs;
	dcs << "baud=" << speed << " parity=n data=8 stop=1";
	if (!BuildCommDCB(dcs.str().c_str(), &dcb)) {
		*err_msg =
		    create_error_message("error", "Unable to set port settings");
		fd.h = INVALID_HANDLE_VALUE;
		pdclose(fd);
		return fd;
	}

	if (!SetCommState(fd.h, &dcb)) {
		*err_msg =
		    create_error_message("error", "Unable to set port settings");
		fd.h = INVALID_HANDLE_VALUE;
		return fd;
	}

	memset(&fd.ovr, 0, sizeof(fd.ovr));
	memset(&fd.ovw, 0, sizeof(fd.ovw));
	fd.ovw.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	fd.ovr.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	return fd;
}
#endif

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
bool write_port(PD fd, const char *buffer, int size)
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
#else
bool write_port(PD fd, const char *buffer, int size)
{
	bool ret = false;
	int total_written = 0;
	DWORD last_written = 0;
	time_t start = time(NULL);

	sz_log(2, "Writing to port:%s", create_printable_string(buffer, size).c_str());

	fd.ovw.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	while (total_written < size) {
		DWORD res;
		if (WriteFile(fd.h, buffer + total_written, size - total_written, &last_written, &fd.ovw)) {
			total_written += last_written;
			continue;
		}

		if (GetLastError() != ERROR_IO_PENDING)
			goto end;

		int wait = start + 10 - time(NULL);
		if (wait <= 0)
			goto end;
				
		res = WaitForSingleObject(fd.ovw.hEvent, wait * 1000);
		switch (res) {
			case WAIT_OBJECT_0:
				if (!GetOverlappedResult(fd.h, &fd.ovw, &last_written, FALSE))
					goto end;
				break;
			default:
				goto end;
		}

		total_written += last_written;
	}

	ret = true;
end:

	return ret;
}
#endif

#ifndef MINGW32
bool read_port(PD fd, char **buffer, int *size)
{
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
		ret = read(fd, *buffer + pos, buffer_size - pos);
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
#else
bool read_port(PD fd, char **buffer, int *size) {

	const int start_buffer_size = 512;
	const int max_buffer_size = start_buffer_size * 4;

	int buffer_size = start_buffer_size;
	*buffer = (char *)malloc(start_buffer_size);

	bool first = true;
	DWORD last_read;
	int was_read = 0;
	string r;

	while (true) {
		if (!ReadFile(fd.h, *buffer + was_read, buffer_size - was_read, &last_read, &fd.ovr)) {
			if (GetLastError() != ERROR_IO_PENDING)
				goto error;

			DWORD res;
			res = WaitForSingleObject(fd.ovr.hEvent, first ? 5000 : 1000);
			switch (res) {
				case WAIT_OBJECT_0:
					if (!GetOverlappedResult(fd.h, &fd.ovr, &last_read, FALSE))
						goto error;
					break;
				case WAIT_TIMEOUT:
					CancelIo(fd.h);
					goto out;
				default:
					goto error;
			}

		}

		first = false;

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
out:
	*size = was_read;
	sz_log(4, "Response read from port: %s", create_printable_string(*buffer, *size).c_str());
	return true;

error:
	free(*buffer);
	return false;
}

#endif

bool wait_for_ok(PD fd)
{
	static const char OK[] = "\x0dOK\x0d";
	char *data;
	int size;

	if (read_port(fd, &data, &size) == false)
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

xmlDocPtr read_regulator_id(xmlDocPtr doc, PD fd, char *id) {
	for (int i = 0; i < max_communiation_attempts; i++) {
		string command = string("\x11\x02V") + id + "\x03";
		if (write_port(fd, command.c_str(), command.size()) == false) {
			xmlFreeDoc(doc);
			doc =
			    create_error_message("error",
						 "Regulator communication error");
			return doc;
		}

		char *buffer;
		int size;

		if (read_port(fd, &buffer, &size) == false) {
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

xmlDocPtr read_regulator_settings(PD fd, char *id, bool* ok)
{
	xmlDocPtr result;
	for (int i = 0; i < max_communiation_attempts; i++) {
		string command = string("\x02Q") + id + "\x03";
		if (write_port(fd, command.c_str(), command.size()) == false) {
			result =
			    create_error_message("error",
						 "Regulator communication error");
			if (ok)
				*ok = false;
			return result;
		}

		char *buffer;
		int size;

		if (read_port(fd, &buffer, &size) == false) {
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

bool reset_packs(PD fd, char *id, xmlDocPtr& error) {

	string command = string("\x11\x02R") + id + "ST\x03";

	for (int i = 0; i < max_communiation_attempts; ++i) {

		if (write_port(fd, command.c_str(), command.size()) == false) {
			error =
			    create_error_message("error",
						 "Regulator communication error");
			return false;
		}

		if (wait_for_ok(fd)) 
			return true;
	}

	error =
	    create_error_message("error", "Regulator does not respond");

	return false;
}


xmlDocPtr handle_set_packs_type_cmd(xmlDocPtr request, char *path, char *id,
				    int speed, DaemonStopper& stopper)
{
	xmlDocPtr result = NULL;
	string command;
	char c = 0;
	PD fd = invalid_port_pd;
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

	fd = prepare_device(path, speed, &result, stopper);
	if (!pdvalid(fd)) 
		goto end;

	if (reset_packs(fd, id, result) == false) 
		goto end;

	for (int i = 0; i < max_communiation_attempts; i++) {

		if (write_port(fd, command.c_str(), command.size()) == false) {
			result =
			    create_error_message("error",
						 "Regulator communication error");
			goto end;
		}

		if (wait_for_ok(fd) == true) {
			ok = true;
			break;
		}
	}

	if (ok) {
		result = read_regulator_settings(fd, id, &ok);
		if (ok)
			result = read_regulator_id(result, fd, id);
	} else
		result =
		    create_error_message("error", "Regulator does not respond");

end:
	pdclose(fd);
	stopper.StartDaemon();

	return result;

}

xmlDocPtr handle_reset_packs_cmd(xmlDocPtr request, char *path, char *id,
				 int speed, DaemonStopper &stopper)
{
	xmlDocPtr result = NULL;
	PD fd = prepare_device(path, speed, &result, stopper);
	if (!pdvalid(fd)) 
		goto end;

	if (reset_packs(fd, id, result) == false) 
		goto end;

	bool ok;
	result = read_regulator_settings(fd, id, &ok);
	if (ok)
		result = read_regulator_id(result, fd, id);

	pdclose(fd);
end:
	stopper.StartDaemon();

	return result;
}

xmlDocPtr handle_get_values_cmd(xmlDocPtr request, char *path, char *id,
				int speed, DaemonStopper &stopper)
{
	xmlDocPtr result;
	PD fd = prepare_device(path, speed, &result, stopper);
	if (!pdvalid(fd))
		goto end;

	bool ok;
	result = read_regulator_settings(fd, id, &ok);
	if (ok)
		result = read_regulator_id(result, fd, id);

	pdclose(fd);
end:
	stopper.StartDaemon();

	return result;
}

bool send_set_command(PD fd, char *id, string vals_type, string vals,
		      xmlDocPtr * err)
{
	vals += '\x03';

	int checksum = 0;
	for (size_t i = 1; i < vals.size(); ++i)
		checksum += vals[i];
	checksum &= 0xFFFF;

	ostringstream oss;
	oss << "\x11\x02" << vals_type << id << checksum << vals;

	for (int i = 0; i < max_communiation_attempts; ++i) {
		if (write_port(fd, oss.str().c_str(), oss.str().size()) == false) {
			*err =
			    create_error_message("invalid_message",
						 "Regulator communiation error");
			return false;
		}

		if (wait_for_ok(fd) == true)
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

xmlDocPtr handle_set_constans_cmd(xmlDocPtr request, PD fd, char *id) {
	string command;
	xmlDocPtr result = NULL;

	if (prepare_constants_string(request, command) == false) {
		result =
		    create_error_message("invalid_message", "Invalid request");
		goto end;
	}

	if (send_set_command(fd, id, "C", command, &result) == false) {
		pdclose(fd);
		goto end;
	}

	bool ok;
	result = read_regulator_settings(fd, id, &ok);
	if (ok)
		result = read_regulator_id(result, fd, id);
end:
	return result;
}


xmlDocPtr handle_set_constans_cmd(xmlDocPtr request, char *path, char *id,
				  int speed, DaemonStopper &stopper)
{
	xmlDocPtr result = NULL;
	PD fd;

	fd = prepare_device(path, speed, &result, stopper);
	if (pdvalid(fd)) 
		result = handle_set_constans_cmd(request, fd, id);

	pdclose(fd);
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

xmlDocPtr handle_set_packs_cmd(xmlDocPtr request, char *path, char *id,
			       int speed, DaemonStopper &stopper)
{
	xmlDocPtr result = NULL;
	xmlDocPtr current_settings = NULL;
	string command;
	PD fd = invalid_port_pd;

	vector<string> commands;
	if (prepare_packs_string(request, commands) == false) {
		result =
		    create_error_message("invalid_message", "Invalid request");
		goto end;
	}

	fd = prepare_device(path, speed, &result, stopper);
	if (!pdvalid(fd))
		goto end;

	current_settings = read_regulator_settings(fd, id); 
	if (current_settings == NULL) {
		    result = create_error_message("error", "Cannot read current settings from regulator");
		    pdclose(fd);
		    goto end;
	}

	if (reset_packs(fd, id, result) == false)  {
		pdclose(fd);
		goto end;
	}

	for (size_t i = 0; i < commands.size(); ++i) 
		if (send_set_command(fd, id, "T", commands[i], &result) == false) {
			pdclose(fd);
			goto end;
		}

	result = handle_set_constans_cmd(current_settings, fd, id);

	pdclose(fd);
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
	char *id = NULL;
	int speed;
	xmlXPathContextPtr xpath_ctx = NULL;
	xmlNodePtr device;
	DaemonStopper *stopper = NULL;

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
	if (c == NULL) {
		sz_log(2,
		       "Invalid message from client - path attribute missing");
		result =
		    create_error_message("invalid_message",
					 "Invalid message - path attribute missing");
		goto end;
	}
	path = c;

	c = (char *)xmlGetProp(device, BAD_CAST "id");
	if (c == NULL) {
		sz_log(2, "Invalid message from client - id attribute missing");
		result =
		    create_error_message("invalid_message",
					 "Invalid message - id attribute missing");
		goto end;
	}
	id = c;

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

	stopper = stopper_factory.CreateDaemonStopper(path);

	if (!strcmp(msg_type, "set_packs_type"))
		result = handle_set_packs_type_cmd(request, path, id, speed, *stopper);
	else if (!strcmp(msg_type, "reset_packs"))
		result = handle_reset_packs_cmd(request, path, id, speed, *stopper);
	else if (!strcmp(msg_type, "set_packs"))
		result = handle_set_packs_cmd(request, path, id, speed, *stopper);
	else if (!strcmp(msg_type, "set_constants"))
		result = handle_set_constans_cmd(request, path, id, speed, *stopper);
	else if (!strcmp(msg_type, "get_values"))
		result = handle_get_values_cmd(request, path, id, speed, *stopper);
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

	xmlFree(msg_type);
	xmlFree(id);
	xmlFree(path);

	return result;
}

}

